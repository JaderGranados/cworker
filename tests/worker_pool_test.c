#define PRODUCER_COUNT 4
#define JOBS_PER_PRODUCER 100
#define WORKER_COUNT 4
#define QUEUE_CAPACITY 10

#include <stdio.h>
#include <stdlib.h>
#include "job/job.h"
#include "queue/jobqueue.h"
#include "worker/worker.h"

typedef struct
{
    JobQueue *queue;
} ProducerArgs;

int main() {
    JobQueue *queue = job_queue_create(QUEUE_CAPACITY);
    if (queue == NULL) {
        printf("Failed to create job queue\n");
        return 1;
    }

    WorkerPool *pool = worker_pool_create(queue, WORKER_COUNT);
    if (pool == NULL) {
        printf("Failed to create worker pool\n");
        job_queue_destroy(queue);
        return 1;
    }

    worker_pool_start(pool);

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        for (int j = 0; j < JOBS_PER_PRODUCER; j++) {
            int job_id = i * JOBS_PER_PRODUCER + j + 1;
            Job *job = create_job(job_id, "test", &job_id, sizeof(job_id));
            if (job == NULL) {
                printf("Failed to create job %d\n", job_id);
                continue;
            }
            if (job_queue_push(queue, job) != 0) {
                printf("Failed to push job %d\n", job_id);
                destroy_job(job);
            }
        }
    }
}