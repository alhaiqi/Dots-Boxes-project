/*
 * board.c - Board state, move application, box-completion check and printing.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"

/* Precomputed geometry tables, filled once by geometry_init(). */
static uint64_t g_box_mask[NUM_BOXES];   /* lines around each box          */
static int      g_edge_box[NUM_EDGES][2];/* boxes on each side of a line   */
static int      g_ready = 0;

static int h_index(int r, int c) { return r * BOX_COLS + c; }          /* horizontal line */
static int v_index(int r, int c) { return NUM_H + r * DOT_COLS + c; }  /* vertical line   */

static void geometry_init(void)
{
    if (g_ready)
        return;

    for (int e = 0; e < NUM_EDGES; e++)
        g_edge_box[e][0] = g_edge_box[e][1] = -1;

    for (int i = 0; i < BOX_ROWS; i++) {
        for (int j = 0; j < BOX_COLS; j++) {
            int box = i * BOX_COLS + j;
            int sides[4] = { h_index(i, j), h_index(i + 1, j), v_index(i, j), v_index(i, j + 1) };

            g_box_mask[box] = 0;
            for (int k = 0; k < 4; k++) {
                int e = sides[k];
                g_box_mask[box] |= UINT64_C(1) << e;
                if (g_edge_box[e][0] == -1)
                    g_edge_box[e][0] = box;
                else
                    g_edge_box[e][1] = box;
            }
        }
    }
    g_ready = 1;
}

int edge_box(int edge, int k)        { geometry_init(); return g_edge_box[edge][k]; }
uint64_t box_edge_mask(int box)      { geometry_init(); return g_box_mask[box]; }
int box_sides(uint64_t lines, int box)
{
    return __builtin_popcountll(lines & box_edge_mask(box));
}

char player_letter(int player) { return player == PLAYER_A ? 'A' : 'B'; }

void board_init(Board *b)
{
    geometry_init();
    b->lines = 0;
    memset(b->owner, ' ', sizeof b->owner);
    b->score[PLAYER_A] = 0;
    b->score[PLAYER_B] = 0;
}

int board_has_edge(const Board *b, int edge)
{
    return (int)((b->lines >> edge) & 1u);
}

int board_is_full(const Board *b)
{
    return b->lines == ALL_EDGES;
}

/*
 * Draw a line and award any box it completes.
 * Only the (at most two) boxes touching the line can be completed, so this is O(1).
 */
int board_play(Board *b, int edge, int player)
{
    int completed = 0;

    b->lines |= UINT64_C(1) << edge;

    for (int k = 0; k < 2; k++) {
        int box = g_edge_box[edge][k];
        if (box >= 0 && (b->lines & g_box_mask[box]) == g_box_mask[box]) {
            b->owner[box] = player_letter(player);
            b->score[player]++;
            completed++;
        }
    }
    return completed;
}

/* Returns the line index joining two dots, or -1 if they are not adjacent dots on the grid. */
int board_edge_from_dots(int r1, int c1, int r2, int c2)
{
    if (r1 < 0 || r1 >= DOT_ROWS || r2 < 0 || r2 >= DOT_ROWS ||
        c1 < 0 || c1 >= DOT_COLS || c2 < 0 || c2 >= DOT_COLS)
        return -1;

    if (r1 == r2 && (c1 - c2 == 1 || c2 - c1 == 1))          /* horizontal */
        return h_index(r1, c1 < c2 ? c1 : c2);

    if (c1 == c2 && (r1 - r2 == 1 || r2 - r1 == 1))          /* vertical   */
        return v_index(r1 < r2 ? r1 : r2, c1);

    return -1;
}

void board_edge_to_dots(int edge, int *r1, int *c1, int *r2, int *c2)
{
    if (edge < NUM_H) {
        *r1 = *r2 = edge / BOX_COLS;
        *c1 = edge % BOX_COLS;
        *c2 = *c1 + 1;
    } else {
        int v = edge - NUM_H;
        *r1 = v / DOT_COLS;
        *r2 = *r1 + 1;
        *c1 = *c2 = v % DOT_COLS;
    }
}

/*
 * Prints the board in the same format as the project's sample run:
 *
 *     0 1 2 3 4 5
 *   0 .-.-. . . .
 *     |A|
 *   1 .-. . . . .
 */
void board_print(const Board *b)
{
    printf("  ");
    for (int c = 0; c < DOT_COLS; c++)
        printf("%d ", c);
    printf("\n");

    for (int r = 0; r < DOT_ROWS; r++) {
        printf("%d ", r);
        for (int c = 0; c < DOT_COLS; c++) {
            putchar('.');
            if (c < BOX_COLS)
                putchar(board_has_edge(b, h_index(r, c)) ? '-' : ' ');
        }
        printf("\n");

        if (r < BOX_ROWS) {
            printf("  ");
            for (int c = 0; c < DOT_COLS; c++) {
                putchar(board_has_edge(b, v_index(r, c)) ? '|' : ' ');
                if (c < BOX_COLS)
                    putchar(b->owner[r * BOX_COLS + c]);
            }
            printf("\n");
        }
    }
}

void board_print_scores(const Board *b)
{
    printf("Player A score: %d\n", b->score[PLAYER_A]);
    printf("Player B score: %d\n", b->score[PLAYER_B]);
}
