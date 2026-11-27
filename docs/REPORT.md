# Dots and Boxes in C: Final Report (Sprints 1, 2 and 3)

**Course:** CMPS 241, Systems Programming. **Group:** Faqih Group.
**Team members:** _(add names and IDs)_

## 1. Introduction

Dots and Boxes is a two-player pen-and-paper game. Players take turns drawing a line between
two neighbouring dots. A player who draws the 4th side of a box wins that box, writes their
letter in it, and must play again. When all boxes are taken, the player with more boxes wins.

We wrote a console version in C for a 4 x 5 grid of boxes (5 x 6 dots, 49 lines, 20 boxes).
It runs on Alpine Linux in a virtual machine and starts automatically when the VM boots,
without a login prompt.

- **Sprint 1:** two human players (A and B).
- **Sprint 2:** a human against a hard-level bot.
- **Sprint 3 (bonus):** two players on two machines over the network, with a receiver
  thread and a mutex.

## 2. Program design

The program is split into modules, each with a header (`include/`) and a source file (`src/`):

| Module | Responsibility |
|--------|----------------|
| `board.h/.c` | Board data, drawing a line, detecting completed boxes, printing the board and scores |
| `input.h/.c` | Safe console input with `fgets` + `sscanf`: bad text, EOF and long lines are handled |
| `game.h/.c` | The game loop: turns, extra turn after a box, bot turns, winner |
| `bot.h/.c` | The hard-level bot |
| `net.h/.c` | Network play: TCP host/join, message protocol, receiver thread and its lock |
| `main.c` | Main menu: two players, play against the bot, network game, rules, exit |

A `Makefile` builds everything with `-Wall -Wextra -Wpedantic -g -O0`, so there are no
warnings and the binary has debug symbols for GDB. `make test` runs the unit tests, and
`make valgrind` runs the tests and two scripted games under Valgrind.

### Board representation

Every line has a number from 0 to 48 (25 horizontal, 24 vertical). The drawn lines are stored
in **one 64-bit integer** used as a bit mask: bit *e* is 1 when line *e* is drawn. Two tables
are computed once at start-up: the 4 lines around each box (as a mask) and the 1 or 2 boxes
next to each line. With these:

- checking whether a line is drawn is one shift and one AND,
- checking whether a move completes a box only looks at the ≤ 2 boxes next to the new line,
  and each check is `(lines & box_mask) == box_mask`. This is **O(1)**,
- the whole position is a single number, so the bot can copy it and use it as a key cheaply.

## 3. Sprint 1: two-player game

- The board is printed after every move with the scores, and the letters A/B inside the
  boxes that are taken.
- A move is typed as `row1 col1 row2 col2`. The program rejects text, dots off the board,
  dots that are not neighbours (including diagonals) and lines already drawn, and asks the
  same player again.
- A player who completes a box (or 2 boxes with one line) plays again.
- When all 20 boxes are claimed, the program announces the winner or a draw, then returns
  to the menu.

## 4. Sprint 2: the hard-level bot

The menu has a new option, **2) Play against the bot (hard)**. The human chooses who moves
first. The bot's moves are printed in the same format as a human's (for example
`Player B (bot) plays: 0 1 0 2`), and the board and both scores are shown after every move.

### 4.1 Why the endgame matters

In Dots and Boxes the start of the game is quiet: most lines are **safe**, which means they
don't draw the 3rd side of a box, so they give nothing away. Once the safe lines run out,
every move hands boxes to the opponent, usually a whole **chain** of boxes that can be taken
one after another. The player who controls the order in which the chains are opened usually
wins. The key trick is the **double-cross**: when taking a chain, you leave its last two boxes
on purpose. The opponent takes those two but must then open the next, longer chain for you.

### 4.2 Strategy

The bot (`bot_choose_move` in `src/bot.c`) works in two phases.

**Phase 1: heuristic (more than 22 lines left).**
1. If a line completes a box, take it (a line that completes two boxes is chosen first).
2. Otherwise play a safe line. Among safe lines, prefer those next to boxes with fewer drawn
   sides, which keeps more safe lines for later.
