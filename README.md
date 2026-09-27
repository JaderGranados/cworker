# CWorker

A concurrent job-processing system written in C, built as a hands-on project
for learning systems programming: memory ownership, pthreads, condition
variables, and (eventually) networking and IPC.

## Architecture

```
TCP client (SUBMIT/GET text protocol)
  ↓
TcpServer (accept loop + one thread per connection)
  ↓
Arg mapper (string → typed payload)
  ↓
Job (owns a copy of type + payload)
  ↓
JobQueue (bounded, thread-safe circular buffer)
  ↓
WorkerPool (pthreads pulling jobs off the queue)
  ↓
Handler dispatch (registry keyed by job type)
  ↓
Handler (business logic, e.g. fibonacci)
  ↓
HandlerResult (status + owned result data)
  ↓
ResultStore (submit/complete/get, polled by job_id over TCP)
```

Every stage has an explicit ownership rule for what it allocates and who is
responsible for freeing it — see the ownership notes in each header, and the
per-module comments in `src/`.

## Building

Requires CMake 3.20+ and a C17 compiler with pthreads.

```sh
mkdir -p build
cd build
cmake ..
cmake --build .
```

This produces four executables:

- `cworker` — the TCP server (built with AddressSanitizer enabled)
- `queue_test` — manual exercise of `JobQueue` behavior
- `worker_pool_test` — manual exercise of `WorkerPool` behavior
- `result_store_test` — manual exercise of `ResultStore` behavior (also built with ASan)

## Running

```sh
./cworker [port]   # defaults to 8080
```

It stays running until `Ctrl+C`. Talk to it over TCP with a plain-text,
one-command-per-line protocol — testable with `nc`:

```sh
printf 'SUBMIT fibonacci 10\n' | nc localhost 8080   # -> OK <job_id>
printf 'GET 1\n'               | nc localhost 8080   # -> PENDING | RESULT 1 55 | NOT_FOUND
```

Currently supported job types:

- `fibonacci <n>` — computes the n-th Fibonacci number

## Project layout

```
src/
├── job/            Job — opaque struct, owns a copy of type + payload
├── queue/           JobQueue — bounded circular buffer, mutex + condvars
├── worker/          WorkerPool — thread pool consuming from a JobQueue
├── handler/         Handler dispatch registry + HandlerResult
├── handlers/        Concrete handlers (e.g. fibonaccihandler)
├── mappers/         Boundary parsing: CLI args → typed MappedValue
├── result/          ResultStore — thread-safe job_id -> HandlerResult map
├── server/          TcpServer — accept loop + SUBMIT/GET text protocol
└── main.c
tests/
├── queue_test.c
├── worker_pool_test.c
└── result_store_test.c
```

## Status

Implemented: Job, JobQueue, WorkerPool, handler registry/dispatch,
HandlerResult, the Fibonacci handler, the CLI arg mapper, ResultStore
(submit/complete/get with `PENDING`/`READY`/`NOT_FOUND` states), and a
TcpServer speaking a small text protocol over it — wired together end-to-end
in `main.c` with a full create → start → shutdown → destroy lifecycle.

Not yet implemented: persistence, and the other topics on the long-term
roadmap (IPC, metrics, retries, multiple worker servers, load balancing, a
proper delete/ack command so `ResultStore` can reclaim memory for results
that were already delivered).

## Testing

There is no `ctest` integration yet — each test binary is run directly:

```sh
./queue_test
./worker_pool_test
./result_store_test
```

`cworker` itself is built with `-fsanitize=address`; run it under normal use
to catch memory errors as they're introduced.
