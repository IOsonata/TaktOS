#ifndef KVB_CONFIG_TAKTOS_NRF54L15_H
#define KVB_CONFIG_TAKTOS_NRF54L15_H

#define KVB_PORT_TAKTOS 1

#define KVB_CORE_CLOCK_HZ 128000000u
#define KVB_TICK_HZ 1000u
#define KVB_MEASUREMENT_MS 10000u
#define KVB_WARMUP_MS 100u
#define KVB_RUNNER_STACK_SIZE 2048u
#define KVB_DEFAULT_STACK_SIZE 1024u
#define KVB_QUEUE_DEPTH 8u
#define KVB_QUEUE_MESSAGE_SIZE 16u

/* Thread priorities are no longer numeric.  See KvbPriority in
   include/kvb_kernel_port.h.  Tests use KVB_PRIORITY_NORMAL etc.; each
   kernel port maps the canonical levels onto its own native space. */

#endif /* KVB_CONFIG_TAKTOS_NRF54L15_H */