3. If no safe line is left, sacrifice: for every line, simulate the opponent taking every box
   it can, and play the line that gives away the fewest boxes.

**Phase 2: exact search (22 lines or fewer left).** The bot solves the rest of the game
perfectly with negamax (minimax where my score is minus my opponent's) and a memo table:

```
value(P) = 0                                if no line is left
value(P) = max over free lines e of:
             c + value(P + e)               if e completes c > 0 boxes (I play again)
               - value(P + e)               if e completes nothing (opponent's turn)
```

Every later position is "the current position plus a subset of the free lines", so it can be
numbered by an f-bit integer and its value stored in one byte. Each position is solved once.
The bot then plays a line with the best value (picking at random among equal ones). Because
this search is exact, it automatically finds double-crosses, sacrifices and the right
chain-opening order. We did not have to code each trick by hand.

The memo is kept between the bot's turns: the positions that come later in the same endgame
are already solved, so only the first endgame move takes real time (about 1 second at most).

### 4.3 Valid moves only

The bot only chooses from lines whose bit is 0 in the mask, so it can never return a line
that is already drawn or off the board. `game.c` checks this again before each bot move.
In the tests, 200+ full games were played and every bot move was valid and new.

## 5. Sprint 3: network play and multithreading

### 5.1 Network play

Menu option **3) Network game** asks whether to **host** or **join**:

- The **host** opens a TCP socket on a port (default 5555), waits with `accept()` for one
  opponent, and plays **A** (moves first).
- The **client** types the host's IP address and port, connects with `connect()`, and
  plays **B**.

Both sides exchange a handshake (`HELLO DOTS 1`) so that the program refuses to talk to
something that is not our game. Then every move is sent as one line of text, `MOVE <line>`,
where `<line>` is the 0..48 line number of Sprint 1. Both programs keep their own copy of
the board and apply the moves in the same order, so they always agree. A move that arrives
is checked again (range and "already drawn") before it is drawn, because a program must
never trust data from the network. If a player types `q`, the program sends `QUIT`. If the
connection breaks, the other side notices and goes back to the menu. Sending uses
`MSG_NOSIGNAL`, so a closed connection gives an error instead of killing the program with
`SIGPIPE`.

The same game loop is used as in Sprints 1 and 2. Each player is now one of three kinds:
the human at this keyboard, the bot, or a remote player whose moves come from the network.

### 5.2 Multithreading with a lock

As soon as the connection is ready, a **receiver thread** (`pthread_create`) starts. It
reads the socket line by line and puts each incoming move into a small circular queue. The
main thread runs the game. When it is the opponent's turn it calls `net_wait_move()`, which
takes the next move from that queue.

The queue (and a `closed` flag for "opponent left") is used by both threads, so it is
protected by a `pthread_mutex_t`:

- The receiver locks the mutex, adds the move, signals the condition variable
  `arrived`, and unlocks.
- The main thread locks the mutex and, while the queue is empty, calls
  `pthread_cond_wait`. That call releases the lock while sleeping and takes it back
  when it wakes up. It then removes the move and unlocks.

This means the main thread uses no CPU while waiting (no busy loop), and no move can be lost
or read half-written. At the end of the game `shutdown()` makes the receiver's `recv()`
return, the main thread joins the thread with `pthread_join`, and all memory is freed.

We checked the locking with **Helgrind**: 0 errors. As an experiment we removed the lock
around the queue in the receiver thread. Helgrind then reported over 100 errors, including
"Possible data race during write of size 4" on the queue counter, so the lock really is
needed.

## 6. Complexity analysis

E = 49 lines, B = 20 boxes, f = number of free lines. (Full details: `docs/COMPLEXITY.md`.)

| Operation | Time | Memory |
|-----------|------|--------|
| Convert two dots to a line, check a line, draw a line | O(1) | – |
| Check if a move completes a box | O(1) (≤ 2 boxes, one AND each) | – |
| Check if the game is over | O(1) (`lines == ALL_EDGES`) | – |
| Print the board | O(R·C) | – |
| Bot phase 1: capture or safe line | O(E) | O(E) |
| Bot phase 1: smallest sacrifice | O(E² · B) ≈ 48 000 steps | O(E) |
| **Bot phase 2: exact search** | **O(f · 2^f)** | **O(2^f) bytes** |

