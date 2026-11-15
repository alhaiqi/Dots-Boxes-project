/*
 * input.h - Safe console input (no scanf on raw stdin, handles EOF and bad input).
 */
#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include "board.h"

/* Reads one line (without the newline). Returns 0 on success, -1 on end of input. */
int input_line(char *buf, size_t size);

/* Asks until the user types an integer in [min, max]. An empty line gives def (if def >= min).
 * Returns 0 on success, -1 on end of input. */
int input_int(const char *prompt, int min, int max, int def, int *out);

/* Asks the given player for a move until it is valid.
 * Returns the line index, or -1 if the player typed 'q' or input ended. */
int input_move(const Board *b, int player);

#endif /* INPUT_H */
