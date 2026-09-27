cmake_minimum_required(VERSION 3.20)

project(cworker C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(cworker
    src/main.c
    src/job/job.c
    src/queue/jobqueue.c
    src/handler/handler.c
    src/handler/handlerresult.c
    src/handlers/fibonaccihandler.c
    src/worker/worker.c
    src/mappers/argmapper.c
    src/result/result_store.c
)

add_executable(queue_test
    tests/queue_test.c
    src/job/job.c
    src/queue/jobqueue.c
    src/worker/worker.c
    src/handler/handler.c
    src/handler/handlerresult.c
    src/handlers/fibonaccihandler.c
    src/mappers/argmapper.c
)

add_executable(worker_pool_test
    tests/worker_pool_test.c
    src/job/job.c
    src/queue/jobqueue.c
    src/worker/worker.c
    src/handler/handler.c
    src/handler/handlerresult.c
    src/handlers/fibonaccihandler.c
    src/mappers/argmapper.c
)

add_executable(result_store_test
    tests/result_store_test.c
    src/job/job.c
    src/queue/jobqueue.c
    src/worker/worker.c
    src/handler/handler.c
    src/handler/handlerresult.c
    src/handlers/fibonaccihandler.c
    src/mappers/argmapper.c
    src/result/result_store.c
)

target_compile_options(cworker PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fsanitize=address
)

target_link_options(cworker PRIVATE
    -fsanitize=address
)

target_compile_options(result_store_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fsanitize=address
)

target_link_options(result_store_test PRIVATE
    -fsanitize=address
)

find_package(Threads REQUIRED)
target_link_libraries(cworker PRIVATE Threads::Threads)
target_link_libraries(result_store_test PRIVATE Threads::Threads)
target_include_directories(cworker PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_include_directories(queue_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_include_directories(worker_pool_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_include_directories(result_store_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)