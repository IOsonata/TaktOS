/**-------------------------------------------------------------------------
 * @file    ResetEntry.c
 *
 * @brief   ResetEntry for RA4M1 Nano R4, IOsonata style.
 *
 *          vector_ra4m1.c puts ResetEntry directly at vector[1].
 *          ARM hardware loads SP from vector[0] before entering this.
 *
 *          Flow:
 *              - P204 -> output (LED)
 *              - set VTOR to our table
 *              - enable FPU (CPACR)
 *              - enable MemManage/BusFault/UsageFault (SHCSR) so faults
 *                route to their own handlers instead of HardFault
 *              - copy .data flash -> RAM
 *              - zero .bss
 *              - SystemInit()                  (clock bring-up: HOCO 48 MHz)
 *              - // __libc_init_array()        (intentionally skipped, per
 *                                               IOsonata policy)
 *              - main()                        (Thread-Metric app entry)
 *              - infinite loop if main returns
 *
 *          The startup_ra4m1.S file ships a set of LED-pattern helpers
 *          (pattern_short / pattern_long / pattern_fast_blink /
 *          pattern_steady) that are linked but not called by default. They
 *          are kept as a ready-made diagnostic toolkit for the next bring-up
 *          campaign — if UART output goes silent after a change, drop a
 *          pattern_*() call here or in SystemInit to bisect the boot flow.
 *
 * Author: derived from I-SYST IOsonata ResetEntry.c (Nguyen Hoan Hoang)
 *---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* Linker-script symbols (gcc_ra4m1.ld). */
extern unsigned long _sidata;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;
extern unsigned long _end;

extern void SystemInit(void);
extern int  main(void);

/* ARMv7-M SCB registers. */
#define VTOR_ADDR           (*(volatile uint32_t *)0xE000ED08u)
#define CPACR_ADDR          (*(volatile uint32_t *)0xE000ED88u)
#define SHCSR_ADDR          (*(volatile uint32_t *)0xE000ED24u)
#define FLASH_VECTOR_BASE   0x00004000u

/* RA4M1 PORT2 for the LED. */
#define PORT2_PDR           (*(volatile uint16_t *)0x40040040u)
#define LED_MASK            ((uint16_t)(1u << 4))     /* P204 */

/* us-delay scale factor — SystemCoreClock is hard-wired 48 MHz. */
uint32_t SystemMicroSecLoopCnt = (48000000u + 8000000u) / 16000000u;


void ResetEntry(void)
{
    /* LED pin to output so any diagnostic works. */
    PORT2_PDR |= LED_MASK;

    /* Point exceptions at OUR vector table. */
    VTOR_ADDR = FLASH_VECTOR_BASE;
    __asm__ volatile ("dsb \n isb" ::: "memory");

    /* Turn the FPU on before any C code touches an FP register. */
    CPACR_ADDR |= (0xFu << 20);
    __asm__ volatile ("dsb \n isb" ::: "memory");

    /* Route MemManage/BusFault/UsageFault to their own handlers so
     * "solid LED" tells us WHICH fault type fired (1/2/3/4 pulses
     * before solid — see startup_ra4m1.S fault handlers). */
    SHCSR_ADDR |= (1u << 16) | (1u << 17) | (1u << 18);
    __asm__ volatile ("dsb \n isb" ::: "memory");

    /* Copy .data. */
    {
        unsigned long *src = &_sidata;
        unsigned long *dst = &_sdata;
        while (dst < &_edata) {
            *dst++ = *src++;
        }
    }

    /* Zero .bss. */
    {
        unsigned long *dst = &_sbss;
        while (dst < &_ebss) {
            *dst++ = 0u;
        }
    }

    /* Clock/peripheral bring-up (HOCO -> 48 MHz, PCLKB divider = /1). */
    SystemInit();

    /* __libc_init_array()  -- intentionally skipped, IOsonata policy. */

    /* Hand off to application. */
    (void)main();

    for (;;) {
        __asm__ volatile ("nop");
    }
}


/* ==========================================================================
 *  Weak newlib-nano stubs — from IOsonata/ARM/src/ResetEntry.c.
 * ==========================================================================*/

__attribute__((weak)) void _exit(int status)
{
    (void)status;
    __asm__ volatile ("bkpt #0");
    for (;;) { }
}

__attribute__((weak)) caddr_t _sbrk(int incr)
{
    static uint32_t top = 0;
    if (top == 0) {
        top = (uint32_t)&_end;
    }
    uint32_t prev = top;
    top += (uint32_t)incr;
    if (top > (uint32_t)&_estack) {
        top = prev;
        errno = ENOMEM;
        return (caddr_t)-1;
    }
    return (caddr_t)prev;
}

__attribute__((weak)) int _open (const char *path, int flags, int mode)
{ (void)path; (void)flags; (void)mode; return -1; }

__attribute__((weak)) int _close(int fd)
{ (void)fd; return -1; }

__attribute__((weak)) int _lseek(int fd, int offset, int whence)
{ (void)fd; (void)offset; (void)whence; return -1; }

__attribute__((weak)) int _read (int fd, char *buf, size_t len)
{ (void)fd; (void)buf; (void)len; return -1; }

__attribute__((weak)) int _write(int fd, const char *buf, size_t len)
{ (void)fd; (void)buf; (void)len; return (int)len; }

__attribute__((weak)) int _fstat(int fd, struct stat *st)
{ (void)fd; st->st_mode = S_IFCHR; return 0; }

__attribute__((weak)) int _isatty(int fd)
{ (void)fd; return 1; }

__attribute__((weak)) void _kill(int pid, int sig)
{ (void)pid; (void)sig; }

__attribute__((weak)) int _getpid(void)
{ return -1; }
