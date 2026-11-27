/*
 * game.h - Game loops: two players (A vs B), a human against the hard bot, or a
 *          network game between two machines.
 */
#ifndef GAME_H
#define GAME_H

/* Result of a game. */
typedef enum {
    GAME_FINISHED = 0,   /* all boxes were claimed           */
    GAME_QUIT     = 1    /* a player typed q / input ended   */
} GameResult;

/* Plays one full two-player game on the console. */
GameResult game_play_two_players(void);

/* Plays one full game of a human against the bot.
 * human_first = 1: the human is Player A and moves first; 0: the bot is A and moves first. */
GameResult game_play_vs_bot(int human_first);

/* Plays one game against a player on another machine (Sprint 3).
 * hosting = 1: wait for the opponent on `port` and play A (moves first).
 * hosting = 0: connect to `host`:`port` and play B. */
GameResult game_play_network(int hosting, const char *host, int port);

#endif /* GAME_H */
