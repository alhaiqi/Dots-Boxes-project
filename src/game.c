/*
 * game.c - Game loop, for two humans or a human against the bot.
 *
 * Rules:
 *   - Players A and B take turns drawing a line between two adjacent dots.
 *   - A player who completes a box gets it (their letter is shown inside it)
 *     and plays again.
 *   - When all 20 boxes are claimed, the player with more boxes wins.
 *
 * Each player is one of three kinds: a human at this keyboard, the bot, or a remote
 * player whose moves arrive over the network (Sprint 3).
 */
#include <stdio.h>
#include "board.h"
#include "input.h"
#include "bot.h"
#include "net.h"
#include "game.h"

#define SEPARATOR "***************************************************************************"

static void print_turn_result(const Board *b)
{
    printf("%s\n", SEPARATOR);
    board_print_scores(b);
    printf("%s\n", SEPARATOR);
    board_print(b);
}

typedef enum { LOCAL_HUMAN, BOT, REMOTE } PlayerKind;

static const char *kind_label(const PlayerKind kind[2], int p)
{
    if (kind[p] == BOT)
        return " (bot)";
    if (kind[p] == REMOTE)
        return " (opponent)";
    if (kind[1 - p] != LOCAL_HUMAN)
        return " (you)";
    return "";
}

static void announce_winner(const Board *b, const PlayerKind kind[2])
{
    int a = b->score[PLAYER_A];
    int bs = b->score[PLAYER_B];

    printf("\n================ GAME OVER ================\n");
    board_print_scores(b);
    if (a == bs) {
        printf("It's a draw, %d to %d!\n", a, bs);
    } else {
        int w = a > bs ? PLAYER_A : PLAYER_B;
        printf("Player %c%s wins %d to %d!\n", player_letter(w), kind_label(kind, w),
               b->score[w], b->score[1 - w]);
    }
    printf("===========================================\n");
}

/* Shared loop. kind[p] says who plays player p; net is only used for REMOTE games. */
static GameResult play(const PlayerKind kind[2], NetGame *net)
{
    Board board;
    int current = PLAYER_A;

    board_init(&board);
    board_print(&board);

    while (!board_is_full(&board)) {
        int edge;

        if (kind[current] == BOT) {
            printf("Player %c (bot) is thinking...\n", player_letter(current));
            edge = bot_choose_move(&board);
            /* Safety net: the bot must only choose a line that is not drawn yet. */
            if (edge < 0 || edge >= NUM_EDGES || board_has_edge(&board, edge)) {
                printf("Internal error: the bot chose an invalid line (%d).\n", edge);
                return GAME_QUIT;
            }
            int r1, c1, r2, c2;
            board_edge_to_dots(edge, &r1, &c1, &r2, &c2);
            printf("Player %c (bot) plays: %d %d %d %d\n", player_letter(current),
                   r1, c1, r2, c2);
        } else if (kind[current] == REMOTE) {
            printf("Waiting for Player %c's move...\n", player_letter(current));
            edge = net_wait_move(net);
            if (edge < 0) {
                printf("\nPlayer %c left the game or the connection was lost.\n",
                       player_letter(current));
                return GAME_QUIT;
            }
            if (board_has_edge(&board, edge)) {     /* never trust the network */
                printf("\nPlayer %c sent a line that is already drawn. Game stopped.\n",
                       player_letter(current));
                net_send_quit(net);
                return GAME_QUIT;
            }
            int r1, c1, r2, c2;
            board_edge_to_dots(edge, &r1, &c1, &r2, &c2);
            printf("Player %c plays: %d %d %d %d\n", player_letter(current), r1, c1, r2, c2);
        } else {
            edge = input_move(&board, current);
            if (edge < 0) {
                printf("\nGame ended by Player %c.\n", player_letter(current));
                if (net != NULL)
                    net_send_quit(net);
                return GAME_QUIT;
            }
            if (net != NULL && net_send_move(net, edge) < 0) {
                printf("\nConnection lost.\n");
                return GAME_QUIT;
            }
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

    announce_winner(&board, kind);
    return GAME_FINISHED;
}

GameResult game_play_two_players(void)
{
    const PlayerKind kind[2] = { LOCAL_HUMAN, LOCAL_HUMAN };
    return play(kind, NULL);
}

GameResult game_play_vs_bot(int human_first)
{
    PlayerKind kind[2];
    kind[PLAYER_A] = human_first ? LOCAL_HUMAN : BOT;
    kind[PLAYER_B] = human_first ? BOT : LOCAL_HUMAN;
    printf("\nYou are Player %c. The bot is Player %c.\n",
           human_first ? 'A' : 'B', human_first ? 'B' : 'A');
    return play(kind, NULL);
}

GameResult game_play_network(int hosting, const char *host, int port)
{
    NetGame *net = hosting ? net_host(port) : net_join(host, port);
    if (net == NULL)
        return GAME_QUIT;

    PlayerKind kind[2];
    kind[PLAYER_A] = hosting ? LOCAL_HUMAN : REMOTE;   /* the host is A and starts */
    kind[PLAYER_B] = hosting ? REMOTE : LOCAL_HUMAN;
    printf("\nYou are Player %c. Your opponent is Player %c.\n",
           hosting ? 'A' : 'B', hosting ? 'B' : 'A');

    GameResult r = play(kind, net);
    net_close(net);
    return r;
}
