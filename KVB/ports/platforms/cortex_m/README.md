# KVB Cortex-M Platform Port

This platform port provides the Cortex-M timing source used by KVB.

## DWT/CYCCNT support

`DWT->CYCCNT` is used only when the target architecture is known to provide it.

Supported by default:

- ARMv7-M / ARMv7E-M: Cortex-M3, Cortex-M4, Cortex-M7
- ARMv8-M Mainline / ARMv8.1-M Mainline: Cortex-M33, Cortex-M55, etc.

Not supported:

- ARMv6-M: Cortex-M0, Cortex-M0+

Examples of no-DWT parts include STM32F03xx devices. On those targets the port must not read `0xE0001004`.

## No-DWT fallback

For Cortex-M0/M0+ targets, provide a board-specific strong implementation of:

```c
uint64_t kvb_platform_cortex_m_fallback_time_us(void);
```

That function should return a monotonic microsecond timestamp from a hardware timer or another kernel-independent time source.

If no fallback is provided, timing-dependent KVB tests report `INVALID` instead of publishing meaningless benchmark numbers.

## Override knobs

You may force configuration from the build system:

```c
-DKVB_CORTEX_M_HAS_DWT=0
-DKVB_CORTEX_M_ENABLE_DWT=0
```

Do not force `KVB_CORTEX_M_ENABLE_DWT=1` on ARMv6-M/M0/M0+ targets.
