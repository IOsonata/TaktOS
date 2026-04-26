# TaktOS standard C/C++ thread runtime draft

This draft adds the first standards-facing runtime layer for TaktOS.

## What is implemented

```
include/threads.h              C11/C17/C23 <threads.h> front end
src/std/threads.cpp            C threads implementation over PSE51 pthread
include/cxx/gthr-taktos.h      Draft libstdc++ gthr port over pthread
include/cxx/takt_cpp_runtime.h Runtime hook declarations
src/std/cpp_runtime.cpp        Weak malloc/local-static guard locks
examples/std/std_c_threads.c   Small C <threads.h> example
```

The design keeps one runtime substrate:

```
C <threads.h>
C++ libstdc++ gthr draft
        -> TaktOS PSE51 pthread layer
        -> TaktOS native thread/mutex/semaphore/tick primitives
```

There is no second scheduler and no general heap dependency added by this
draft.  Thread stacks and POSIX objects are still backed by the existing static
TaktOS/PSE51 pools.

## Current limits

- `mtx_recursive` returns `thrd_error` in the C API because the current pthread
  layer does not expose recursive mutexes.
- `gthr-taktos.h` has an internal recursive-mutex wrapper for C++ runtime use,
  but standard `std::thread` support still requires building libstdc++ with this
  gthr file selected for the TaktOS target.
- `std::future`, `std::promise`, and `std::async` are not guaranteed until a
  bounded heap or Partitura-backed allocator policy is added.
- TLS is limited by the current PSE51 pthread key implementation.

## Build integration

Add these two source files to the TaktOS library build when the standard runtime
layer is wanted:

```
src/std/threads.cpp
src/std/cpp_runtime.cpp
```

Use TaktOS include paths before the toolchain system include paths so that:

```
#include <threads.h>
```

finds `include/threads.h` on embedded freestanding targets.

For C++ standard-library integration, use `include/cxx/gthr-taktos.h` as the
TaktOS gthr target file when configuring or patching libstdc++.
