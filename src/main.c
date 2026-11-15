/*
 * main.c - Entry point: main menu for Dots and Boxes.
 *
 * On the Alpine VM this program replaces the login prompt on tty1, so it should
 * never just exit on its own: after a game it returns to the menu.
 */
#include <stdio.h>
#include "input.h"
#include "game.h"

static void clear_screen(void)
{
    printf("\033[2J\033[H");     /* ANSI: clear screen, cursor to top-left */
}

static void print_rules(void)
{
    printf("\nHOW TO PLAY\n");
    printf("  - The board has 5 rows (0-4) and 6 columns (0-5) of dots, i.e. 4 x 5 boxes.\n");
    printf("  - On your turn, enter two adjacent dots as: row1 col1 row2 col2\n");
    printf("      e.g. 0 0 0 1 draws the line between dot (0,0) and dot (0,1).\n");
    printf("  - If your line completes a box, your letter goes in it and you play again.\n");
    printf("  - When all 20 boxes are claimed, the player with the most boxes wins.\n");
    printf("  - Type q instead of a move to stop the current game.\n\n");
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* show output immediately on the console */

    for (;;) {
        clear_screen();
        printf("==============================\n");
        printf("        DOTS AND BOXES\n");
        printf("==============================\n");
        printf("1) Two-player game (A vs B)\n");
        printf("2) How to play\n");
        printf("0) Exit\n");

        int choice;
        if (input_int("Choose an option: ", 0, 2, -1, &choice) < 0)
            return 0;                    /* end of input */

        if (choice == 0) {
            printf("Goodbye!\n");
            return 0;
        }

        if (choice == 1)
            game_play_two_players();
        else
            print_rules();

        printf("\nPress Enter to return to the menu...");
        char line[16];
        if (input_line(line, sizeof line) < 0)
            return 0;
    }
}
