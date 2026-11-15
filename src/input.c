/*
 * input.c - Line-based console input.
 *
 * Every read goes through fgets so that a bad line never leaves junk in stdin,
 * and end of input (EOF) is always detected instead of looping forever.
 */
#include <stdio.h>
#include <string.h>
#include "input.h"

int input_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return -1;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        /* Line was longer than the buffer: throw away the rest of it. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    if (len > 0 && buf[len - 1] == '\r')
        buf[--len] = '\0';
    return 0;
}

int input_int(const char *prompt, int min, int max, int def, int *out)
{
    char line[128];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (input_line(line, sizeof line) < 0)
            return -1;

        if (line[0] == '\0' && def >= min) {
            *out = def;
            return 0;
        }

        int value;
        char extra;
        if (sscanf(line, "%d %c", &value, &extra) == 1 && value >= min && value <= max) {
            *out = value;
            return 0;
        }
        printf("Please enter a number between %d and %d.\n", min, max);
    }
}

int input_move(const Board *b, int player)
{
    char line[128];

    for (;;) {
        printf("Player %c's turn. Enter the row and column of the first dot "
               "(e.g., A0 -> 0 0) and second dot: ", player_letter(player));
        fflush(stdout);

        if (input_line(line, sizeof line) < 0)
            return -1;
        if (line[0] == 'q' || line[0] == 'Q')
            return -1;

        int r1, c1, r2, c2;
        char extra;
        if (sscanf(line, "%d %d %d %d %c", &r1, &c1, &r2, &c2, &extra) != 4) {
            printf("Invalid input. Type four numbers: row1 col1 row2 col2 (or q to quit).\n");
            continue;
        }

        int edge = board_edge_from_dots(r1, c1, r2, c2);
        if (edge < 0) {
            printf("Invalid move. The two dots must be next to each other (horizontally or "
                   "vertically), rows 0-%d and columns 0-%d.\n", DOT_ROWS - 1, DOT_COLS - 1);
            continue;
        }
        if (board_has_edge(b, edge)) {
            printf("That line is already drawn. Choose another one.\n");
            continue;
        }
        return edge;
    }
}
