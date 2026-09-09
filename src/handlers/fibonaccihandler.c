#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "handler/handler.h"
#include "handler/handlerresult.h"
#include "fibonaccihandler.h"

long int fibonacci(int it) {
    int x = 0;
    int y = 1;
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
    (void)payload_size;

    const long int *it = payload;
    long int fibonacci_result = fibonacci(*it);

    HandlerResult *result = handler_result_create(
        0,
        &fibonacci_result,
        sizeof(fibonacci_result)
    );

    // For demonstration purposes, we will just print the payload size.
    // In a real implementation, you would process the payload to compute the Fibonacci number.
    printf("The fibonacci result is: %ld\n", fibonacci_result);

    // Here you would normally compute the Fibonacci number based on the payload.
    // For now, we will just return success.
    return result;
}