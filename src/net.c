/*
 * net.c - TCP connection, line protocol and the receiver thread for network play.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "board.h"
#include "net.h"

#define HELLO      "HELLO DOTS 1"
#define QUEUE_SIZE 64                 /* more than the 49 moves of a whole game */

struct NetGame {
    int fd;                           /* connected socket */
    pthread_t receiver;
    int receiver_started;

    /* ---- shared between the main thread and the receiver thread ---- */
    pthread_mutex_t lock;             /* protects everything below          */
    pthread_cond_t  arrived;          /* signalled when a move or EOF comes */
    int queue[QUEUE_SIZE];            /* opponent's moves, in order         */
    int head, count;
    int closed;                       /* 1 = opponent quit / connection lost */
};

/* ---------- small socket helpers ---------- */

static int send_line(int fd, const char *text)
{
    char buf[64];
    int len = snprintf(buf, sizeof buf, "%s\n", text);
    int sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, (size_t)(len - sent), MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        sent += (int)n;
    }
    return 0;
}

/* Reads one line (without '\n'), one byte at a time. Returns 0, or -1 on EOF/error. */
static int recv_line(int fd, char *buf, size_t size)
{
    size_t len = 0;
    for (;;) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        if (c == '\n')
            break;
        if (c != '\r' && len + 1 < size)
            buf[len++] = c;
    }
    buf[len] = '\0';
    return 0;
}

/* ---------- receiver thread ---------- */

static void mark_closed(NetGame *g)
{
    pthread_mutex_lock(&g->lock);
    g->closed = 1;
    pthread_cond_signal(&g->arrived);
    pthread_mutex_unlock(&g->lock);
}

static void *receiver_main(void *arg)
{
    NetGame *g = arg;
    char line[64];

    while (recv_line(g->fd, line, sizeof line) == 0) {
        int edge;
        char extra;
        if (sscanf(line, "MOVE %d %c", &edge, &extra) == 1 && edge >= 0 && edge < NUM_EDGES) {
            pthread_mutex_lock(&g->lock);
            if (g->count < QUEUE_SIZE) {
                g->queue[(g->head + g->count) % QUEUE_SIZE] = edge;
                g->count++;
            }
            pthread_cond_signal(&g->arrived);      /* wake the main thread */
            pthread_mutex_unlock(&g->lock);
        } else if (strcmp(line, "QUIT") == 0) {
            break;
        } else {
            fprintf(stderr, "\nNetwork: unexpected message \"%s\", closing.\n", line);
            break;
        }
    }
    mark_closed(g);                                 /* EOF, QUIT or bad message */
    return NULL;
}

/* ---------- setup ---------- */

static NetGame *start(int fd)
{
    char line[64];

    /* Handshake: both sides say hello, so we know the other program is our game. */
    if (send_line(fd, HELLO) < 0 || recv_line(fd, line, sizeof line) < 0 ||
        strcmp(line, HELLO) != 0) {
        printf("The other side is not a Dots and Boxes game (bad handshake).\n");
        close(fd);
        return NULL;
    }

    NetGame *g = calloc(1, sizeof *g);
    if (g == NULL) {
        close(fd);
        return NULL;
    }
    g->fd = fd;
    pthread_mutex_init(&g->lock, NULL);
    pthread_cond_init(&g->arrived, NULL);

    if (pthread_create(&g->receiver, NULL, receiver_main, g) != 0) {
        printf("Could not start the receiver thread.\n");
        net_close(g);
        return NULL;
    }
    g->receiver_started = 1;
    return g;
}

NetGame *net_host(int port)
{
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0) {
        perror("socket");
        return NULL;
    }
    int yes = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);      /* accept on every network card */
    addr.sin_port = htons((uint16_t)port);

    if (bind(ls, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(ls, 1) < 0) {
        perror("bind/listen");
        close(ls);
        return NULL;
    }
    printf("Waiting for the other player to join on port %d...\n", port);

    struct sockaddr_in peer;
    socklen_t plen = sizeof peer;
    int fd = accept(ls, (struct sockaddr *)&peer, &plen);
    close(ls);                                      /* only one opponent */
    if (fd < 0) {
        perror("accept");
        return NULL;
    }
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
    printf("Player B connected from %s.\n", ip);
    return start(fd);
}

NetGame *net_join(const char *host, int port)
{
    char port_text[16];
    snprintf(port_text, sizeof port_text, "%d", port);

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo(host, port_text, &hints, &res);
    if (err != 0) {
        printf("Unknown host \"%s\": %s\n", host, gai_strerror(err));
        return NULL;
    }

    int fd = -1;
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) {
        printf("Could not connect to %s:%d (is the host waiting?).\n", host, port);
        return NULL;
    }
    printf("Connected to %s:%d.\n", host, port);
    return start(fd);
}

/* ---------- during the game ---------- */

int net_send_move(NetGame *g, int edge)
{
    char text[32];
    snprintf(text, sizeof text, "MOVE %d", edge);
    return send_line(g->fd, text);
}

void net_send_quit(NetGame *g)
{
    send_line(g->fd, "QUIT");
}

int net_wait_move(NetGame *g)
{
    int edge = -1;

    pthread_mutex_lock(&g->lock);
    while (g->count == 0 && !g->closed)
        pthread_cond_wait(&g->arrived, &g->lock);  /* sleeps, releasing the lock */
    if (g->count > 0) {
        edge = g->queue[g->head];
        g->head = (g->head + 1) % QUEUE_SIZE;
        g->count--;
    }
    pthread_mutex_unlock(&g->lock);
    return edge;
}

void net_close(NetGame *g)
{
    if (g == NULL)
        return;
    shutdown(g->fd, SHUT_RDWR);                     /* makes the receiver's recv() return */
    if (g->receiver_started)
        pthread_join(g->receiver, NULL);
    close(g->fd);
    pthread_cond_destroy(&g->arrived);
    pthread_mutex_destroy(&g->lock);
    free(g);
}
