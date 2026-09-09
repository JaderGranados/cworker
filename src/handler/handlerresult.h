#ifndef HANDLERRESULT_H
#define HANDLERRESULT_H

#include <stddef.h>

typedef struct HandlerResult HandlerResult;

HandlerResult *handler_result_create(int status, void *result, size_t result_size);

void handler_result_destroy(HandlerResult *result);

int handler_result_get_status(const HandlerResult *result);

void *handler_result_get_result(const HandlerResult *result);

size_t handler_result_get_result_size(const HandlerResult *result);

#endif // HANDLERRESULT_H