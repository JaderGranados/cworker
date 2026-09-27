#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <poll.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tcpserver.h"
#include "job/job.h"
#include "handler/handlerresult.h"
#include "mappers/argmapper.h"

struct TcpServer {
    int port;
    int listen_fd;
    volatile int running;

    JobQueue *queue;
    ResultStore *result_store;

    pthread_t accept_thread;

    int next_job_id;
    pthread_mutex_t job_id_mutex;
};

typedef struct ConnectionArgs {
    int client_fd;
    TcpServer *server;
} ConnectionArgs;

static ssize_t read_line(int fd, char *buf, size_t buf_size) {
    size_t len = 0;
    while (len + 1 < buf_size) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0 || c == '\n') {
            break;
        }
        buf[len++] = c;
    }
    buf[len] = '\0';
    return (ssize_t)len;
}

static void handle_submit(TcpServer *server, char *args, char *response, size_t response_size) {
    char type[64];
    char value[64];

    if (sscanf(args, "%63s %63s", type, value) != 2) {
        snprintf(response, response_size, "ERR invalid SUBMIT\n");
        return;
    }

    MappedValue *payload = from_arg_to_value(value, type);
    if (payload == NULL) {
        snprintf(response, response_size, "ERR invalid argument\n");
        return;
    }

    pthread_mutex_lock(&server->job_id_mutex);
    int job_id = server->next_job_id++;
    pthread_mutex_unlock(&server->job_id_mutex);

    if (result_store_submit(server->result_store, job_id) != 0) {
        free(payload->value);
        free(payload);
        snprintf(response, response_size, "ERR could not reserve job id\n");
        return;
    }

    Job *job = create_job(job_id, type, payload->value, payload->size);
    free(payload->value);
    free(payload);

    if (job == NULL) {
        snprintf(response, response_size, "ERR could not create job\n");
        return;
    }

    if (job_queue_push(server->queue, job) != 0) {
        destroy_job(job);
        snprintf(response, response_size, "ERR could not enqueue job\n");
        return;
    }

    snprintf(response, response_size, "OK %d\n", job_id);
}

static void handle_get(TcpServer *server, char *args, char *response, size_t response_size) {
    int job_id;
    if (sscanf(args, "%d", &job_id) != 1) {
        snprintf(response, response_size, "ERR invalid GET\n");
        return;
    }

    ResultLookup lookup = result_store_get(server->result_store, job_id);

    switch (lookup.status) {
        case RESULT_NOT_FOUND:
            snprintf(response, response_size, "NOT_FOUND\n");
            break;
        case RESULT_PENDING:
            snprintf(response, response_size, "PENDING\n");
            break;
        case RESULT_READY: {
            uint64_t value = *(uint64_t *)handler_result_get_result(lookup.result);
            snprintf(response, response_size, "RESULT %d %" PRIu64 "\n", job_id, value);
            break;
        }
    }
}

static void *connection_thread(void *arg) {
    ConnectionArgs *conn = arg;
    TcpServer *server = conn->server;
    int fd = conn->client_fd;
    free(conn);

    char line[256];
    read_line(fd, line, sizeof(line));

    char response[128];
    if (strncmp(line, "SUBMIT ", 7) == 0) {
        handle_submit(server, line + 7, response, sizeof(response));
    } else if (strncmp(line, "GET ", 4) == 0) {
        handle_get(server, line + 4, response, sizeof(response));
    } else {
        snprintf(response, sizeof(response), "ERR unknown command\n");
    }

    write(fd, response, strlen(response));
    close(fd);
    return NULL;
}

static void *accept_loop(void *arg) {
    TcpServer *server = arg;

    while (server->running) {
        struct pollfd pfd = { .fd = server->listen_fd, .events = POLLIN };
        int n = poll(&pfd, 1, 500);
        if (n <= 0) {
            continue;
        }

        int client_fd = accept(server->listen_fd, NULL, NULL);
        if (client_fd < 0) {
            continue;
        }

        ConnectionArgs *conn = malloc(sizeof(*conn));
        if (conn == NULL) {
            close(client_fd);
            continue;
        }
        conn->client_fd = client_fd;
        conn->server = server;

        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&thread, &attr, connection_thread, conn) != 0) {
            free(conn);
            close(client_fd);
        }
        pthread_attr_destroy(&attr);
    }

    close(server->listen_fd);
    return NULL;
}

TcpServer *tcp_server_create(int port, JobQueue *queue, ResultStore *result_store) {
    if (queue == NULL || result_store == NULL) {
        return NULL;
    }

    TcpServer *server = malloc(sizeof(*server));
    if (server == NULL) {
        return NULL;
    }

    server->port = port;
    server->listen_fd = -1;
    server->running = 0;
    server->queue = queue;
    server->result_store = result_store;
    server->next_job_id = 1;

    if (pthread_mutex_init(&server->job_id_mutex, NULL) != 0) {
        free(server);
        return NULL;
    }

    return server;
}

int tcp_server_start(TcpServer *server) {
    if (server == NULL) {
        return -1;
    }

    /* A client that disconnects before we write its response would otherwise
     * raise SIGPIPE and kill the whole process — sockets need this ignored. */
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return -1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)server->port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(listen_fd);
        return -1;
    }

    if (listen(listen_fd, 16) != 0) {
        close(listen_fd);
        return -1;
    }

    server->listen_fd = listen_fd;
    server->running = 1;

    if (pthread_create(&server->accept_thread, NULL, accept_loop, server) != 0) {
        close(listen_fd);
        server->listen_fd = -1;
        server->running = 0;
        return -1;
    }

    return 0;
}

void tcp_server_shutdown(TcpServer *server) {
    if (server == NULL || server->running == 0) {
        return;
    }
    server->running = 0;
    pthread_join(server->accept_thread, NULL);
}

void tcp_server_destroy(TcpServer *server) {
    if (server == NULL) {
        return;
    }
    pthread_mutex_destroy(&server->job_id_mutex);
    free(server);
}
