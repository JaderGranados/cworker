#include "job.h"
#include <stdlib.h>
#include <string.h>

struct Job {
    int id;
    char *type;
    void *payload;
    size_t payload_size;
};

Job *create_job(int id, const char *type, void *payload, size_t payload_size) {
    Job *job = malloc(sizeof(Job));
    if (job == NULL) {
        return NULL;
    }

    job->id = id;
    job->type = NULL;
    job->payload = NULL;
    job->payload_size = payload_size;

    job->type = malloc(strlen(type) + 1);
    if (job->type == NULL) {
        free(job);
        return NULL;
    }
    strcpy(job->type, type);

    if (payload_size > 0) {
        job->payload = malloc(payload_size);
        if (job->payload == NULL) {
            free(job->type);
            free(job);
            return NULL;
        }
        memcpy(job->payload, payload, payload_size);
    }

    return job;
}

int get_job_id(const Job *job) {
    return job->id;
}

const char *get_job_type(const Job *job) {
    return job->type;
}

void *get_job_payload(const Job *job) {
    return job->payload;
}

size_t get_job_payload_size(const Job *job) {
    return job->payload_size;
}

void destroy_job(Job *job) {
    free(job->type);
    free(job->payload);
    free(job);
}