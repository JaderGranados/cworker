#include "job/job.h"
#include "queue/jobqueue.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

typedef struct
{
    JobQueue *queue;
} ProducerArgs;

void *producer(void *arg)
{
    ProducerArgs *args = arg;

    for (int i = 1; i <= 5; i++)
    {
        printf("Producer: creating Job %d\n", i);

        Job *job = create_job(i, "test", NULL, 0);

        if (job == NULL)
        {
            printf("Producer: failed to create Job %d\n", i);
            continue;
        }

        printf("Producer: pushing Job %d\n", i);

        if (job_queue_push(args->queue, job) != 0)
        {
            printf("Producer: failed to push Job %d\n", i);
            destroy_job(job);
            continue;
        }

        printf("Producer: pushed Job %d\n", i);
    }

    return NULL;
}

typedef struct
{
    JobQueue *queue;
} ConsumerArgs;

void *consumer(void *arg)
{
    ConsumerArgs *args = arg;

    sleep(1);

    for (int i = 1; i <= 5; i++)
    {
        printf("Consumer: waiting for a job...\n");

        Job *job = job_queue_pop(args->queue);

        if (job == NULL)
        {
            printf("Consumer: failed to get job\n");
            continue;
        }

        printf("Consumer: got Job %d\n", get_job_id(job));

        destroy_job(job);

        sleep(1);
    }

    return NULL;
}

int main(void)
{
    JobQueue *queue = job_queue_create(2);

    if (queue == NULL)
    {
        printf("Failed to create queue\n");
        return 1;
    }

    ProducerArgs producer_args = {
        .queue = queue
    };

    ConsumerArgs consumer_args = {
        .queue = queue
    };

    pthread_t producer_thread;
    pthread_t consumer_thread;

    pthread_create(
        &producer_thread,
        NULL,
        producer,
        &producer_args
    );

    pthread_create(
        &consumer_thread,
        NULL,
        consumer,
        &consumer_args
    );

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    job_queue_destroy(queue);

    return 0;
}