#include "job/job.h"
#include "jobqueue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

struct JobQueue
{
    Job **jobs;

    size_t capacity;
    size_t count;

    size_t head;
    size_t tail;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    bool shutdown;
};

JobQueue *job_queue_create(size_t capacity)
{
    JobQueue *queue = malloc(sizeof(*queue));
    if (queue == NULL)
    {
        return NULL;
    }
    queue->jobs = malloc(sizeof(*queue->jobs) * capacity);
    if (queue->jobs == NULL)
    {
        free(queue);
        return NULL;
    }
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->shutdown = false;
    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
    {
        free(queue->jobs);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->jobs);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0)
    {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->jobs);
        free(queue);
        return NULL;
    }
    return queue;
}

void job_queue_destroy(JobQueue *queue)
{
    if (queue == NULL)
    {
        return;
    }

    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->jobs);
    free(queue);
}

int job_queue_push(JobQueue *queue, Job *job)
{
    if (queue == NULL || job == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count >= queue->capacity && !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    queue->jobs[queue->tail] = job;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

Job *job_queue_pop(JobQueue *queue)
{
    if (queue == NULL)
    {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    if (queue->shutdown && queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    Job *job = queue->jobs[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return job;
}

size_t job_queue_size(JobQueue *queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    size_t cont = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    
    return cont;
}

size_t job_queue_capacity(const JobQueue *queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    return queue->capacity;
}

void job_queue_shutdown(JobQueue *queue) {
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
}