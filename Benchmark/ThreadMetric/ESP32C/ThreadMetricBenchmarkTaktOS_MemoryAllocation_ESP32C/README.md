# TaktOS Thread-Metric ESP32-C suite

This suite mirrors the existing Nordic TaktOS Thread-Metric layout, but the
chip-specific work is isolated in `tm_target_esp32c.h` and
`tm_target_esp32c_stub.cpp`.

What is ready:
- the six Thread-Metric benchmark projects
- the full TaktOS-to-Thread-Metric API shim (`tm_port.cpp`)
- benchmark logic files copied from the working TaktOS/nRF54L15 suite
- buffered report output via `tm_putchar()` -> `tm_target_console_write()`

What you still need to wire for your ESP32-C port:
1. `tm_target_console_init()`
2. `tm_target_console_write()`
3. `tm_target_install_soft_irq()` and `tm_target_raise_soft_irq()`
4. actual startup / linker / board project files for your ESP32-C bring-up

Notes:
- The current six selected Thread-Metric tests do not depend on the software
  interrupt hook for their core logic, so you can bring up most of the suite
  before wiring the soft-IRQ path.
- `TM_ESP32C_CORE_CLOCK_HZ` and `TM_ESP32C_TICK_HZ` default to 160 MHz / 1000 Hz
  and can be overridden per target in `tm_target_esp32c.h`.
