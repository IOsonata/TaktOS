#ifndef KVB_CONFIG_H
#define KVB_CONFIG_H

#include <stdint.h>

#ifndef KVB_VERSION_STRING
#define KVB_VERSION_STRING "0.1.0-private"
#endif

#ifndef KVB_TICK_HZ
#define KVB_TICK_HZ 1000u
#endif

#ifndef KVB_CORE_CLOCK_HZ
#define KVB_CORE_CLOCK_HZ 64000000u
#endif

#ifndef KVB_MEASUREMENT_MS
#define KVB_MEASUREMENT_MS 1000u
#endif

#ifndef KVB_WARMUP_MS
#define KVB_WARMUP_MS 100u
#endif

/* KVB_DEFAULT_STACK_SIZE is the USABLE stack size requested by tests for
   each test thread, in bytes.  Tests must allocate
   KVB_THREAD_BUF_SIZE(KVB_DEFAULT_STACK_SIZE) bytes of backing buffer so
   that every kernel — including kernels that store the TCB inside the
   caller-provided block — delivers the same usable stack. */
#ifndef KVB_DEFAULT_STACK_SIZE
#define KVB_DEFAULT_STACK_SIZE 1024u
#endif

#ifndef KVB_RUNNER_STACK_SIZE
#define KVB_RUNNER_STACK_SIZE 2048u
#endif

#ifndef KVB_WORKER_THREAD_COUNT
#define KVB_WORKER_THREAD_COUNT 5u
#endif

#ifndef KVB_QUEUE_DEPTH
#define KVB_QUEUE_DEPTH 16u
#endif

#ifndef KVB_QUEUE_MESSAGE_SIZE
#define KVB_QUEUE_MESSAGE_SIZE 16u
#endif

#ifndef KVB_TEST_TIMEOUT_TICKS
#define KVB_TEST_TIMEOUT_TICKS (5u * KVB_TICK_HZ)
#endif

#ifndef KVB_SLEEP_TEST_TICKS
#define KVB_SLEEP_TEST_TICKS 10u
#endif

/* Tick-based sleep APIs normally wake after N tick boundaries, not necessarily
   after N complete wall-clock tick periods from the call site.  If a test calls
   sleep just before the next tick, a request for N ticks may measure close to
   (N - 1) tick periods.  Keep strict minimum disabled unless the test explicitly
   aligns to a tick boundary before measuring. */
#ifndef KVB_SLEEP_TEST_STRICT_MINIMUM
#define KVB_SLEEP_TEST_STRICT_MINIMUM 0
#endif

/* Maximum number of extra scheduler ticks accepted by TIME_SLEEP_001.
   Tick-based sleep APIs may round to the next tick boundary; one extra tick
   is expected on ports that report time using the native kernel tick count. */
#ifndef KVB_SLEEP_TEST_MAX_EXTRA_TICKS
#define KVB_SLEEP_TEST_MAX_EXTRA_TICKS 1u
#endif

/* Number of operations between time checks in throughput hot loops.  Larger
   values amortise the time-read cost across more measured ops, reducing
   measurement bias on fast kernel paths. */
#ifndef KVB_THROUGHPUT_BATCH
#define KVB_THROUGHPUT_BATCH 1024u
#endif

#ifndef KVB_LOG_BUFFER_SIZE
#define KVB_LOG_BUFFER_SIZE 192u
#endif

#ifndef KVB_MAX_METRICS_PER_TEST
#define KVB_MAX_METRICS_PER_TEST 8u
#endif

/* UART reset settling. Some debug probes / USB-UART bridges can deliver stale
   bytes from the previous run immediately after target reset. This quiet time
   lets the host-side virtual COM path settle before the first parseable KVB
   record is emitted. */
#ifndef KVB_STARTUP_QUIET_MS
#define KVB_STARTUP_QUIET_MS 250u
#endif

/* Conservative drain window for UART platform ports that can enqueue bytes
   into a TX FIFO but cannot query a hardware/driver TX-empty flag. */
#ifndef KVB_LOG_DRAIN_MS
#define KVB_LOG_DRAIN_MS 40u
#endif

#endif /* KVB_CONFIG_H */
