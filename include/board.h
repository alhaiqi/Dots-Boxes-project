/*
 * board.h - Board representation for Dots and Boxes (4 x 5 boxes, 5 x 6 dots).
 *
 * Every line (edge) on the board has an index from 0 to 48:
 *   - Horizontal line between dot (r,c) and dot (r,c+1):  index = r*5 + c        (0..24)
 *   - Vertical   line between dot (r,c) and dot (r+1,c):  index = 25 + r*6 + c   (25..48)
 *
 * The set of drawn lines is stored as a 64-bit bit mask: bit e is 1 if line e is drawn.
 * Boxes are numbered row by row: box (i,j) has index i*5 + j (0..19).
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define BOX_ROWS   4
#define BOX_COLS   5
#define DOT_ROWS   (BOX_ROWS + 1)              /* 5  */
#define DOT_COLS   (BOX_COLS + 1)              /* 6  */
#define NUM_H      (DOT_ROWS * BOX_COLS)       /* 25 horizontal lines */
#define NUM_V      (BOX_ROWS * DOT_COLS)       /* 24 vertical lines   */
#define NUM_EDGES  (NUM_H + NUM_V)             /* 49 lines in total   */
#define NUM_BOXES  (BOX_ROWS * BOX_COLS)       /* 20 boxes            */
#define ALL_EDGES  ((UINT64_C(1) << NUM_EDGES) - 1)

#define PLAYER_A 0
#define PLAYER_B 1

typedef struct {
    uint64_t lines;           /* bit e set  <=>  line e has been drawn          */
    char     owner[NUM_BOXES];/* ' ' if unclaimed, otherwise 'A' or 'B'        */
    int      score[2];        /* score[PLAYER_A], score[PLAYER_B]              */
} Board;

/* ---- game state ---- */
void board_init(Board *b);
int  board_play(Board *b, int edge, int player);   /* returns boxes completed (0, 1 or 2) */
int  board_has_edge(const Board *b, int edge);
int  board_is_full(const Board *b);
char player_letter(int player);

/* ---- coordinates <-> line index ---- */
int  board_edge_from_dots(int r1, int c1, int r2, int c2);   /* -1 if not two adjacent dots */
void board_edge_to_dots(int edge, int *r1, int *c1, int *r2, int *c2);

/* ---- output ---- */
void board_print(const Board *b);
void board_print_scores(const Board *b);

/* ---- geometry helpers (used by the bot) ---- */
int      edge_box(int edge, int k);    /* k = 0 or 1: index of an adjacent box, or -1 */
uint64_t box_edge_mask(int box);       /* bit mask of the 4 lines around a box        */
int      box_sides(uint64_t lines, int box); /* how many of the box's 4 lines are drawn */

#endif /* BOARD_H */
