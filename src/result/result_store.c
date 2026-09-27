#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "result_store.h"

typedef struct ResultEntry {
    int job_id;
    ResultStatus status;
    HandlerResult *result;
} ResultEntry;

struct ResultStore {
    ResultEntry *entries;
    size_t size;
    size_t capacity;

    pthread_mutex_t mutex;
};

ResultStore *result_store_create(size_t capacity) {
    ResultStore *result_store = malloc(sizeof(*result_store));
    if (result_store == NULL) {
        return NULL;
    }
    if (capacity < 1) {
        capacity = 1;
    }
    result_store->entries = malloc(sizeof(*result_store->entries) * capacity);
    if (result_store->entries == NULL) {
        free(result_store);
        return NULL;
    }

    result_store->capacity = capacity;
    result_store->size = 0;

    if (pthread_mutex_init(&result_store->mutex, NULL) != 0) {
        free(result_store->entries);
        free(result_store);
        return NULL;
    }

    return result_store;
}

void result_store_destroy(ResultStore *result_store) {
    if (result_store == NULL) {
        return;
    }

    for (size_t i = 0; i < result_store->size; i++) {
        handler_result_destroy(result_store->entries[i].result);
    }
    free(result_store->entries);
    pthread_mutex_destroy(&result_store->mutex);
    free(result_store);
}

int result_store_submit(ResultStore *result_store, int job_id) {
    pthread_mutex_lock(&result_store->mutex);

    for (size_t i = 0; i < result_store->size; i++) {
        if (result_store->entries[i].job_id == job_id) {
            pthread_mutex_unlock(&result_store->mutex);
            return -1;
        }
    }

    if (result_store->size == result_store->capacity) {
        void *new_entries = realloc(result_store->entries, sizeof(*result_store->entries) * result_store->capacity * 2);
        if (new_entries == NULL) {
            pthread_mutex_unlock(&result_store->mutex);
            return -1;
        }
        result_store->capacity = result_store->capacity * 2;
        result_store->entries = new_entries;
    }

    result_store->entries[result_store->size].job_id = job_id;
    result_store->entries[result_store->size].status = RESULT_PENDING;
    result_store->entries[result_store->size].result = NULL;
    result_store->size++;

    pthread_mutex_unlock(&result_store->mutex);

    return 0;
}

int result_store_complete(ResultStore *result_store, int job_id, HandlerResult *result) {
    if (result == NULL) {
        return -1;
    }

    pthread_mutex_lock(&result_store->mutex);

    for (size_t i = 0; i < result_store->size; i++) {
        if (result_store->entries[i].job_id == job_id) {
            if (result_store->entries[i].status == RESULT_READY) {
                pthread_mutex_unlock(&result_store->mutex);
                return -1;
            }
            result_store->entries[i].result = result;
            result_store->entries[i].status = RESULT_READY;
            pthread_mutex_unlock(&result_store->mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&result_store->mutex);
    return -1;
}

ResultLookup result_store_get(ResultStore *result_store, int job_id) {
    ResultLookup lookup = { RESULT_NOT_FOUND, NULL };

    pthread_mutex_lock(&result_store->mutex);

    for (size_t i = 0; i < result_store->size; i++) {
        if (result_store->entries[i].job_id == job_id) {
            lookup.status = result_store->entries[i].status;
            lookup.result = result_store->entries[i].result;
            break;
        }
    }

    pthread_mutex_unlock(&result_store->mutex);

    return lookup;
}
