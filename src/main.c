#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "job/job.h"
#include "queue/jobqueue.h"
#include "worker/worker.h"
#include "mappers/argmapper.h"
#include "result/result_store.h"

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
    size_t capacity = 3;

    JobQueue *queue = job_queue_create(
        capacity
    );
    ResultStore *result_store = result_store_create(
        capacity
    );
    WorkerPool *pool = worker_pool_create(queue, 2, result_store);
    int job_id = 1;
    worker_pool_start(pool);
    if (job_queue_push(queue, create_job(job_id, type, payload->value, payload->size)) != 0) {
        printf("Failed to push job to queue\n");
        return 1;
    }

    sleep(3); // Give some time for the worker to process the job
    worker_pool_shutdown(pool);
    HandlerResult *result = result_store_get(result_store, job_id);
    printf("Job %d result: %llu\n", job_id, *((uint64_t *)handler_result_get_result(result)));
    handler_result_destroy(result);
    worker_pool_destroy(pool);
    job_queue_destroy(queue);
    result_store_destroy(result_store);
    free(payload->value);
    free(payload);
    return 0;
}