#include <stddef.h>
#include <stdlib.h>

#include "handlerresult.h"

struct HandlerResult {
    int status;
    void *result;
    size_t result_size;
};

HandlerResult *handler_result_create(int status, void *result, size_t result_size) {
    HandlerResult *handler_result = malloc(sizeof(HandlerResult));
    
    if (result == NULL || result_size == 0) {
        return NULL;
    }
    handler_result->status = status;
    handler_result->result_size = result_size;

    handler_result->result = malloc(result_size);
    if (handler_result->result == NULL) {
        free(result);
        return NULL;
    }

    return handler_result;
}

void handler_result_destroy(HandlerResult *result) {
    free(result->result);
    free(result);
}

int handler_result_get_status(const HandlerResult *result) {
    if (result == NULL) {
        return -1;
    }
    return result->status;
}

void *handler_result_get_result(const HandlerResult *result) {
    if (result == NULL) {
        return NULL;
    }
    return result->result;
}

size_t handler_result_get_result_size(const HandlerResult *result) {
    if (result == NULL) {
        return 0;
    }
    return result->result_size;
}