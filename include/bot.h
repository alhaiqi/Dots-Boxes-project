/*
 * bot.h - Hard-level computer player for Dots and Boxes.
 *
 * Strategy (details and complexity in docs/COMPLEXITY.md):
 *   1. Opening / middle game (more than BOT_EXACT_LIMIT lines left):
 *        a. take every box that can be completed,
 *        b. otherwise play a "safe" line, one that does not draw the 3rd side of any box,
 *        c. if no safe line exists, sacrifice the line that gives the opponent the
 *           fewest boxes.
 *   2. Endgame (BOT_EXACT_LIMIT lines or fewer left): an exact search (negamax with a
 *      memo table over every remaining position) finds the move with the best possible
 *      final score. This is where real games are won: it plays the chains correctly,
 *      including the "double-cross" (leaving the last 2 boxes of a chain on purpose).
 *
 * The bot only ever returns a line that is not drawn yet, so it cannot make an
 * invalid or repeated move.
 */
#ifndef BOT_H
#define BOT_H

#include <stdint.h>
#include "board.h"

/* Exact search is used when this many undrawn lines (or fewer) remain.
 * 22 lines -> at most 2^22 = 4 194 304 positions, 4 MB of memo, well under a second. */
#define BOT_EXACT_LIMIT 22

/* Seeds the bot's random tie-breaking (use a fixed seed for reproducible tests). */
void bot_seed(unsigned seed);

/* Returns the line (0..48) the bot plays on board b. Always an undrawn line.
 * Returns -1 only if the board is already full. */
int bot_choose_move(const Board *b);

/* Frees the bot's endgame memo (call before the program exits). */
void bot_free(void);

/* ---- exposed for the unit tests ---- */

/* Best final score difference (player to move minus opponent) from position `lines`,
 * counting only boxes still to be won. Exact; needs at most BOT_EXACT_LIMIT free lines.
 * Returns -128 if there are too many free lines or memory runs out. */
int bot_exact_value(uint64_t lines);

/* 1 if drawing `edge` would complete at least one box. */
int bot_is_capture(uint64_t lines, int edge);

/* 1 if drawing `edge` neither completes a box nor draws the 3rd side of a box. */
int bot_is_safe(uint64_t lines, int edge);

#endif /* BOT_H */
