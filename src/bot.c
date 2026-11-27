/*
 * bot.c - Hard-level bot: greedy captures + safe moves + exact endgame search.
 */
#include <stdlib.h>
#include <string.h>
#include "bot.h"

#define UNKNOWN 127              /* memo entry not computed yet (values are -20..20) */

/* ------------------------------------------------------------------ */
/* Random tie-breaking (small xorshift generator, so tests are repeatable) */

static uint32_t g_rng = 2463534242u;

void bot_seed(unsigned seed) { g_rng = seed ? seed : 2463534242u; }

static int rand_below(int n)
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return (int)(g_rng % (uint32_t)n);
}

/* ------------------------------------------------------------------ */
/* Geometry tables (copied once from board.c so the search loop is fast) */

static uint64_t t_box_mask[NUM_BOXES];
static int      t_edge_box[NUM_EDGES][2];
static int      t_ready = 0;

static void tables_init(void)
{
    if (t_ready)
        return;
    for (int b = 0; b < NUM_BOXES; b++)
        t_box_mask[b] = box_edge_mask(b);
    for (int e = 0; e < NUM_EDGES; e++) {
        t_edge_box[e][0] = edge_box(e, 0);
        t_edge_box[e][1] = edge_box(e, 1);
    }
    t_ready = 1;
}

static uint64_t bit(int e) { return UINT64_C(1) << e; }

/* Number of boxes completed by drawing `edge` (which must not be drawn yet in `lines`). O(1). */
static int boxes_closed(uint64_t lines, int edge)
{
    uint64_t after = lines | bit(edge);
    int n = 0;
    int b0 = t_edge_box[edge][0], b1 = t_edge_box[edge][1];
    if (b0 >= 0 && (after & t_box_mask[b0]) == t_box_mask[b0])
        n++;
    if (b1 >= 0 && (after & t_box_mask[b1]) == t_box_mask[b1])
        n++;
    return n;
}

int bot_is_capture(uint64_t lines, int edge)
{
    tables_init();
    return boxes_closed(lines, edge) > 0;
}

int bot_is_safe(uint64_t lines, int edge)
{
    tables_init();
    for (int k = 0; k < 2; k++) {
        int box = t_edge_box[edge][k];
        if (box >= 0 && __builtin_popcountll(lines & t_box_mask[box]) >= 2)
            return 0;                                 /* would become 3 (or 4) sides */
    }
    return 1;
}

/* How many boxes the opponent can grab, one after another, after we draw `edge`. */
static int boxes_given_away(uint64_t lines, int edge)
{
    lines |= bit(edge);
    int total = 0;
    for (;;) {
        int took = 0;
        for (int e = 0; e < NUM_EDGES; e++) {
            if (!(lines & bit(e)) && boxes_closed(lines, e) > 0) {
                total += boxes_closed(lines, e);
                lines |= bit(e);
                took = 1;
            }
        }
        if (!took)
            return total;
    }
}

/* ------------------------------------------------------------------ */
/* Exact endgame search                                                */
/*
 * Take a "root" position with k undrawn lines. Every later position of that endgame is
 * the root plus some of those k lines, so it is described by a k-bit number idx
 * (bit i set = the i-th free line has been drawn). memo[idx] stores the best final
 * score difference for the player to move, so each position is solved only once:
 *
 *   value(position) = max over free lines e of
 *        c + value(position + e)   if e completes c > 0 boxes (same player moves again)
 *          - value(position + e)   if e completes nothing      (turn passes)
 *
 * The bot keeps its memo between its turns: the positions later in the same game are
 * already in it, so only its first endgame move does the real work.
 */
typedef struct {
    uint64_t base;               /* lines drawn at the root                */
    int      free_e[NUM_EDGES];  /* the k free lines at the root           */
    int      k;
    int8_t  *memo;               /* 2^k entries, UNKNOWN if not solved yet */
} Solver;

static Solver g_solver;          /* the bot's own solver, reused between turns */

static int solve(Solver *s, uint32_t idx, uint64_t lines)
{
    if (s->memo[idx] != UNKNOWN)
        return s->memo[idx];
    if (lines == ALL_EDGES)
        return s->memo[idx] = 0;                      /* no line left: nothing more to win */

    int best = -NUM_BOXES - 1;
    for (int i = 0; i < s->k; i++) {
        if (idx & (1u << i))
            continue;
        int e = s->free_e[i];
        int c = boxes_closed(lines, e);
        int child = solve(s, idx | (1u << i), lines | bit(e));
        int v = c > 0 ? c + child : -child;
        if (v > best)
            best = v;
    }
    s->memo[idx] = (int8_t)best;
    return best;
}

