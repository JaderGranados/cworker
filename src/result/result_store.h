#ifndef RESULT_STORE_H
#define RESULT_STORE_H

#include <stddef.h>
#include "handler/handlerresult.h"

typedef enum ResultStatus {
    RESULT_NOT_FOUND,
    RESULT_PENDING,
    RESULT_READY
} ResultStatus;

typedef struct ResultLookup {
    ResultStatus status;
    HandlerResult *result;
} ResultLookup;

typedef struct ResultStore ResultStore;

ResultStore *result_store_create(size_t capacity);
void result_store_destroy(ResultStore *result_store);

int result_store_submit(ResultStore *result_store, int job_id);
int result_store_complete(ResultStore *result_store, int job_id, HandlerResult *result);
ResultLookup result_store_get(ResultStore *result_store, int job_id);

#endif
