/* SPDX-License-Identifier: MIT
 *
 * Application entry for the KVB benchmark suite on Zephyr.
 *
 * Zephyr starts the kernel before main(), so the application thread
 * is already a real kernel thread (priority CONFIG_MAIN_THREAD_PRIORITY,
 * stack CONFIG_MAIN_STACK_SIZE).
 *
 * kvb_zephyr_start_default() (in KVB/ports/kernels/zephyr/
 * kvb_port_zephyr.c) lifts this thread to KVB_PRIORITY_HIGHEST,
 * registers the default tests, and runs the KVB runner.  When the
 * runner returns the application falls through to the Zephyr idle
 * thread — there is no exit; the device sits idle until reset.
 *
 * Identical for every board.  Board / SoC selection happens via
 * `west build -b <board>` at build time.
 */

#include "kvb_zephyr_port.h"

int main(void)
{
    return kvb_zephyr_start_default();
}
