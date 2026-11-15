/*
 * game.c - Two-player game loop.
 *
 * Rules:
 *   - Players A and B take turns drawing a line between two adjacent dots.
 *   - A player who completes a box gets it (their letter is shown inside it)
 *     and plays again.
 *   - When all 20 boxes are claimed, the player with more boxes wins.
 */
#include <stdio.h>
#include "board.h"
#include "input.h"
#include "game.h"

#define SEPARATOR "***************************************************************************"

static void print_turn_result(const Board *b)
{
    printf("%s\n", SEPARATOR);
    board_print_scores(b);
    printf("%s\n", SEPARATOR);
    board_print(b);
}

static void announce_winner(const Board *b)
{
    int a = b->score[PLAYER_A];
    int bs = b->score[PLAYER_B];

    printf("\n================ GAME OVER ================\n");
    board_print_scores(b);
    if (a > bs)
        printf("Player A wins %d to %d!\n", a, bs);
    else if (bs > a)
        printf("Player B wins %d to %d!\n", bs, a);
    else
        printf("It's a draw, %d to %d!\n", a, bs);
    printf("===========================================\n");
}

GameResult game_play_two_players(void)
{
    Board board;
    int current = PLAYER_A;

    board_init(&board);
    board_print(&board);

    while (!board_is_full(&board)) {
        int edge = input_move(&board, current);
        if (edge < 0) {
            printf("\nGame ended by Player %c.\n", player_letter(current));
            return GAME_QUIT;
        }

        int completed = board_play(&board, edge, current);
        print_turn_result(&board);

        if (completed > 0) {
            if (!board_is_full(&board))
                printf("Player %c completed %d box%s and plays again.\n",
                       player_letter(current), completed, completed > 1 ? "es" : "");
        } else {
            current = (current == PLAYER_A) ? PLAYER_B : PLAYER_A;
        }
    }

    announce_winner(&board);
    return GAME_FINISHED;
}
