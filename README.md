# jfc-dispatch
A header-only work-dispatch interface that allows a library to parallelize work using worker threads supplied by its caller, while falling back to serial execution if no worker threads are supplied. The idea is to be able to take advantage of multithreading, but without a library itself directly owning its own threads and without forcing the user to use one threading strategy or another in particular.

## Overview
jfc-dispatch provides two interfaces for dispatching work to a task scheduler or thread pool.

### Indexed work
The primary interface distributes a fixed number of chunks of work. A task_dispatcher_type accepts a chunk count and a callback that processes an individual chunk by index. The run function invokes this interface, or executes the chunks serially when no dispatcher is supplied or only one chunk is requested.

A dispatcher must ensure that all chunks have finished executing before it returns. Exceptions thrown by the chunk callback are captured and rethrown by run after dispatch completes.

### Discrete tasks
For workloads expressed as independent tasks rather than indexed chunks, task_submitter_type provides an interface for submitting a collection of parameterless tasks. The run_inline function executes such a collection serially.

### Adapting a task scheduler
The dispatcher_of and submitter_of functions adapt an existing task scheduler or thread pool to these interfaces. They expect the pool to provide run_and_wait and add_tasks, respectively.

These adapters capture the supplied pool by reference, so the pool must outlive the resulting interface object.
