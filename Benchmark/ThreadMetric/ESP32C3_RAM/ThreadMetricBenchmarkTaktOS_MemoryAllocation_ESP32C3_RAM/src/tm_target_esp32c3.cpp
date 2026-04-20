#include <stdint.h>
#include <stddef.h>

extern "C" {
#include "tm_target_esp32c3.h"
}

#include "esp32c3_board.h"

static void (*g_tm_soft_irq_handler)(void) = nullptr;

extern "C" int tm_target_install_soft_irq(void (*handler)(void))
{
    g_tm_soft_irq_handler = handler;
    return 0; // not wired yet; keep explicit so interrupt benchmarks are not faked
}

extern "C" void tm_target_raise_soft_irq(void)
{
    (void)g_tm_soft_irq_handler;
}

extern "C" void tm_target_console_init(void)
{
    esp32c3_board_early_init();
}

extern "C" void tm_target_console_write(const char *buf, uint32_t len)
{
    esp32c3_board_console_write(buf, len);
}
