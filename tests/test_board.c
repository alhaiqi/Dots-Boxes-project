/*
 * test_board.c - Automated tests for the board logic.  Run with:  make test
 */
#include <assert.h>
#include <stdio.h>
#include "board.h"

static int E(int r1, int c1, int r2, int c2) { return board_edge_from_dots(r1, c1, r2, c2); }

static void test_coordinates(void)
{
    assert(E(0, 0, 0, 1) == 0);
    assert(E(0, 1, 0, 0) == 0);            /* order of the dots does not matter */
    assert(E(0, 0, 1, 0) == NUM_H);
    assert(E(4, 4, 4, 5) == NUM_H - 1);
    assert(E(3, 5, 4, 5) == NUM_EDGES - 1);

    assert(E(0, 0, 1, 1) == -1);           /* diagonal          */
    assert(E(0, 0, 0, 2) == -1);           /* not adjacent      */
    assert(E(0, 0, 0, 0) == -1);           /* same dot          */
    assert(E(5, 0, 4, 0) == -1);           /* row out of range  */
    assert(E(0, 5, 0, 6) == -1);           /* col out of range  */
    assert(E(-1, 0, 0, 0) == -1);

    for (int e = 0; e < NUM_EDGES; e++) {  /* round trip for every line */
        int r1, c1, r2, c2;
        board_edge_to_dots(e, &r1, &c1, &r2, &c2);
        assert(E(r1, c1, r2, c2) == e);
    }
    printf("  coordinates ............ OK\n");
}

static void test_single_box(void)
{
    Board b;
    board_init(&b);
    assert(board_play(&b, E(0, 0, 0, 1), PLAYER_A) == 0);
    assert(board_play(&b, E(1, 0, 1, 1), PLAYER_B) == 0);
    assert(board_play(&b, E(0, 0, 1, 0), PLAYER_A) == 0);
    assert(board_play(&b, E(0, 1, 1, 1), PLAYER_B) == 1);   /* 4th side: B gets the box */
    assert(b.owner[0] == 'B');
    assert(b.score[PLAYER_B] == 1 && b.score[PLAYER_A] == 0);
    assert(board_has_edge(&b, E(0, 0, 0, 1)));
    printf("  single box ............. OK\n");
}

static void test_double_box(void)
{
    Board b;
    board_init(&b);
    int lines[] = { E(0,0,0,1), E(1,0,1,1), E(0,0,1,0),
                    E(0,1,0,2), E(1,1,1,2), E(0,2,1,2) };
    for (int i = 0; i < 6; i++)
        assert(board_play(&b, lines[i], PLAYER_A) == 0);
    assert(board_play(&b, E(0, 1, 1, 1), PLAYER_A) == 2);   /* middle line closes 2 boxes */
    assert(b.score[PLAYER_A] == 2 && b.owner[0] == 'A' && b.owner[1] == 'A');
    printf("  two boxes, one line .... OK\n");
}

static void test_full_board(void)
{
    Board b;
    board_init(&b);
    for (int e = 0; e < NUM_EDGES; e++) {
        assert(!board_is_full(&b));
        board_play(&b, e, e % 2);
    }
    assert(board_is_full(&b));
    assert(b.score[PLAYER_A] + b.score[PLAYER_B] == NUM_BOXES);
    for (int i = 0; i < NUM_BOXES; i++)
        assert(b.owner[i] == 'A' || b.owner[i] == 'B');
    printf("  full board ............. OK\n");
}

int main(void)
{
    printf("Running board tests\n");
    test_coordinates();
    test_single_box();
    test_double_box();
    test_full_board();
    printf("All tests passed.\n");
    return 0;
}
