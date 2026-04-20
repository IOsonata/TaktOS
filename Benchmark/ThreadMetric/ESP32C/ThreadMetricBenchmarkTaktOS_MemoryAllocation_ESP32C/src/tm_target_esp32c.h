#ifndef TM_TARGET_ESP32C_H
#define TM_TARGET_ESP32C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TM_ESP32C_CORE_CLOCK_HZ
#define TM_ESP32C_CORE_CLOCK_HZ    160000000u
#endif

#ifndef TM_ESP32C_TICK_HZ
#define TM_ESP32C_TICK_HZ          1000u
#endif

// Install a target-specific software interrupt and bind it to the supplied
// handler. Return non-zero on success.
int  tm_target_install_soft_irq(void (*handler)(void));

// Raise the software interrupt previously installed with
// tm_target_install_soft_irq().
void tm_target_raise_soft_irq(void);

// Console hooks used by tm_report.c through tm_putchar().
void tm_target_console_init(void);
void tm_target_console_write(const char *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // TM_TARGET_ESP32C_H
