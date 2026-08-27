#ifndef TM_TARGET_ESP32C3_H
#define TM_TARGET_ESP32C3_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TM_ESP32C3_CORE_CLOCK_HZ
#define TM_ESP32C3_CORE_CLOCK_HZ 80000000u
#endif

#ifndef TM_ESP32C3_TICK_HZ
#define TM_ESP32C3_TICK_HZ 1000u
#endif

int  tm_target_install_soft_irq(void (*handler)(void));
void tm_target_raise_soft_irq(void);
void tm_target_console_init(void);
void tm_target_console_write(const char *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // TM_TARGET_ESP32C3_H
