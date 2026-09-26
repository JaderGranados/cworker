#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "job/job.h"
#include "queue/jobqueue.h"
#include "worker/worker.h"
#include "mappers/argmapper.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <number_of_jobs>\n", argv[0]);
        return 1;
    }

    char *type = argv[1];
    MappedValue *payload = from_arg_to_value(argv[2], argv[1]);
    if (payload == NULL) {
        fprintf(stderr, "Failed to map argument '%s' for type '%s'\n", argv[2], type);
        return 1;
    }

    JobQueue *queue = job_queue_create(
        3
    );
    WorkerPool *pool = worker_pool_create(queue, 2);
    worker_pool_start(pool);
    if (job_queue_push(queue, create_job(1, type, payload->value, payload->size)) != 0) {
        printf("Failed to push job to queue\n");
        return 1;
    }

    sleep(3); // Give some time for the worker to process the job
    worker_pool_shutdown(pool);
    worker_pool_destroy(pool);
    job_queue_destroy(queue);
    free(payload->value);
    free(payload);
    return 0;
}