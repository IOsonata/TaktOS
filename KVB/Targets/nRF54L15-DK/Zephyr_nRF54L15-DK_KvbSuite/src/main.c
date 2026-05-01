/* SPDX-License-Identifier: MIT
 *
 * Application entry point for the KVB benchmark suite on Zephyr.
 *
 * Zephyr starts the kernel before main(), so the application thread
 * itself is already a real kernel thread with the priority configured
 * by CONFIG_MAIN_STACK_SIZE / CONFIG_MAIN_THREAD_PRIORITY.
 *
 * kvb_zephyr_start_default() (defined in
 * KVB/ports/kernels/zephyr/kvb_port_zephyr.c) re-prioritises this
 * thread to KVB_PRIORITY_HIGHEST, registers the default tests, and
 * runs the KVB runner.  When the runner returns the application falls
 * through to the Zephyr idle thread.
 */

#include "kvb_zephyr_port.h"

int main(void)
{
    return kvb_zephyr_start_default();
}
