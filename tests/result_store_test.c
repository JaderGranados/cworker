#define THREAD_COUNT 8

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "handler/handler.h"
#include "handler/handlerresult.h"
#include "result/result_store.h"

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { \
            printf("PASS: %s\n", msg); \
        } else { \
            printf("FAIL: %s\n", msg); \
            failures++; \
        } \
    } while (0)

static HandlerResult *make_result_with_value(uint64_t value)
{
    uint64_t *data = malloc(sizeof(*data));
    *data = value;
    return handler_result_create(JOB_SUCCESS, data, sizeof(*data));
}

static void test_create_destroy_empty(void)
{
    ResultStore *store = result_store_create(4);
    CHECK(store != NULL, "create_destroy_empty: store created");
    result_store_destroy(store);
    printf("PASS: create_destroy_empty: destroy on empty store did not crash\n");
}

static void test_create_destroy_with_leftovers(void)
{
    ResultStore *store = result_store_create(4);
    CHECK(store != NULL, "create_destroy_with_leftovers: store created");

    HandlerResult *r1 = make_result_with_value(1);
    HandlerResult *r2 = make_result_with_value(2);

    CHECK(result_store_submit(store, 1) == 0, "create_destroy_with_leftovers: submit job 1 succeeded");
    CHECK(result_store_complete(store, 1, r1) == 0, "create_destroy_with_leftovers: complete job 1 succeeded");
    CHECK(result_store_submit(store, 2) == 0, "create_destroy_with_leftovers: submit job 2 succeeded");
    CHECK(result_store_complete(store, 2, r2) == 0, "create_destroy_with_leftovers: complete job 2 succeeded");

    /* Neither result was ever retrieved via get() — destroy must free them itself. */
    result_store_destroy(store);
    printf("PASS: create_destroy_with_leftovers: destroy freed un-retrieved results without crashing\n");
}

static void test_submit_complete_then_get(void)
{
    ResultStore *store = result_store_create(4);
    HandlerResult *r = make_result_with_value(42);

    CHECK(result_store_submit(store, 10) == 0, "submit_complete_then_get: submit succeeded");
    CHECK(result_store_complete(store, 10, r) == 0, "submit_complete_then_get: complete succeeded");

    ResultLookup lookup = result_store_get(store, 10);
    CHECK(lookup.status == RESULT_READY, "submit_complete_then_get: status is RESULT_READY");
    CHECK(lookup.result == r, "submit_complete_then_get: get returned the same HandlerResult pointer that was completed");
    CHECK(handler_result_get_status(lookup.result) == JOB_SUCCESS, "submit_complete_then_get: status matches");
    CHECK(handler_result_get_result_size(lookup.result) == sizeof(uint64_t), "submit_complete_then_get: data_size matches");
    CHECK(*(uint64_t *)handler_result_get_result(lookup.result) == 42, "submit_complete_then_get: data matches");

    ResultLookup second_lookup = result_store_get(store, 10);
    CHECK(second_lookup.status == RESULT_READY, "submit_complete_then_get: get() is non-destructive (second get is still RESULT_READY)");
    CHECK(second_lookup.result == r, "submit_complete_then_get: second get returns the same pointer");

    /* get() no longer transfers ownership — the store still owns `r`, destroy() will free it. */
    result_store_destroy(store);
}

static void test_get_missing_id(void)
{
    ResultStore *store = result_store_create(4);

    ResultLookup lookup = result_store_get(store, 999);
    CHECK(lookup.status == RESULT_NOT_FOUND, "get_missing_id: get on unknown job_id returns RESULT_NOT_FOUND");
    CHECK(lookup.result == NULL, "get_missing_id: result is NULL");

    result_store_destroy(store);
}

static void test_get_pending(void)
{
    ResultStore *store = result_store_create(4);

    CHECK(result_store_submit(store, 5) == 0, "get_pending: submit succeeded");

    ResultLookup lookup = result_store_get(store, 5);
    CHECK(lookup.status == RESULT_PENDING, "get_pending: status is RESULT_PENDING before completion");
    CHECK(lookup.result == NULL, "get_pending: result is NULL while pending");

    result_store_destroy(store);
}

static void test_success_with_no_data(void)
{
    ResultStore *store = result_store_create(4);

    HandlerResult *empty = handler_result_create(JOB_SUCCESS, NULL, 0);
    CHECK(empty != NULL, "success_with_no_data: handler_result_create allows NULL data");
    CHECK(result_store_submit(store, 20) == 0, "success_with_no_data: submit succeeded");
    CHECK(result_store_complete(store, 20, empty) == 0, "success_with_no_data: complete succeeded");

    ResultLookup lookup = result_store_get(store, 20);
    CHECK(lookup.status == RESULT_READY, "success_with_no_data: status is RESULT_READY");
    CHECK(handler_result_get_status(lookup.result) == JOB_SUCCESS, "success_with_no_data: status is JOB_SUCCESS");
    CHECK(handler_result_get_result(lookup.result) == NULL, "success_with_no_data: data is NULL");
    CHECK(handler_result_get_result_size(lookup.result) == 0, "success_with_no_data: data_size is 0");

    result_store_destroy(store);
}

