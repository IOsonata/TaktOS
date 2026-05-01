#ifndef KVB_THREADX_PORT_H
#define KVB_THREADX_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Convenience: standard "start the runner with default args" entry point.
 * Same shape as kvb_freertos_start_default — lets a per-MCU main.cpp
 * call a single function regardless of which kernel is selected. */
int kvb_threadx_start_default(void);

#ifdef __cplusplus
}
#endif

#endif /* KVB_THREADX_PORT_H */
