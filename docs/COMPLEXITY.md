# Complexity of the key operations (Sprint 1)

Notation: the board has R x C boxes (here 4 x 5), so it has (R+1)(C+1) = 30 dots and
E = (R+1)C + R(C+1) = 49 lines.

## Board representation

- `lines`: one 64-bit integer in which bit *e* is 1 if line *e* is drawn (49 bits used).
- `owner[20]`: the letter in each box.
- Two lookup tables built once at start-up:
  - `g_box_mask[box]`: the 4 lines around each box, as a bit mask.
  - `g_edge_box[line][2]`: the (at most 2) boxes touching each line.

Building the tables takes O(R·C) time once. Memory is O(R·C + E), which for this board
is a few hundred bytes.

## Operations

| Operation | Function | Time | Why |
|-----------|----------|------|-----|
| Convert two dots to a line | `board_edge_from_dots` | O(1) | Bounds check, then an arithmetic formula: `r*5+c` or `25+r*6+c` |
| Check if a line is drawn | `board_has_edge` | O(1) | One shift and AND on the bit mask |
| Draw a line | `board_play` | O(1) | Set one bit |
| **Check if a move completes a box** | `board_play` | **O(1)** | Only the ≤ 2 boxes touching the new line can change. For each, `(lines & box_mask) == box_mask` checks all 4 sides with a single AND and compare |
| Check if the game is over | `board_is_full` | O(1) | `lines == ALL_EDGES`, one comparison |
| Print the board | `board_print` | O(R·C) | Every dot, line and box is printed once |
| Whole game | `game_play_two_players` | O(E·R·C) | E moves, and the board is reprinted after each |

## Compared with a naive approach

A simple version stores the lines in 2-D arrays and, after every move, scans **all** boxes
to see which ones are now complete. That costs O(R·C) per move, so O(E·R·C) for the game,
just for the checks. Our version checks only the two boxes next to the new line, so each
check is O(1) and all the checks in a game cost O(E).
On a 4 x 5 board both are instant. The difference grows with the board size, and the bit mask
representation will also make the Sprint 2 bot faster, since it can copy and compare whole
positions in O(1).
