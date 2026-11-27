# Complexity analysis (Sprints 1, 2 and 3)

## Part 1: Board operations (Sprint 1)

Notation: the board has R x C boxes (here 4 x 5), so it has (R+1)(C+1) = 30 dots and
E = (R+1)C + R(C+1) = 49 lines.

### Board representation

- `lines`: one 64-bit integer in which bit *e* is 1 if line *e* is drawn (49 bits used).
- `owner[20]`: the letter in each box.
- Two lookup tables built once at start-up:
  - `g_box_mask[box]`: the 4 lines around each box, as a bit mask.
  - `g_edge_box[line][2]`: the (at most 2) boxes touching each line.

Building the tables takes O(R·C) time once. Memory is O(R·C + E), which for this board
is a few hundred bytes.

### Operations

| Operation | Function | Time | Why |
|-----------|----------|------|-----|
| Convert two dots to a line | `board_edge_from_dots` | O(1) | Bounds check, then an arithmetic formula: `r*5+c` or `25+r*6+c` |
| Check if a line is drawn | `board_has_edge` | O(1) | One shift and AND on the bit mask |
| Draw a line | `board_play` | O(1) | Set one bit |
| **Check if a move completes a box** | `board_play` | **O(1)** | Only the ≤ 2 boxes touching the new line can change. For each, `(lines & box_mask) == box_mask` checks all 4 sides with a single AND and compare |
| Check if the game is over | `board_is_full` | O(1) | `lines == ALL_EDGES`, one comparison |
| Print the board | `board_print` | O(R·C) | Every dot, line and box is printed once |
| Whole game | `game_play_two_players` | O(E·R·C) | E moves, and the board is reprinted after each |

### Compared with a naive approach

A simple version stores the lines in 2-D arrays and, after every move, scans **all** boxes
to see which ones are now complete. That costs O(R·C) per move, so O(E·R·C) for the game,
just for the checks. Our version checks only the two boxes next to the new line, so each
check is O(1) and all the checks in a game cost O(E).
On a 4 x 5 board both are instant. The difference grows with the board size, and the bit mask
representation will also make the Sprint 2 bot faster, since it can copy and compare whole
positions in O(1).

## Part 2: The hard bot (Sprint 2)

Notation: E = 49 lines, B = 20 boxes, f = number of lines not drawn yet.

### The strategy in short

Dots and Boxes is decided at the end of the game. Early on, the lines are "safe": drawing
them gives nothing away. Once every safe line is gone, each move must hand boxes to the
opponent, usually a whole **chain** (a corridor of boxes that can be taken one after another).
Whoever controls the order in which chains are opened wins. The trick for keeping control is
the **double-cross**: when you take a chain, you leave its last 2 boxes on purpose, so your
opponent takes those 2 but then has to open the next (bigger) chain for you.

The bot plays in two phases (`src/bot.c`, `bot_choose_move`):

| Phase | When | What the bot does |
|-------|------|-------------------|
| 1. Heuristic | more than 22 lines left | a) take any box it can complete (a line that closes 2 boxes first); b) otherwise draw a **safe** line, one that does not draw the 3rd side of any box, preferring lines next to boxes with fewer sides; c) if no safe line exists, **sacrifice** the line that lets the opponent take the fewest boxes |
| 2. Exact search | 22 lines or fewer left | negamax search with a memo table over every reachable position. It returns the move with the best possible final score, so it plays the chains perfectly, including the double-cross and the "long chain" parity fight |

The bot can only return a line that is not drawn yet. It picks from the free lines and
never from anywhere else, so it cannot make an invalid or repeated move. `game.c` also
checks this before each bot move. Ties between equally good moves are broken at random, so
the bot does not play the same game every time.

### The exact search

Value of a position = (boxes the player to move will still win) − (boxes the opponent will
still win), if both play perfectly:

```
value(P) = 0                                  if no line is left
value(P) = max over free lines e of:
             c + value(P + e)                 if e completes c > 0 boxes (same player again)
               − value(P + e)                 if e completes nothing (turn passes)
```

This is minimax written as **negamax**: my best score is the opposite of my opponent's best
score. With f free lines, any later position is "the root + some subset of those f lines",
so it can be numbered by an f-bit integer `idx`, and `memo[idx]` stores its value (one byte,
−20..20). Each position is solved **once**.

### Complexity of the bot

| Part | Time | Memory | Why |
|------|------|--------|-----|
| Does line e complete a box? (`boxes_closed`) | O(1) | – | checks the ≤ 2 boxes next to e with one AND each, using precomputed masks |
| Is line e safe? (`bot_is_safe`) | O(1) | – | popcount of the ≤ 2 neighbouring boxes |
| Phase 1a/1b: capture or safe line | O(E) | O(E) | one pass over the 49 lines |
| Phase 1c: smallest sacrifice | O(E² · B) | O(E) | for each of the ≤ E lines, simulate the opponent grabbing boxes: ≤ B + 1 passes of O(E). About 48 000 steps |
| Phase 2: exact search, first call | **O(f · 2^f)** | **O(2^f)** bytes | ≤ 2^f positions, each tries ≤ f lines once |
| Phase 2: later moves in the same endgame | O(f) to O(f · 2^f) | reuses the memo | the bot keeps its memo between its turns, so most later positions are already solved |

With f ≤ 22 that is at most 2^22 ≈ 4.2 million positions, about 92 million line checks and
4 MB of memory. Measured on our test machine (built with `-O0` for GDB): the slowest single
bot move was about 1.5 s, and all other moves were instant.

**Why memoization matters.** Plain minimax without the memo explores every order of the
remaining moves, which is O(f!) : 22! ≈ 1.1 × 10^21 sequences, impossible. Many different
orders reach the same set of drawn lines, and the result depends only on that set, so the
memo turns O(f!) into O(f · 2^f).

**Why the limit is 22 lines.** The cost doubles with every extra line: 24 lines would need
16 MB and about 4 times longer, and the whole board (49 lines) would need 2^49 bytes
(512 TB). At 22 lines the search starts at about the time the safe lines run out, which is
exactly when the game is decided.

**Whole game.** Phase 1 moves cost O(E) or O(E² · B), which is constant for a fixed board.
So a whole game costs O(f · 2^f) with f = 22, dominated by one exact search.

### How we checked it is correct (`tests/test_bot.c`)

- The memo search gives the same value as a plain brute-force search (no memo) on 30
  random positions with 9 free lines.
- In real endgame positions, the bot's move always has the best possible value, and is
  never worse than a greedy "take everything" move.
- In hundreds of full games, every bot move was an undrawn line (valid, no repeats),
  all 49 lines got drawn and the 20 boxes were all awarded.

## Part 3: Network play (Sprint 3)

| Operation | Time | Why |
|-----------|------|-----|
| Send a move (`net_send_move`) | O(1) | one short line, `MOVE <0..48>`, at most 9 bytes |
| Receive a move (receiver thread) | O(1) per move | reads one line, then a constant-time push into a circular queue under the mutex |
| Take a move from the queue (`net_wait_move`) | O(1) + waiting | pop from the circular queue; the thread sleeps on the condition variable (no busy waiting, so no CPU is used while waiting) |
| Check an incoming move | O(1) | the same bit test `board_has_edge` as for a typed move |
| Whole network game | O(E) messages | exactly one message per line drawn, 49 in total, plus the handshake |

The queue holds at most 64 moves (more than the 49 lines of a game), so its memory is O(1).
