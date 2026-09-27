#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "result_store.h"

typedef struct ResultEntry {
    int job_id;
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

    if (result_store->size > 0) {
        for (size_t i = 0; i < result_store->size; i++) {
            handler_result_destroy(result_store->entries[i].result);
        }
    }
    free(result_store->entries);
    pthread_mutex_destroy(&result_store->mutex);
    free(result_store);
}

int result_store_put(ResultStore *result_store, int job_id, HandlerResult *result) {
    if (result == NULL) {
        return -1;
    }
    pthread_mutex_lock(&result_store->mutex);
    if (result_store->size > 0) {
        for (size_t i = 0; i < result_store->size; i++) {
            if (result_store->entries[i].job_id == job_id) {
                pthread_mutex_unlock(&result_store->mutex);
                return -1;
            }
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
    result_store->entries[result_store->size].result = result;
    result_store->size++;

    pthread_mutex_unlock(&result_store->mutex);

    return 0;
}

HandlerResult *result_store_get(ResultStore *result_store, int job_id) {
    HandlerResult *result;

    pthread_mutex_lock(&result_store->mutex);
    for (size_t i = 0; i < result_store->size; i++) {
        if (result_store->entries[i].job_id == job_id) {
            result = result_store->entries[i].result;
            if (i < result_store->size-1) {
                for (size_t j = i; j < result_store->size-1; j++) {
                    result_store->entries[j] = result_store->entries[j + 1];
                }
            }
            result_store->size--;
            pthread_mutex_unlock(&result_store->mutex);
            return result;
        }
    }
    pthread_mutex_unlock(&result_store->mutex);

    return NULL;
}