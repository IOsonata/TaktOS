#include <stdint.h>
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);
void PendSV_Handler(void); void SysTick_Handler(void); void Irq0_Handler(void);
void Reset_Handler(void);
static void Default_Handler(void) { for (;;) {} }
void HardFault_Handler(void) { volatile int x = 1; while (x) {} }
__attribute__((section(".vectors"), used)) void (*const g_Vectors[16 + 8])(void) = {
  (void (*)(void))(&_estack), Reset_Handler, Default_Handler, HardFault_Handler,
  Default_Handler, Default_Handler, Default_Handler, 0, 0, 0, 0,
  Default_Handler, Default_Handler, 0, PendSV_Handler, SysTick_Handler,
  Irq0_Handler, Default_Handler, Default_Handler, Default_Handler,
  Default_Handler, Default_Handler, Default_Handler, Default_Handler };
void Reset_Handler(void) {
  uint32_t *s = &_sidata, *d = &_sdata; while (d < &_edata) *d++ = *s++;
  for (d = &_sbss; d < &_ebss; ) *d++ = 0;
  *(volatile uint32_t *)0xE000ED88 |= (0xFu << 20);   /* CPACR: enable FPU */
  __asm volatile ("dsb\n isb");
  main(); for (;;) {}
}
