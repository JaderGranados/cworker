#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "handler/handler.h"
#include "handler/handlerresult.h"
#include "fibonaccihandler.h"

uint64_t fibonacci(int it) {
    uint64_t x = 0;
    uint64_t y = 1;
    if (it < 0) {
        return -1;
    } else if (it < 1) {
        return 1;
    }

    for (int i = 2; i <= it; i++) {
        int aux = x;
        x = y;
        y += aux;
    }

    return y;
}

HandlerResult *fibonacci_handler(const void *payload, size_t payload_size) {
    if (payload == NULL || payload_size == 0) {
        return handler_result_create(
            JOB_FAILURE,
            NULL,
            0
        );
    }
    const long int *it = payload;
    uint64_t *fibonacci_result = malloc(sizeof(*fibonacci_result));
    if (fibonacci_result == NULL) {
        return handler_result_create(
            JOB_FAILURE,
            NULL,
            0
        );
    }
    *fibonacci_result = fibonacci(*it);

    HandlerResult *result = handler_result_create(
        JOB_SUCCESS,
        &fibonacci_result,
        sizeof(fibonacci_result)
    );

    printf("The fibonacci result is: %"PRIu64"\n", *fibonacci_result);
    return result;
}