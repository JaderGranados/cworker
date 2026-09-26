#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "worker.h"
#include "job/job.h"
#include "queue/jobqueue.h"
#include "handler/handler.h"
#include "handler/handlerresult.h"

struct WorkerPool
{
    JobQueue *queue;
    pthread_t *threads;
    size_t worker_count;
};

static void *worker_thread(void *arg)
{
    WorkerPool *pool = arg;

    while (1)
    {
        Job *job = job_queue_pop(pool->queue);

        if (job == NULL) {
            break;
        }

        HandlerResult *result = handler_dispatch(
            get_job_type(job),
            get_job_payload(job),
            get_job_payload_size(job)
        );
        destroy_job(job);
        handler_result_destroy(result);
    }

    return NULL;
}

WorkerPool *worker_pool_create(JobQueue *queue, size_t worker_count)
{
    if (queue == NULL || worker_count == 0)
    {
        return NULL;
    }

    WorkerPool *pool = malloc(sizeof(*pool));
    if (pool == NULL)
    {
        return NULL;
    }

    pool->queue = queue;
    pool->worker_count = worker_count;

    pool->threads = malloc(sizeof(*pool->threads) * worker_count);
    if (pool->threads == NULL)
    {
        free(pool);
        return NULL;
    }

    return pool;
}

void worker_pool_start(WorkerPool *pool)
{
    for (size_t i = 0; i < pool->worker_count; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0)
        {
            for (size_t j = 0; j < i; j++)
            {
                pthread_cancel(pool->threads[j]);
            }
            for (size_t j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }
            free(pool->threads);
            free(pool);
            return;
        }
    }
}

void worker_pool_shutdown(WorkerPool *pool)
{
    if (pool == NULL)
    {
        return;
    }

    job_queue_shutdown(pool->queue);

    for (size_t i = 0; i < pool->worker_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }
}

void worker_pool_destroy(WorkerPool *pool)
{
    if (pool == NULL)
    {
        return;
    }

    free(pool->threads);
    free(pool);
}