#ifndef TX_USER_H
#define TX_USER_H

/* All Thread-X switches below use explicit '#define <NAME> 1'. Different
 * ThreadX architecture ports historically drifted between '#ifdef NAME',
 * '#if defined(NAME)', and '#if NAME'. Defining each switch with value 1
 * works for ALL three forms:
 *   #ifdef NAME         sees the symbol defined           OK
 *   #if defined(NAME)   sees the symbol defined           OK
 *   #if NAME            sees value 1, evaluates true      OK
 *   #if NAME == 1       explicit comparison succeeds      OK
 * Any future ThreadX port we adopt is therefore covered without re-editing
 * this header. The empty-macro form '#define NAME' would silently break
 * the '#if NAME' form ("#if with no expression").
 */

#define TX_DISABLE_ERROR_CHECKING               1
#define TX_DISABLE_PREEMPTION_THRESHOLD         1
#define TX_DISABLE_NOTIFY_CALLBACKS             1
#define TX_DISABLE_REDUNDANT_CLEARING           1

/* On Cortex-M33 / M55 (ARMv8-M Mainline) we run single-mode secure: no
 * TrustZone, no NS world. Without TX_SINGLE_MODE_SECURE the M33/M55 port
 * emits secure/NS context-switch glue that requires a TrustZone partition
 * we don't have. On Cortex-M0/M3/M4/M7 (ARMv6-M / ARMv7-M) the macro is
 * unknown to the port and harmless to omit. */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8_1M_MAIN__)
#define TX_SINGLE_MODE_SECURE                   1
#endif

/* #define TX_DISABLE_STACK_FILLING              1 */
/* #define TX_NOT_INTERRUPTABLE                  1 */

#define TX_TIMER_TICKS_PER_SECOND               1000
#define TX_TIMER_PROCESS_IN_ISR                 1
#define TX_ENABLE_STACK_CHECKING                1
#define TX_ENABLE_FPU_SUPPORT                   1

#endif
