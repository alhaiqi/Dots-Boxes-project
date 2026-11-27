/*
 * test_bot.c - Automated tests for the bot.  Run with:  make test
 *
 *   ./test_bot          full run (also plays many games against simple opponents)
 *   ./test_bot --quick  fewer games (used under Valgrind, which is slow)
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "board.h"
#include "bot.h"

static int E(int r1, int c1, int r2, int c2) { return board_edge_from_dots(r1, c1, r2, c2); }

/* ---- simple opponents ---- */

static uint32_t rng = 12345;
static int rnd(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 16) % (uint32_t)n); }

static int random_move(const Board *b)
{
    int list[NUM_EDGES], n = 0;
    for (int e = 0; e < NUM_EDGES; e++)
        if (!board_has_edge(b, e))
            list[n++] = e;
    return list[rnd(n)];
}

/* Takes a box if it can, else avoids 3rd sides, else random: a decent beginner. */
static int greedy_move(const Board *b)
{
    int list[NUM_EDGES], n = 0;
    for (int e = 0; e < NUM_EDGES; e++)
        if (!board_has_edge(b, e) && bot_is_capture(b->lines, e))
            return e;
    for (int e = 0; e < NUM_EDGES; e++)
        if (!board_has_edge(b, e) && bot_is_safe(b->lines, e))
            list[n++] = e;
    return n ? list[rnd(n)] : random_move(b);
}

/* ---- slow reference solver (plain recursion, no memo) to check the fast one ---- */

static int reference_value(uint64_t lines)
{
    if (lines == ALL_EDGES)
        return 0;
    int best = -100;
    for (int e = 0; e < NUM_EDGES; e++) {
        if (lines & (UINT64_C(1) << e))
            continue;
        Board tmp;
        board_init(&tmp);
        tmp.lines = lines;
        int c = board_play(&tmp, e, PLAYER_A);
        int child = reference_value(tmp.lines);
        int v = c > 0 ? c + child : -child;
        if (v > best)
            best = v;
    }
    return best;
}

/* ---- tests ---- */

static void test_safe_and_capture(void)
{
    Board b;
    board_init(&b);
    board_play(&b, E(0, 0, 0, 1), PLAYER_A);   /* box 0: top    */
    board_play(&b, E(0, 0, 1, 0), PLAYER_A);   /* box 0: left   */
    assert(!bot_is_safe(b.lines, E(1, 0, 1, 1)));   /* would be the 3rd side of box 0 */
    assert(bot_is_safe(b.lines, E(4, 4, 4, 5)));
    board_play(&b, E(1, 0, 1, 1), PLAYER_A);   /* box 0: bottom -> 3 sides */
    assert(bot_is_capture(b.lines, E(0, 1, 1, 1)));
    assert(bot_choose_move(&b) == E(0, 1, 1, 1));   /* bot takes the free box */
    printf("  safe lines / captures .. OK\n");
}

static void test_exact_against_reference(void)
{
    /* Random positions with 9 free lines: compare the memo solver with plain recursion. */
    for (int t = 0; t < 30; t++) {
        uint64_t lines = ALL_EDGES;
        int removed = 0;
        while (removed < 9) {
            int e = rnd(NUM_EDGES);
            if (lines & (UINT64_C(1) << e)) {
                lines &= ~(UINT64_C(1) << e);
                removed++;
            }
        }
        assert(bot_exact_value(lines) == reference_value(lines));
    }
    printf("  exact search = brute force (30 positions) OK\n");
}

/* Value of playing `move` in position `lines`, for the player who plays it. */
static int move_value(uint64_t lines, int move)
{
    Board tmp;
    board_init(&tmp);
    tmp.lines = lines;
    int got = board_play(&tmp, move, PLAYER_A);
    int child = bot_exact_value(tmp.lines);
    return got > 0 ? got + child : -child;
}

static void test_endgame_optimal(int positions)
{
    /*
     * Real endgames: two greedy players play until no safe line is left (every line now
     * gives boxes away), with a box possibly left to capture. In each such position the
     * bot's move must have the best possible value, and we count how often it beats the
     * greedy move (that is where chain tricks like the double-cross pay off).
     */
    int better = 0, tested = 0, total_gain = 0;
    while (tested < positions) {
        Board b;
        board_init(&b);
        int cur = PLAYER_A;
        while (!board_is_full(&b)) {
            int free_lines = NUM_EDGES - __builtin_popcountll(b.lines);
            int any_safe = 0;
            for (int e = 0; e < NUM_EDGES; e++)
                if (!board_has_edge(&b, e) && bot_is_safe(b.lines, e))
                    any_safe = 1;
            if (!any_safe && free_lines <= BOT_EXACT_LIMIT) {
                int move = bot_choose_move(&b);
                assert(move >= 0 && !board_has_edge(&b, move));
                int best = bot_exact_value(b.lines);
                int v = move_value(b.lines, move);
                int gv = move_value(b.lines, greedy_move(&b));
                assert(v == best);                 /* the bot's move is optimal  */
                assert(v >= gv);                   /* never worse than greedy    */
                if (v > gv) {
                    better++;
                    total_gain += v - gv;
                }
                tested++;
                if (tested >= positions)
                    break;
            }
            int e = greedy_move(&b);
            if (board_play(&b, e, cur) == 0)
                cur = 1 - cur;
        }
    }
    printf("  endgame moves optimal (%d positions; beats greedy in %d, by %d boxes total) OK\n",
           tested, better, total_gain);
}

/* Plays one game. bot_side plays bot_choose_move, the other side uses `opp`.
 * Checks every bot move is valid and never repeated. Returns bot score - opponent score. */
static int play_game(int bot_side, int (*opp)(const Board *))
{
    Board b;
    board_init(&b);
    int cur = PLAYER_A;
    int moves = 0;
    while (!board_is_full(&b)) {
        int e = (cur == bot_side) ? bot_choose_move(&b) : opp(&b);
        assert(e >= 0 && e < NUM_EDGES);
        assert(!board_has_edge(&b, e));              /* valid, never a repeat */
        if (board_play(&b, e, cur) == 0)
            cur = 1 - cur;
        moves++;
    }
    assert(moves == NUM_EDGES);
    assert(b.score[0] + b.score[1] == NUM_BOXES);
    return b.score[bot_side] - b.score[1 - bot_side];
}

static void match(const char *name, int (*opp)(const Board *), int games)
{
    int win = 0, draw = 0, loss = 0, diff = 0;
    clock_t t0 = clock();
    for (int g = 0; g < games; g++) {
        int d = play_game(g % 2, opp);              /* alternate who moves first */
        diff += d;
        if (d > 0) win++; else if (d == 0) draw++; else loss++;
    }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("  vs %-7s %3d games: %3d won, %d drawn, %d lost, avg margin %+.1f boxes (%.2f s/game)\n",
           name, games, win, draw, loss, (double)diff / games, secs / games);
    assert(win * 10 >= games * 8);                 /* the bot must win at least 80% */
}

int main(int argc, char **argv)
{
    int quick = argc > 1 && strcmp(argv[1], "--quick") == 0;
    bot_seed(777);

    printf("Running bot tests\n");
    test_safe_and_capture();
    test_exact_against_reference();
    test_endgame_optimal(quick ? 2 : 10);
    match("random", random_move, quick ? 1 : 40);
    match("greedy", greedy_move, quick ? 1 : 40);
    bot_free();
    printf("All bot tests passed.\n");
    return 0;
}
