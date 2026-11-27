# Dots and Boxes (CMPS 241, Fall 2026/27)

A console version of Dots and Boxes written in C for Alpine Linux.

- **Sprint 1:** two players (A and B) on a 4 x 5 grid of boxes.
- **Sprint 2:** play against a hard-level bot (menu option 2). The bot takes free
  boxes, avoids giving boxes away, and solves the endgame exactly (see `docs/COMPLEXITY.md`).
- **Sprint 3 (bonus):** network game between two machines over TCP (menu option 3). A
  receiver thread, protected by a mutex and a condition variable, handles incoming moves.

## Build and run

Requires `gcc`, `make` and POSIX threads (on Alpine: `apk add build-base`).

```sh
make            # builds ./dots_and_boxes
make run        # builds and runs the game
make test       # runs the automated board and bot tests
make valgrind   # memory check (see docs/DEBUGGING.md)
make netgame    # full scripted network game between two processes on 127.0.0.1
make helgrind   # the same network game under Helgrind (checks the locking)
make clean
```

## How to play

Choose **1** in the menu for two players, or **2** to play against the bot (you then choose
whether you or the bot moves first). Dots are numbered by row (0 to 4) and column (0 to 5).
On your turn, type the two dots you want to join as `row1 col1 row2 col2`:

```
  0 1 2 3 4 5
0 .-.-. . . .
  |A|
1 .-. . . . .
```

- `0 2 0 3` draws the line from dot (0,2) to dot (0,3).
- `1 0 2 0` draws the line from dot (1,0) down to dot (2,0).

If your line completes a box, your letter goes inside it and you play again. The scores are
shown after every move. When all
20 boxes are claimed, the player with more boxes wins. Type `q` to stop a game.

Against the bot, its move is printed (for example `Player B (bot) plays: 0 1 0 2`) and the
board is shown again after it. Near the end of the game the bot may think for a second the first
time it starts its exact search.

Invalid moves are rejected with a message, and the same player is asked again. These are:
text instead of numbers, dots off the board, dots that are not next to each other
(including diagonals), and lines that are already drawn.

## Network game (Sprint 3)

1. Both machines (or VMs) must be on the same network. For two VirtualBox VMs, set the
   network adapter of both to **Bridged Adapter** or the same **Internal Network**/**Host-only**
   network. Find a machine's IP with `ip addr` (look at `eth0`).
2. On the first machine choose **3**, then **1) Host**, and keep port 5555 (or type another).
   It waits for the other player. The host is **Player A** and moves first.
3. On the second machine choose **3**, then **2) Join**, type the host's IP and the same port.
   It plays **Player B**.

Both programs keep their own board and send each move as a line of text (`MOVE 17`). Every
move that arrives is checked again before it is drawn. If a player types `q` or the
connection drops, the other side is told and returns to the menu. To try it on one machine,
run two copies in two terminals and join `127.0.0.1`.

## Running on Alpine Linux in a VM (boots straight into the game)

1. Create a VM (VirtualBox, VMware or QEMU) and boot the Alpine **Standard** or **Virtual** ISO.
2. Log in as `root` and run `setup-alpine`. Install to disk with the `sys` disk mode,
   otherwise changes are lost on reboot. Say yes to enabling the community repository.
   Then reboot, and remove the ISO.
3. Get the project and run the setup script:
   ```sh
   apk add git
   git clone https://github.com/USERNAME/DotsAndBoxes.git   # your team's repository
   cd DotsAndBoxes
   sh setup.sh
   reboot
   ```

`setup.sh` installs the compiler, GDB and Valgrind. It builds and tests the game, then copies
it to `/usr/local/bin/dots_and_boxes`. Finally, it replaces the login prompt on `tty1` in
`/etc/inittab` with:

```
tty1::respawn:/usr/local/bin/dots_and_boxes
```

After the reboot the VM opens directly into the game, with no username or password. If the
game exits, `init` restarts it. For maintenance, press **Alt+F2** to get a normal login on
tty2. `sh setup.sh uninstall` puts the login prompt back.

## Project structure

```
include/board.h   board data structure and operations
include/input.h   safe console input
include/game.h    game loops (two players, or human vs bot)
include/bot.h     hard-level bot
include/net.h     network play (Sprint 3)
src/board.c       lines, boxes, box-completion check, printing
src/input.c       reading moves and menu choices (handles bad input and EOF)
src/game.c        turns, extra turn after a box, bot turns, winner
src/bot.c         bot strategy: captures, safe lines, sacrifices, exact endgame search
src/net.c         TCP host/join, message protocol, receiver thread + mutex
src/main.c        menu
tests/            unit tests (board, bot), scripted games, net_game.sh (network test)
docs/             complexity analysis, GDB/Valgrind guide, report
setup.sh          Alpine install + boot-to-game
Makefile
```
