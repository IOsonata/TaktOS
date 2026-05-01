# KVB Zephyr Minimal Example

This is the minimal application entry point for running KVB on Zephyr.

Build requirements:

- define `KVB_PORT_ZEPHYR=1`
- include `KVB/include`
- include `KVB/ports/kernels/zephyr`
- compile `KVB/src/core/*.c`
- compile `KVB/src/tests/*/*.c`
- compile `KVB/ports/kernels/zephyr/kvb_port_zephyr.c`
- compile one platform port, or use the weak default platform functions

Recommended Zephyr settings for balanced validation are provided in:

```text
KVB/Targets/Zephyr/common/prj.conf
```

Zephyr is a build-config-driven RTOS.  Do not create one project per MCU.
Select the MCU/board with `west build -b <board>`.
