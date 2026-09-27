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

    CHECK(result_store_put(store, 1, r1) == 0, "create_destroy_with_leftovers: put job 1 succeeded");
    CHECK(result_store_put(store, 2, r2) == 0, "create_destroy_with_leftovers: put job 2 succeeded");

    /* Neither result was ever retrieved via get() — destroy must free them itself. */
    result_store_destroy(store);
    printf("PASS: create_destroy_with_leftovers: destroy freed un-retrieved results without crashing\n");
}

static void test_put_then_get(void)
{
    ResultStore *store = result_store_create(4);
    HandlerResult *r = make_result_with_value(42);

    CHECK(result_store_put(store, 10, r) == 0, "put_then_get: put succeeded");

    HandlerResult *fetched = result_store_get(store, 10);
    CHECK(fetched != NULL, "put_then_get: get returned non-NULL");
    CHECK(fetched == r, "put_then_get: get returned the same HandlerResult pointer that was put");
    CHECK(handler_result_get_status(fetched) == JOB_SUCCESS, "put_then_get: status matches");
    CHECK(handler_result_get_result_size(fetched) == sizeof(uint64_t), "put_then_get: data_size matches");
    CHECK(*(uint64_t *)handler_result_get_result(fetched) == 42, "put_then_get: data matches");

    HandlerResult *second_fetch = result_store_get(store, 10);
    CHECK(second_fetch == NULL, "put_then_get: get() removed the entry (second get returns NULL)");

    /* We now own `fetched` (ownership transferred by get()). */
    handler_result_destroy(fetched);

    result_store_destroy(store);
}

static void test_get_missing_id(void)
{
    ResultStore *store = result_store_create(4);

    HandlerResult *fetched = result_store_get(store, 999);
    CHECK(fetched == NULL, "get_missing_id: get on unknown job_id returns NULL");

    result_store_destroy(store);
}

static void test_success_with_no_data(void)
{
    ResultStore *store = result_store_create(4);

    HandlerResult *empty = handler_result_create(JOB_SUCCESS, NULL, 0);
    CHECK(empty != NULL, "success_with_no_data: handler_result_create allows NULL data");
    CHECK(result_store_put(store, 20, empty) == 0, "success_with_no_data: put succeeded");

    HandlerResult *fetched = result_store_get(store, 20);
    CHECK(fetched != NULL, "success_with_no_data: get returned non-NULL");
    CHECK(handler_result_get_status(fetched) == JOB_SUCCESS, "success_with_no_data: status is JOB_SUCCESS");
    CHECK(handler_result_get_result(fetched) == NULL, "success_with_no_data: data is NULL");
    CHECK(handler_result_get_result_size(fetched) == 0, "success_with_no_data: data_size is 0");

    handler_result_destroy(fetched);
    result_store_destroy(store);
}

static void test_duplicate_job_id(void)
{
    ResultStore *store = result_store_create(4);
    HandlerResult *r1 = make_result_with_value(1);
    HandlerResult *r2 = make_result_with_value(2);

    CHECK(result_store_put(store, 30, r1) == 0, "duplicate_job_id: first put succeeded");
    CHECK(result_store_put(store, 30, r2) != 0, "duplicate_job_id: second put with same id failed");

    /* put() for r2 failed, so ownership never transferred — we must free it ourselves. */
    handler_result_destroy(r2);

    HandlerResult *fetched = result_store_get(store, 30);
    CHECK(fetched == r1, "duplicate_job_id: original entry (r1) is still the one stored");
    CHECK(*(uint64_t *)handler_result_get_result(fetched) == 1, "duplicate_job_id: original value unchanged");

    handler_result_destroy(fetched);
    result_store_destroy(store);
}

static void test_put_null_result(void)
{
    ResultStore *store = result_store_create(4);

    CHECK(result_store_put(store, 40, NULL) != 0, "put_null_result: put with NULL result fails");

    result_store_destroy(store);
}

static void test_growth(void)
{
    ResultStore *store = result_store_create(2);
    CHECK(store != NULL, "growth: store created with small capacity");

    const int count = 10;
    for (int i = 0; i < count; i++)
    {
        HandlerResult *r = make_result_with_value((uint64_t)i);
        int rc = result_store_put(store, i, r);
        if (rc != 0)
        {
            printf("FAIL: growth: put job %d failed\n", i);
            failures++;
            handler_result_destroy(r);
        }
    }

    int all_ok = 1;
    for (int i = 0; i < count; i++)
    {
        HandlerResult *fetched = result_store_get(store, i);
        if (fetched == NULL || *(uint64_t *)handler_result_get_result(fetched) != (uint64_t)i)
        {
            all_ok = 0;
        }
        if (fetched != NULL)
        {
            handler_result_destroy(fetched);
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

static void *concurrent_put_worker(void *arg)
{
    ThreadArgs *args = arg;
    for (int i = 0; i < args->count; i++)
    {
        int job_id = args->start_id + i;
        HandlerResult *r = make_result_with_value((uint64_t)job_id);
        if (result_store_put(args->store, job_id, r) != 0)
        {
            handler_result_destroy(r);
        }
    }
    return NULL;
}

static void test_concurrent_put_get(void)
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
        pthread_create(&threads[t], NULL, concurrent_put_worker, &args[t]);
    }

    for (int t = 0; t < THREAD_COUNT; t++)
    {
        pthread_join(threads[t], NULL);
    }

    int total = THREAD_COUNT * per_thread;
    int retrieved_ok = 1;
    for (int id = 0; id < total; id++)
    {
        HandlerResult *fetched = result_store_get(store, id);
        if (fetched == NULL || *(uint64_t *)handler_result_get_result(fetched) != (uint64_t)id)
        {
            retrieved_ok = 0;
            printf("FAIL: concurrent_put_get: job %d not retrievable or wrong value\n", id);
            failures++;
        }
        else
        {
            handler_result_destroy(fetched);
        }
    }
    CHECK(retrieved_ok, "concurrent_put_get: every concurrently-put job was retrieved exactly once with the right value");

    /* Every id should now be gone. */
    HandlerResult *should_be_null = result_store_get(store, 0);
    CHECK(should_be_null == NULL, "concurrent_put_get: store is empty after all results were retrieved");

    result_store_destroy(store);
}

int main(void)
{
    test_create_destroy_empty();
    test_create_destroy_with_leftovers();
    test_put_then_get();
    test_get_missing_id();
    test_success_with_no_data();
    test_duplicate_job_id();
    test_put_null_result();
    test_growth();
    test_concurrent_put_get();

    if (failures == 0)
    {
        printf("\nAll result_store tests passed.\n");
        return 0;
    }

    printf("\n%d result_store test(s) failed.\n", failures);
    return 1;
}
