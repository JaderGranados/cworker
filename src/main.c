#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "queue/jobqueue.h"
#include "worker/worker.h"
#include "result/result_store.h"
#include "server/tcpserver.h"

static volatile sig_atomic_t running = 1;

static void handle_sigint(int signum) {
    (void)signum;
    running = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);

    int port = 8080;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    size_t capacity = 16;

    JobQueue *queue = job_queue_create(capacity);
    ResultStore *result_store = result_store_create(capacity);
    WorkerPool *pool = worker_pool_create(queue, 2, result_store);
    TcpServer *server = tcp_server_create(port, queue, result_store);

    if (queue == NULL || result_store == NULL || pool == NULL || server == NULL) {
        fprintf(stderr, "Failed to initialize server\n");
        tcp_server_destroy(server);
        worker_pool_destroy(pool);
        job_queue_destroy(queue);
        result_store_destroy(result_store);
        return 1;
    }

    worker_pool_start(pool);

    if (tcp_server_start(server) != 0) {
        fprintf(stderr, "Failed to start TCP server on port %d\n", port);
        worker_pool_shutdown(pool);
        worker_pool_destroy(pool);
        tcp_server_destroy(server);
        job_queue_destroy(queue);
        result_store_destroy(result_store);
        return 1;
    }

    printf("cworker listening on port %d (Ctrl+C to stop)\n", port);
    while (running) {
        sleep(1);
    }

    tcp_server_shutdown(server);
    worker_pool_shutdown(pool);
    tcp_server_destroy(server);
    worker_pool_destroy(pool);
    job_queue_destroy(queue);
    result_store_destroy(result_store);

    return 0;
}
