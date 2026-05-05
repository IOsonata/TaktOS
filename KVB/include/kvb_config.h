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

/* KVB_DEFAULT_STACK_SIZE is the baseline usable stack size, in bytes.
 * Individual test roles can override it with the role-specific stack
 * macros below.  Tests must allocate buffers through KVB_THREAD_STACK_*
 * macros so each kernel port can add its native TCB, frame, guard, and
 * alignment overhead while preserving the requested usable stack. */
#ifndef KVB_DEFAULT_STACK_SIZE
#define KVB_DEFAULT_STACK_SIZE 1024u
#endif

#ifndef KVB_RUNNER_STACK_SIZE
#define KVB_RUNNER_STACK_SIZE 2048u
#endif

/* Test-thread stack sizes are split by role so small-SRAM targets can tune
 * only the helper threads that need less call depth while keeping the runner
 * stack large enough for logging.  Values are usable stack bytes; kernel
 * ports still apply their native TCB, frame, guard, and alignment overhead. */
#ifndef KVB_TEST_THREAD_STACK_SIZE
#define KVB_TEST_THREAD_STACK_SIZE KVB_DEFAULT_STACK_SIZE
#endif

#ifndef KVB_SCHED_WORKER_STACK_SIZE
#define KVB_SCHED_WORKER_STACK_SIZE KVB_TEST_THREAD_STACK_SIZE
#endif

#ifndef KVB_RT_THREAD_STACK_SIZE
#define KVB_RT_THREAD_STACK_SIZE KVB_TEST_THREAD_STACK_SIZE
#endif

#ifndef KVB_SYNC_THREAD_STACK_SIZE
#define KVB_SYNC_THREAD_STACK_SIZE KVB_TEST_THREAD_STACK_SIZE
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

/* TIME_SLEEP_001 also includes fixed odd durations: 1, 3, 5, 7,
   11, 19, and 37 ticks.  Keep this configurable case available for
   target-specific smoke checks without removing the odd-duration spread. */

/* Tick-based sleep APIs normally wake after N tick boundaries, not necessarily
   after N complete wall-clock tick periods from the call site.  If a test calls
   sleep just before the next tick, a request for N ticks may measure close to
   (N - 1) tick periods.  Keep strict minimum disabled unless the test explicitly
   aligns to a tick boundary before measuring. */
#ifndef KVB_SLEEP_TEST_STRICT_MINIMUM
#define KVB_SLEEP_TEST_STRICT_MINIMUM 0
#endif

#ifndef KVB_SLEEP_TEST_LONG_TICKS
#define KVB_SLEEP_TEST_LONG_TICKS 100u
#endif

#ifndef KVB_SLEEP_TEST_REPEAT_COUNT
#define KVB_SLEEP_TEST_REPEAT_COUNT 8u
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


/* Real-time responsiveness implementation selector.  The KVB manifest stays
 * canonical: every target registers the same RT test IDs.  Mainline
 * Cortex-M targets normally use DWT/CYCCNT.  Small/no-DWT targets should
 * provide kvb_platform_time_us() and tune KVB_RT_* stack/iteration values
 * before disabling this group.  Set KVB_ENABLE_RT_TESTS to 0 only when the
 * target truly has no usable high-resolution timebase or cannot reserve even
 * the shared RT helper stacks. */
#ifndef KVB_ENABLE_RT_TESTS
#define KVB_ENABLE_RT_TESTS 1u
#endif

#ifndef KVB_RT_LATENCY_ITERATIONS
#define KVB_RT_LATENCY_ITERATIONS 256u
#endif

#ifndef KVB_RT_JITTER_ITERATIONS
#define KVB_RT_JITTER_ITERATIONS 128u
#endif

#ifndef KVB_IRQ_PROBE_PRIORITY
#define KVB_IRQ_PROBE_PRIORITY 0xC0u
#endif

/* TaktOS-only wake-path trace diagnostic. Keep disabled for clean benchmark
   runs because it instruments the kernel tick/PendSV path. */
#ifndef KVB_ENABLE_TAKT_WAKE_TRACE_TEST
#define KVB_ENABLE_TAKT_WAKE_TRACE_TEST 0u
#endif

#ifndef KVB_LOG_BUFFER_SIZE
#define KVB_LOG_BUFFER_SIZE 192u
#endif

#ifndef KVB_MAX_METRICS_PER_TEST
#define KVB_MAX_METRICS_PER_TEST 16u
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
