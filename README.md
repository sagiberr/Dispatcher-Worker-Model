# Dispatcher-Worker Model

A multithreaded dispatcher/worker system using POSIX threads in C. The dispatcher reads commands from a file and distributes them to worker threads via a synchronized queue.

##  Features

- Thread pool using `pthread`
- Dispatcher handles commands like:
  - `dispatcher_msleep`, `dispatcher_wait`
  - `worker msleep`, `increment`, `decrement`, `repeat`
- Worker threads process jobs concurrently
- Logs and statistics are generated

##  Usage

### Compile
```bash
gcc -pthread Dispatcher_Worker_model.c -o dispatcher_worker
