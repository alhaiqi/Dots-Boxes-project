# Debugging with GDB and Valgrind

The Makefile always compiles with `-g -O0`, so every binary has debug symbols and
no optimisation, which is what GDB and Valgrind need for readable output.

Each team member should run these themselves and take screenshots for the report.

## Valgrind (memory errors and leaks)

```sh
make valgrind
```

This runs two programs under Valgrind with `--leak-check=full`:

1. `./test_board`: the automated board tests.
2. `./dots_and_boxes < tests/full_game.txt`: a scripted game that goes through the
   menu, types invalid input (text, diagonal dots, dots off the board, a line that is
   already drawn), plays all 49 lines to the end and exits.

Expected result for both:

```
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

To check an interactive game by hand:

```sh
valgrind --leak-check=full --show-leak-kinds=all ./dots_and_boxes
```

**Why it is clean.** The board is a fixed-size struct on the stack (no `malloc`), and
all input is read with `fgets` into fixed buffers, so it cannot overflow. Valgrind would
report invalid reads or writes if an index went out of range, for example if a line index
was computed wrongly from bad coordinates. That is why every coordinate is range-checked in
`board_edge_from_dots` before it is used.

## GDB (stepping through the game logic)

```sh
gdb ./dots_and_boxes
```

Useful session:

```
(gdb) break board_play              # stop every time a line is drawn
(gdb) run                           # play: enter 1, then a move
(gdb) print edge                    # index of the line (0..48)
(gdb) print *b                      # whole board: lines mask, owners, scores
(gdb) print/x b->lines              # drawn lines as a bit mask
(gdb) next                          # step through the box-completion check
(gdb) print completed               # how many boxes this line closed
(gdb) bt                            # call stack: main -> game_play_two_players -> board_play
(gdb) continue
```

Other breakpoints worth showing:

- `break input_move`: watch invalid input being rejected.
- `break board_edge_from_dots`: see how `r1 c1 r2 c2` becomes a line index.
- `break game.c:59 if completed > 0`: stop only when a box is completed.
- `watch board.score[0]`: inside `game_play_two_players`, stop whenever A's score changes.

Run GDB with a scripted game instead of typing:

```
(gdb) run < tests/full_game.txt
```

## Bugs found during development (examples for the report)

| Bug | How it was found | Fix |
|-----|------------------|-----|
| Typing letters made `scanf` loop forever printing "Invalid input" | Running the game with input piped from a file | All input is read with `fgets` + `sscanf`, and EOF ends the game cleanly |
| Two dots far apart (e.g. `0 0 0 2`) were accepted | Unit test `test_coordinates` | `board_edge_from_dots` checks that the dots differ by exactly 1 in one direction |
| Line closing two boxes only awarded one | Unit test `test_double_box` | `board_play` checks *both* boxes next to the line |

(Replace or add to this table with the bugs your team actually hit.)