/* Prepares s for root position `lines`. Returns 0 on success. */
static int solver_begin(Solver *s, uint64_t lines)
{
    s->base = lines;
    s->k = 0;
    for (int e = 0; e < NUM_EDGES; e++)
        if (!(lines & bit(e)))
            s->free_e[s->k++] = e;
    if (s->k > BOT_EXACT_LIMIT)
        return -1;

    size_t size = (size_t)1 << s->k;
    s->memo = malloc(size);
    if (s->memo == NULL)
        return -1;
    memset(s->memo, UNKNOWN, size);
    return 0;
}

static void solver_end(Solver *s)
{
    free(s->memo);
    s->memo = NULL;
}

/* idx of position `lines` inside solver s, or -1 if it is not a later position of s's root. */
static long solver_index(const Solver *s, uint64_t lines)
{
    if (s->memo == NULL || (lines & s->base) != s->base)
        return -1;
    uint32_t idx = 0;
    for (int i = 0; i < s->k; i++)
        if (lines & bit(s->free_e[i]))
            idx |= 1u << i;
    return (long)idx;
}

int bot_exact_value(uint64_t lines)
{
    Solver s;
    tables_init();
    if (solver_begin(&s, lines) != 0)
        return -128;
    int v = solve(&s, 0, lines);
    solver_end(&s);
    return v;
}

/* Exact best move; returns -1 if the position is too big or memory runs out. */
static int exact_move(uint64_t lines)
{
    Solver *s = &g_solver;
    long idx = solver_index(s, lines);
    if (idx < 0) {                                    /* new endgame: start a fresh memo */
        solver_end(s);
        if (solver_begin(s, lines) != 0)
            return -1;
        idx = 0;
    }

    int best = -NUM_BOXES - 1;
    int best_moves[NUM_EDGES];
    int nbest = 0;

    for (int i = 0; i < s->k; i++) {
        if ((uint32_t)idx & (1u << i))
            continue;
        int e = s->free_e[i];
        int c = boxes_closed(lines, e);
        int child = solve(s, (uint32_t)idx | (1u << i), lines | bit(e));
        int v = c > 0 ? c + child : -child;
        if (v > best) {
            best = v;
            nbest = 0;
        }
        if (v == best)
            best_moves[nbest++] = e;
    }
    return best_moves[rand_below(nbest)];
}

void bot_free(void)
{
    solver_end(&g_solver);
}

/* ------------------------------------------------------------------ */
/* Heuristic for the opening and middle game                           */

static int heuristic_move(uint64_t lines)
{
    int list[NUM_EDGES];
    int n = 0;

    /* a. Take a box if we can (prefer a line that closes two). */
    int best_c = 0;
    for (int e = 0; e < NUM_EDGES; e++) {
        if (lines & bit(e))
            continue;
        int c = boxes_closed(lines, e);
        if (c > best_c) {
            best_c = c;
            n = 0;
        }
        if (c > 0 && c == best_c)
            list[n++] = e;
    }
    if (n > 0)
        return list[rand_below(n)];

    /* b. Play a safe line (gives nothing away). Among them prefer lines next to boxes
     *    with fewer drawn sides, which keeps more safe lines for later. */
    int best_score = 99;
    for (int e = 0; e < NUM_EDGES; e++) {
        if ((lines & bit(e)) || !bot_is_safe(lines, e))
            continue;
        int score = 0;
        for (int k = 0; k < 2; k++) {
            int box = t_edge_box[e][k];
            if (box >= 0)
                score += __builtin_popcountll(lines & t_box_mask[box]);
        }
        if (score < best_score) {
            best_score = score;
            n = 0;
        }
        if (score == best_score)
            list[n++] = e;
    }
    if (n > 0)
        return list[rand_below(n)];

    /* c. Every line gives something away: give away as few boxes as possible. */
    int fewest = NUM_BOXES + 1;
    for (int e = 0; e < NUM_EDGES; e++) {
        if (lines & bit(e))
            continue;
        int g = boxes_given_away(lines, e);
        if (g < fewest) {
            fewest = g;
            n = 0;
        }
        if (g == fewest)
            list[n++] = e;
    }
    return n > 0 ? list[rand_below(n)] : -1;
}

/* ------------------------------------------------------------------ */

int bot_choose_move(const Board *b)
{
    uint64_t lines = b->lines;
    tables_init();
    if (lines == ALL_EDGES)
        return -1;

    int free_lines = NUM_EDGES - __builtin_popcountll(lines);
    int move = -1;

    if (free_lines <= BOT_EXACT_LIMIT)
        move = exact_move(lines);
    if (move < 0)                       /* too early for exact search (or out of memory) */
        move = heuristic_move(lines);
    return move;
}
