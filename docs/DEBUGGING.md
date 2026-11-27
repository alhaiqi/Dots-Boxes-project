# Debugging with GDB and Valgrind

The Makefile always compiles with `-g -O0`, so every binary has debug symbols and
no optimisation, which is what GDB and Valgrind need for readable output.

Each team member should run these themselves and take screenshots for the report.

## Valgrind (memory errors and leaks)

```sh
make valgrind
```

This runs four programs under Valgrind with `--leak-check=full`:

1. `./test_board`: the automated board tests.
2. `./test_bot --quick`: the bot tests, with fewer games (Valgrind is about 50 times slower).
3. `./dots_and_boxes < tests/full_game.txt`: a scripted two-player game that goes through
   the menu, types invalid input (text, diagonal dots, dots off the board, a line that is
   already drawn), plays all 49 lines to the end and exits.
4. `./dots_and_boxes < tests/bot_game.txt`: a full game against the bot. The "human" tries
   every line in order; lines the bot already drew are rejected, so the game runs to the end.

5. A full network game between two copies of the game on 127.0.0.1 (`tests/net_game.sh`),
   both under Valgrind.

Expected result for all of them:

```
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

To check an interactive game by hand:

```sh
valgrind --leak-check=full --show-leak-kinds=all ./dots_and_boxes
```

**Why it is clean.** The board is a fixed-size struct on the stack. The only `malloc` is the
bot's endgame memo (up to 4 MB). It is freed when a new endgame starts, and `main` registers
`atexit(bot_free)` so it is also freed when the program exits. Also,
all input is read with `fgets` into fixed buffers, so it cannot overflow. Valgrind would
report invalid reads or writes if an index went out of range, for example if a line index
was computed wrongly from bad coordinates. That is why every coordinate is range-checked in
`board_edge_from_dots` before it is used.

## Helgrind (thread errors, Sprint 3)

```sh
make helgrind
```

Helgrind is a Valgrind tool that reports data races (two threads using the same memory
without a lock), wrong lock usage and lock-order problems. It runs a full network game
with both players under Helgrind. Expected: `ERROR SUMMARY: 0 errors` for both.

What it protects: the receiver thread writes the move queue and the `closed` flag, and the
main thread reads them. Every access is between `pthread_mutex_lock` and
`pthread_mutex_unlock` on `g->lock`. The main thread waits with
`pthread_cond_wait(&g->arrived, &g->lock)` inside a `while` loop, so it never misses a
move and never busy-waits. If you remove the lock around the queue in `receiver_main`,
Helgrind reports "Possible data race".

GDB with threads:

```
(gdb) break receiver_main            # stops in the receiver thread when it starts
(gdb) info threads                   # shows the main thread and the receiver thread
(gdb) break net_wait_move
(gdb) print *g                       # queue, count, closed
(gdb) thread 2                       # switch to the receiver thread, then bt
```

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
- `watch board.score[0]`: inside `play` (game.c), stop whenever A's score changes.

For the bot (Sprint 2):

```
(gdb) break bot_choose_move          # stop at every bot turn
(gdb) run                            # choose 2 (vs bot), then play
(gdb) print free_lines               # after a few `next`: how many lines are left
(gdb) break exact_move               # stop when the exact endgame search starts
(gdb) finish                         # run it and show the chosen line ("Value returned")
(gdb) print g_solver.k               # number of free lines the search works on
(gdb) print/x g_solver.base          # the lines already drawn when the search started
(gdb) break bot.c:solve if idx == 0  # stop only on the root call of the search
```

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

### Sprint 2 (bot)

| Bug | How it was found | Fix |
|-----|------------------|-----|
| The first version of the bot test took 10 minutes (about 3 s per game) | Timing `./test_bot` | The search loop called `edge_box()`/`box_edge_mask()` (which re-check `geometry_init`) millions of times: the bot now copies the tables once into its own arrays. The memo is also kept between the bot's turns, so only the first endgame move does a full search |
| The endgame memo would still be allocated at exit | Code review when the `malloc` was added (`--show-leak-kinds=all` would list it as "still reachable") | `bot_free()` is called through `atexit` in `main`, and at the end of `test_bot` |
| A test of "the bot beats greedy" never triggered on random positions | `test_bot` printed "better than greedy in 0" | Random positions are not real endgames; the test now builds endgames by playing greedy games until no safe line is left |
