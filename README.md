# CWorker

A concurrent job-processing system written in C, built as a hands-on project
for learning systems programming: memory ownership, pthreads, condition
variables, and (eventually) networking and IPC.

## Architecture

```
CLI args
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

This produces three executables:

- `cworker` — the main CLI entry point (built with AddressSanitizer enabled)
- `queue_test` — manual exercise of `JobQueue` behavior
- `worker_pool_test` — manual exercise of `WorkerPool` behavior

## Running

```sh
./cworker <type> <value>
```

Currently supported job types:

- `fibonacci <n>` — computes the n-th Fibonacci number

Example:

```sh
./cworker fibonacci 10
```

## Project layout

```
src/
├── job/            Job — opaque struct, owns a copy of type + payload
├── queue/           JobQueue — bounded circular buffer, mutex + condvars
├── worker/          WorkerPool — thread pool consuming from a JobQueue
├── handler/         Handler dispatch registry + HandlerResult
├── handlers/        Concrete handlers (e.g. fibonaccihandler)
├── mappers/         Boundary parsing: CLI args → typed MappedValue
└── main.c
tests/
├── queue_test.c
└── worker_pool_test.c
```

## Status

Implemented: Job, JobQueue, WorkerPool, handler registry/dispatch,
HandlerResult, the Fibonacci handler, and the CLI arg mapper — wired together
end-to-end in `main.c` with a full create → start → shutdown → destroy
lifecycle.

Not yet implemented: `ResultStore` (persisting completed job results so they
can be retrieved by job ID after processing), a TCP server/client, a custom
wire protocol, persistence, and the other topics on the long-term roadmap
(signals, IPC, metrics, retries, multiple worker servers, load balancing).

## Testing

There is no `ctest` integration yet — `queue_test` and `worker_pool_test` are
run directly:

```sh
./queue_test
./worker_pool_test
```

`cworker` itself is built with `-fsanitize=address`; run it under normal use
to catch memory errors as they're introduced.
