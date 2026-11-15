# Dots and Boxes (CMPS 270, Fall 2026/27)

A console version of Dots and Boxes written in C for Alpine Linux.

**Sprint 1 (current):** two players (A and B) on a 4 x 5 grid of boxes.

## Build and run

Requires `gcc` and `make` (on Alpine: `apk add build-base`).

```sh
make            # builds ./dots_and_boxes
make run        # builds and runs the game
make test       # runs the automated board tests
make valgrind   # memory check (see docs/DEBUGGING.md)
make clean
```

## How to play

Choose **1** in the menu. Dots are numbered by row (0 to 4) and column (0 to 5).
On your turn, type the two dots you want to join as `row1 col1 row2 col2`:

```
  0 1 2 3 4 5
0 .-.-. . . .
  |A|
1 .-. . . . .
```

- `0 2 0 3` draws the line from dot (0,2) to dot (0,3).
- `1 0 2 0` draws the line from dot (1,0) down to dot (2,0).

If your line completes a box, your letter goes inside it and you play again. When all
20 boxes are claimed, the player with more boxes wins. Type `q` to stop a game.

Invalid moves are rejected with a message, and the same player is asked again. These are:
text instead of numbers, dots off the board, dots that are not next to each other
(including diagonals), and lines that are already drawn.

## Running on Alpine Linux in a VM (boots straight into the game)

1. Create a VM (VirtualBox, VMware or QEMU) and boot the Alpine **Standard** or **Virtual** ISO.
2. Log in as `root` and run `setup-alpine`. Install to disk with the `sys` disk mode,
   otherwise changes are lost on reboot. Say yes to enabling the community repository.
   Then reboot, and remove the ISO.
3. Get the project and run the setup script:
   ```sh
   apk add git
   git clone https://github.com/<your-username>/<your-repo>.git
   cd <your-repo>
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
include/game.h    game loop
src/board.c       lines, boxes, box-completion check, printing
src/input.c       reading moves and menu choices (handles bad input and EOF)
src/game.c        turns, extra turn after a box, winner
src/main.c        menu
tests/            unit tests and a scripted full game (used for Valgrind)
docs/             complexity analysis and GDB/Valgrind guide
setup.sh          Alpine install + boot-to-game
Makefile
```
