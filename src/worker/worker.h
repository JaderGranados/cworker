#ifndef WORKER_H
#define WORKER_H

#include "queue/jobqueue.h"
#include "result/result_store.h"

typedef struct WorkerPool WorkerPool;

WorkerPool *worker_pool_create(JobQueue *queue, size_t worker_count, ResultStore *result_store);
void worker_pool_start(WorkerPool *pool);
void worker_pool_shutdown(WorkerPool *pool);
void worker_pool_destroy(WorkerPool *pool);

#endif