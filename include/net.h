/*
 * net.h - Network play (Sprint 3): two players on two machines over TCP.
 *
 * One player HOSTS (opens a port and waits) and plays A; the other JOINS (connects to
 * the host's IP address) and plays B. Both programs keep their own copy of the board
 * and send each move as one line of text:
 *
 *     HELLO DOTS 1      handshake, sent by both sides when they connect
 *     MOVE <line>       a move, <line> = 0..48
 *     QUIT              the player left the game
 *
 * Multithreading: after connecting, a RECEIVER THREAD reads the socket and puts the
 * opponent's moves into a queue. The game (main thread) takes moves out of that queue.
 * The queue is shared by the two threads, so it is protected by a pthread mutex, and a
 * condition variable wakes the main thread when a move arrives.
 */
#ifndef NET_H
#define NET_H

#define NET_DEFAULT_PORT 5555

typedef struct NetGame NetGame;   /* opaque: defined in net.c */

/* Waits for one opponent on `port`. Returns NULL on error (message printed). */
NetGame *net_host(int port);

/* Connects to a host. Returns NULL on error (message printed). */
NetGame *net_join(const char *host, int port);

/* Sends our move to the opponent. Returns 0 on success, -1 if the connection is gone. */
int net_send_move(NetGame *g, int edge);

/* Tells the opponent we are leaving. */
void net_send_quit(NetGame *g);

/* Blocks until the opponent's next move arrives (from the receiver thread).
 * Returns the line 0..48, or -1 if the opponent quit or the connection was lost. */
int net_wait_move(NetGame *g);

/* Stops the receiver thread, closes the socket and frees everything. */
void net_close(NetGame *g);

#endif /* NET_H */
