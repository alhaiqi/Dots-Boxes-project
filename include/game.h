/*
 * game.h - Game loop for a two-player (A vs B) local game.
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

#endif /* GAME_H */
