#ifndef JOBQUEUE_H
#define JOBQUEUE_H
#include "job/job.h"

typedef struct JobQueue JobQueue;

JobQueue *job_queue_create(size_t capacity);
void job_queue_destroy(JobQueue *queue);

int job_queue_push(JobQueue *queue, Job *job);
Job *job_queue_pop(JobQueue *queue);

size_t job_queue_size(JobQueue *queue);
size_t job_queue_capacity(const JobQueue *queue);

void job_queue_shutdown(JobQueue *queue);

#endif // JOBQUEUE_H