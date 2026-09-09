#ifndef FIBONACCIHANDLER_H
#define FIBONACCIHANDLER_H
#include <stddef.h>

#include "handler/handler.h"
#include "handler/handlerresult.h"

HandlerResult *fibonacci_handler(const void *payload, size_t payload_size);

#endif // FIBONACCIHANDLER_H