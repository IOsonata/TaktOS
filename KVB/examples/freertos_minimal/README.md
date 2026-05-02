# KVB FreeRTOS Minimal Example

This is the minimal application entry point for running KVB on FreeRTOS.

Build requirements:

- define `KVB_PORT_FREERTOS=1`
- include `KVB/include`
- include `KVB/ports/kernels/freertos`
- include the FreeRTOS headers and configured `FreeRTOSConfig.h`
- compile `KVB/src/core/*.c`
- compile `KVB/src/tests/*/*.c`
- compile `KVB/ports/kernels/freertos/kvb_port_freertos.c`
- compile one platform port, for example `KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c`

For like-for-like validation, configure FreeRTOS with comparable tick rate, stack checking, queue depth, mutex priority inheritance, and assertion behavior.
