# Minimal TaktOS KVB Example

This example is intentionally small. It shows the expected application entry point for running KVB on TaktOS.

```c
#include "kvb_taktos_port.h"

int main(void)
{
    return kvb_taktos_start_default();
}
```

## Required include paths

Add these include paths to the firmware project:

```text
KVB/include
KVB/ports/kernels/taktos
TaktOS_Dev/include
TaktOS_Dev/ARM/include
```

## Required source files

Add these source files:

```text
KVB/src/core/kvb_default_tests.c
KVB/src/core/kvb_log.c
KVB/src/core/kvb_platform_default.c
KVB/src/core/kvb_registry.c
KVB/src/core/kvb_result.c
KVB/src/core/kvb_runner.c
KVB/src/tests/sched/kvb_test_sched.c
KVB/src/tests/sync/kvb_test_sync.c
KVB/src/tests/ipc/kvb_test_ipc.c
KVB/src/tests/time/kvb_test_time.c
KVB/ports/kernels/taktos/kvb_port_taktos.c
KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c
KVB/examples/taktos_minimal/main.c
```

Define the target config header through your compiler options or include one before `kvb_config.h`.

For nRF52832:

```text
-include KVB/configs/taktos/kvb_config_taktos_nrf52832.h
```

For nRF54L15:

```text
-include KVB/configs/taktos/kvb_config_taktos_nrf54l15.h
```

The board/application should provide `kvb_platform_log_write()` if UART output is desired.
