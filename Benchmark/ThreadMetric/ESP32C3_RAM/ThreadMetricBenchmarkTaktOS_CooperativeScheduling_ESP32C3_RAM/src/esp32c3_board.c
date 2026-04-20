#include <stdint.h>
#include <stddef.h>

#include "esp32c3_board.h"

// These ROM APIs are provided by Espressif's ROM linker scripts.
extern void esp_rom_install_uart_printf(void);
extern void esp_rom_output_tx_one_char(char c);
extern int  esp_rom_printf(const char *fmt, ...);

void esp32c3_board_early_init(void)
{
    // Keep bring-up deliberately tiny. This assumes debugger-loaded RAM bring-up
    // and uses the ROM console path so we do not need a full UART driver yet.
    esp_rom_install_uart_printf();
    esp_rom_printf("
[TaktOS ESP32-C3 RAM bring-up]
");
}

void esp32c3_board_console_write(const char *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i) {
        if (buf[i] == '
') {
            esp_rom_output_tx_one_char('');
        }
        esp_rom_output_tx_one_char(buf[i]);
    }
}

__attribute__((weak)) void esp32c3_handle_irq(uint32_t irq_num)
{
    (void)irq_num;
}

void esp32c3_trap_dispatch(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    const uint32_t is_interrupt = mcause >> 31;
    const uint32_t reason = mcause & 0x7FFFFFFFu;

    if (is_interrupt) {
        esp32c3_handle_irq(reason);
        return;
    }

    esp_rom_printf("TRAP mcause=%lu mepc=0x%08lx mtval=0x%08lx
",
                   (unsigned long)reason,
                   (unsigned long)mepc,
                   (unsigned long)mtval);
    for (;;) {
        __asm__ volatile ("wfi");
    }
}
