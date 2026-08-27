#include <stdint.h>
#include <stddef.h>

extern "C" {
#include "tm_target_esp32c.h"
}

// -----------------------------------------------------------------------------
// ESP32-C target stub for Thread-Metric.
//
// Replace these functions with your actual ESP32-C bring-up code once the
// TaktOS port reaches the point where UART and an internal software interrupt
// are available.
// -----------------------------------------------------------------------------

static void (*g_tm_soft_irq_handler)(void) = nullptr;

extern "C" int tm_target_install_soft_irq(void (*handler)(void))
{
    g_tm_soft_irq_handler = handler;

    // TODO: reserve and configure a real CPU/software interrupt source here.
    // Return non-zero when a real target interrupt has been bound.
    return 0;
}

extern "C" void tm_target_raise_soft_irq(void)
{
    // TODO: trigger the real target software interrupt here.
    // Keep the handler unused for now so the benchmark cannot accidentally
    // report interrupt results from a direct function call stub.
    (void)g_tm_soft_irq_handler;
}

extern "C" void tm_target_console_init(void)
{
    // TODO: configure your chosen UART / log console here.
}

extern "C" void tm_target_console_write(const char *buf, uint32_t len)
{
    // TODO: replace with a polling UART write or a minimal DMA/log backend.
    (void)buf;
    (void)len;
}