| Send / receive / check a network move | O(1) each, 49 messages per game | O(1) (64-entry queue) |

With f ≤ 22: at most 2^22 ≈ 4.2 million positions, about 92 million line checks and 4 MB.
Without the memo, minimax would try every order of the remaining moves, which is O(f!):
22! ≈ 10^21, impossible. The memo works because the result only depends on *which* lines
are drawn, not on the order they were drawn in. The cost doubles with each extra line, which
is why the exact search starts at 22 lines. That is about when the safe lines run out and
the game is decided.

## 7. Testing and results

`make test` runs two test programs:

- `test_board`: line numbering for every line, one box, two boxes with one line, a full board.
- `test_bot`:
  - safe-line and capture detection,
  - the memo search agrees with a slow brute-force search on 30 random positions,
  - in real endgames the bot's move is always optimal and never worse than a greedy move,
  - full games against two simple opponents (results of `./test_bot`):

| Opponent | Games | Bot won | Draw | Bot lost | Average margin |
|----------|-------|---------|------|----------|----------------|
| Random lines | 40 | 40 | 0 | 0 | +19.6 boxes |
| Greedy (takes boxes, avoids 3rd sides) | 40 | 37 | 2 | 1 | +10.8 boxes |

`make netgame` plays a full game between two copies of the program over `127.0.0.1`: both
sides draw all 49 lines and report the same final result. We also tested a player quitting
in the middle (the other side is told) and joining when no host is waiting (a clear error).

In endgame positions where the bot's move differed from the greedy move, the bot gained
on average 10 boxes more by the end of the game. The double-cross is where most of that
gain comes from.

## 8. Debugging with GDB and Valgrind

- **Valgrind:** `make valgrind` runs both test programs, a scripted two-player game
  (with invalid inputs), a full game against the bot and a full network game. Each one reports
  `All heap blocks were freed -- no leaks are possible` and `ERROR SUMMARY: 0 errors`.
  The only heap memory is the bot's memo, freed with `atexit(bot_free)`.
- **Helgrind:** `make helgrind` runs the network game with both players under Helgrind:
  0 errors (see section 5.2).
- **GDB:** we used breakpoints on `board_play`, `input_move`, `bot_choose_move` and
  `exact_move`, printed the board (`print/x b->lines`, `print *b`), and watched the scores
  with `watch board.score[0]`. The session is described in `docs/DEBUGGING.md`.
- Bugs we found and fixed are listed in `docs/DEBUGGING.md`. For example, the first version
  of the endgame search was too slow (3 s per game), and we fixed it with local lookup tables
  and by reusing the memo between turns.

_(Add your team's GDB and Valgrind screenshots here.)_

## 9. Running on Alpine Linux

`setup.sh` installs `build-base`, `gdb` and `valgrind`, builds and tests the game, copies it
to `/usr/local/bin/dots_and_boxes`, and replaces the login prompt on `tty1` in
`/etc/inittab` with `tty1::respawn:/usr/local/bin/dots_and_boxes`. After a reboot the VM
opens directly into the game with no username or password. If the game exits, `init`
restarts it. A normal login is still available on tty2 (Alt+F2) for maintenance.

For a network game between two VMs, both VMs need a network adapter on the same network
(VirtualBox "Bridged Adapter", or the same "Internal Network"). `ip addr` shows each VM's IP
address.

## 10. Conclusion

We built a complete console Dots and Boxes game in modular C, with a two-player mode, a
hard bot, and a network mode with a receiver thread protected by a mutex. The bot plays a fast heuristic early and switches to a perfect search at the end,
where the game is decided. Using a bit mask for the board made both the box-completion check
O(1) and the exact search practical.

**Possible improvements:** use the "long chain rule" to steer the middle game, and start the
exact search earlier with alpha-beta pruning.

## 11. Team contributions

_(Who did what: for example board and input, bot, networking and threads, testing and debugging, VM setup, report and slides.)_