static void test_duplicate_job_id(void)
{
    ResultStore *store = result_store_create(4);
    HandlerResult *r1 = make_result_with_value(1);
    HandlerResult *r2 = make_result_with_value(2);

    CHECK(result_store_submit(store, 30) == 0, "duplicate_job_id: first submit succeeded");
    CHECK(result_store_submit(store, 30) != 0, "duplicate_job_id: second submit with same id failed");
    CHECK(result_store_complete(store, 30, r1) == 0, "duplicate_job_id: complete succeeded");
    CHECK(result_store_complete(store, 30, r2) != 0, "duplicate_job_id: second complete on an already-READY entry failed");

    /* The second complete() failed, so ownership of r2 never transferred — free it ourselves. */
    handler_result_destroy(r2);

    ResultLookup lookup = result_store_get(store, 30);
    CHECK(lookup.result == r1, "duplicate_job_id: original entry (r1) is still the one stored");
    CHECK(*(uint64_t *)handler_result_get_result(lookup.result) == 1, "duplicate_job_id: original value unchanged");

    result_store_destroy(store);
}

static void test_complete_null_result(void)
{
    ResultStore *store = result_store_create(4);

    CHECK(result_store_submit(store, 40) == 0, "complete_null_result: submit succeeded");
    CHECK(result_store_complete(store, 40, NULL) != 0, "complete_null_result: complete with NULL result fails");

    result_store_destroy(store);
}

static void test_complete_without_submit(void)
{
    ResultStore *store = result_store_create(4);
    HandlerResult *r = make_result_with_value(7);

    CHECK(result_store_complete(store, 99, r) != 0, "complete_without_submit: complete on a never-submitted id fails");

    /* complete() failed, so we still own r. */
    handler_result_destroy(r);
    result_store_destroy(store);
}

static void test_growth(void)
{
    ResultStore *store = result_store_create(2);
    CHECK(store != NULL, "growth: store created with small capacity");

    const int count = 10;
    for (int i = 0; i < count; i++)
    {
        if (result_store_submit(store, i) != 0)
        {
            printf("FAIL: growth: submit job %d failed\n", i);
            failures++;
            continue;
        }
        HandlerResult *r = make_result_with_value((uint64_t)i);
        if (result_store_complete(store, i, r) != 0)
        {
            printf("FAIL: growth: complete job %d failed\n", i);
            failures++;
            handler_result_destroy(r);
        }
    }

    int all_ok = 1;
    for (int i = 0; i < count; i++)
    {
        ResultLookup lookup = result_store_get(store, i);
        if (lookup.status != RESULT_READY || *(uint64_t *)handler_result_get_result(lookup.result) != (uint64_t)i)
        {
            all_ok = 0;
        }
    }
    CHECK(all_ok, "growth: all entries beyond initial capacity remained independently retrievable");

    result_store_destroy(store);
}

typedef struct
{
    ResultStore *store;
    int start_id;
    int count;
} ThreadArgs;

static void *concurrent_submit_worker(void *arg)
{
    ThreadArgs *args = arg;
    for (int i = 0; i < args->count; i++)
    {
        int job_id = args->start_id + i;
        if (result_store_submit(args->store, job_id) != 0)
        {
            continue;
        }
        HandlerResult *r = make_result_with_value((uint64_t)job_id);
        if (result_store_complete(args->store, job_id, r) != 0)
        {
            handler_result_destroy(r);
        }
    }
    return NULL;
}

static void test_concurrent_submit_complete_get(void)
{
    const int per_thread = 50;
    ResultStore *store = result_store_create(4);

    pthread_t threads[THREAD_COUNT];
    ThreadArgs args[THREAD_COUNT];

    for (int t = 0; t < THREAD_COUNT; t++)
    {
        args[t].store = store;
        args[t].start_id = t * per_thread;
        args[t].count = per_thread;
        pthread_create(&threads[t], NULL, concurrent_submit_worker, &args[t]);
    }

    for (int t = 0; t < THREAD_COUNT; t++)
    {
        pthread_join(threads[t], NULL);
    }

    int total = THREAD_COUNT * per_thread;
    int retrieved_ok = 1;
    for (int id = 0; id < total; id++)
    {
        ResultLookup lookup = result_store_get(store, id);
        if (lookup.status != RESULT_READY || *(uint64_t *)handler_result_get_result(lookup.result) != (uint64_t)id)
        {
            retrieved_ok = 0;
            printf("FAIL: concurrent_submit_complete_get: job %d not retrievable or wrong value\n", id);
            failures++;
        }
    }
    CHECK(retrieved_ok, "concurrent_submit_complete_get: every concurrently-submitted job was retrieved with the right value");

    /* get() is non-destructive now — entries stay put instead of disappearing after one read. */
    ResultLookup still_there = result_store_get(store, 0);
    CHECK(still_there.status == RESULT_READY, "concurrent_submit_complete_get: entries remain retrievable after get (non-destructive)");

    result_store_destroy(store);
}

int main(void)
{
    test_create_destroy_empty();
    test_create_destroy_with_leftovers();
    test_submit_complete_then_get();
    test_get_missing_id();
    test_get_pending();
    test_success_with_no_data();
    test_duplicate_job_id();
    test_complete_null_result();
    test_complete_without_submit();
    test_growth();
    test_concurrent_submit_complete_get();

    if (failures == 0)
    {
        printf("\nAll result_store tests passed.\n");
        return 0;
    }

    printf("\n%d result_store test(s) failed.\n", failures);
    return 1;
}
