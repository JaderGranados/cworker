#ifndef RESULT_STORE_H
#define RESULT_STORE_H

#include <stddef.h>
#include "handler/handlerresult.h"

typedef struct ResultStore ResultStore;

ResultStore *result_store_create(size_t capacity);
void result_store_destroy(ResultStore *result_store);
int result_store_put(ResultStore *result_store, int job_id, HandlerResult *result);
HandlerResult *result_store_get(ResultStore *result_store, int job_id);

#endif