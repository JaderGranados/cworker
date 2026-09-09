#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

#include "job/job.h"
#include "queue/jobqueue.h"
#include "worker/worker.h"
#include "mappers/argmapper.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_of_jobs>\n", argv[0]);
        return 1;
    }

    char *type = argv[1];
    void *payload = from_arg_to_value(argv[2], argv[1]);
    size_t payload_size = sizeof(payload);

    JobQueue *queue = job_queue_create(
        3
    );
    WorkerPool *pool = worker_pool_create(queue, 2);
    worker_pool_start(pool);
    if (job_queue_push(queue, create_job(1, type, payload, payload_size)) != 0) {
        printf("Failed to push job to queue\n");
        return 1;
    }

    sleep(1); // Give some time for the worker to process the job
    return 0;
}