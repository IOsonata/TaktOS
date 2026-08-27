#ifndef ESP32C3_BOARD_H
#define ESP32C3_BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void esp32c3_board_early_init(void);
void esp32c3_board_console_write(const char *buf, uint32_t len);
void esp32c3_trap_dispatch(uint32_t mcause, uint32_t mepc, uint32_t mtval);

#ifdef __cplusplus
}
#endif

#endif // ESP32C3_BOARD_H
