# KVB ThreadX Minimal Example

This is the minimal application entry point for running KVB on ThreadX.

Build requirements:

- define `KVB_PORT_THREADX=1`
- include `KVB/include`
- include `KVB/ports/kernels/threadx`
- include ThreadX headers and `tx_user.h`
- compile `KVB/src/core/*.c`
- compile `KVB/src/tests/*/*.c`
- compile `KVB/ports/kernels/threadx/kvb_port_threadx.c`
- compile one platform port, for example `KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c`

The ThreadX KVB port provides `tx_application_define()`. Do not define another application-level `tx_application_define()` in the same benchmark executable unless you intentionally split the hook.
