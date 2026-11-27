/*
 * main.c - Entry point: main menu for Dots and Boxes.
 *
 * On the Alpine VM this program replaces the login prompt on tty1, so it should
 * never just exit on its own: after a game it returns to the menu.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "input.h"
#include "bot.h"
#include "net.h"
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
    printf("  - Type q instead of a move to stop the current game.\n");
    printf("  - Against the bot, you can choose to move first (Player A) or second (B).\n");
    printf("  - Network game: one machine hosts (Player A, moves first), the other joins\n");
    printf("    with the host's IP address (Player B). Find the IP with: ip addr\n\n");
}

/* Asks host/join, IP and port, then plays. Returns -1 on end of input. */
static int network_menu(void)
{
    int mode, port;
    char host[128];

    if (input_int("1) Host a game (you are A)  2) Join a game (you are B)  [1]: ",
                  1, 2, 1, &mode) < 0)
        return -1;
    if (mode == 2) {
        printf("Host IP address [127.0.0.1]: ");
        if (input_line(host, sizeof host) < 0)
            return -1;
        if (host[0] == '\0')
            strcpy(host, "127.0.0.1");
    }
    char prompt[64];
    snprintf(prompt, sizeof prompt, "Port [%d]: ", NET_DEFAULT_PORT);
    if (input_int(prompt, 1024, 65535, NET_DEFAULT_PORT, &port) < 0)
        return -1;

    game_play_network(mode == 1, host, port);
    return 0;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* show output immediately on the console */
    bot_seed((unsigned)time(NULL));     /* the bot varies its choice between equal moves */
    atexit(bot_free);                   /* release the bot's endgame memo on exit */

    for (;;) {
        clear_screen();
        printf("==============================\n");
        printf("        DOTS AND BOXES\n");
        printf("==============================\n");
        printf("1) Two-player game (A vs B)\n");
        printf("2) Play against the bot (hard)\n");
        printf("3) Network game (two machines)\n");
        printf("4) How to play\n");
        printf("0) Exit\n");

        int choice;
        if (input_int("Choose an option: ", 0, 4, -1, &choice) < 0)
            return 0;                    /* end of input */

        if (choice == 0) {
            printf("Goodbye!\n");
            return 0;
        }

        if (choice == 1) {
            game_play_two_players();
        } else if (choice == 2) {
            int first;
            if (input_int("Who moves first? 1) You  2) The bot  [1]: ", 1, 2, 1, &first) < 0)
                return 0;
            game_play_vs_bot(first == 1);
        } else if (choice == 3) {
            if (network_menu() < 0)
                return 0;
        } else {
            print_rules();
        }

        printf("\nPress Enter to return to the menu...");
        char line[16];
        if (input_line(line, sizeof line) < 0)
            return 0;
    }
}
