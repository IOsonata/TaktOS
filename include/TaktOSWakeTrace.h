/**---------------------------------------------------------------------------
@file   TaktOSWakeTrace.h

@brief  Optional TaktOS wake-path timing trace for KVB diagnostics

This interface is intended for benchmark/debug builds that need to split a
sleep wakeup into kernel stages. Normal library builds do not enable these hooks.
Define TAKTOS_WAKE_TRACE_ENABLE only for wake-path diagnostic builds.

----------------------------------------------------------------------------*/
#ifndef __TAKTOSWAKETRACE_H__
#define __TAKTOSWAKETRACE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile uint32_t Enabled;
    volatile uint32_t Sequence;
    volatile uint32_t TickCount;
    volatile uint32_t ExpiredCount;
    volatile uint32_t SwitchRequested;
    volatile uint32_t CurrentPriority;
    volatile uint32_t NextPriority;
    volatile uint32_t TickEntryCycles;
    volatile uint32_t AfterWakeCycles;
    volatile uint32_t BeforePendSVCycles;
    volatile uint32_t PendSVEntryCycles;
    volatile uint32_t PendSVReturnCycles;
} TaktOSWakeTrace_t;

extern TaktOSWakeTrace_t g_TaktWakeTrace;

#define TAKT_WAKE_TRACE_ENABLED_OFFSET             0u
#define TAKT_WAKE_TRACE_SEQUENCE_OFFSET            4u
#define TAKT_WAKE_TRACE_TICK_COUNT_OFFSET          8u
#define TAKT_WAKE_TRACE_EXPIRED_COUNT_OFFSET       12u
#define TAKT_WAKE_TRACE_SWITCH_REQUESTED_OFFSET    16u
#define TAKT_WAKE_TRACE_CURRENT_PRIORITY_OFFSET    20u
#define TAKT_WAKE_TRACE_NEXT_PRIORITY_OFFSET       24u
#define TAKT_WAKE_TRACE_TICK_ENTRY_CYCLES_OFFSET   28u
#define TAKT_WAKE_TRACE_AFTER_WAKE_CYCLES_OFFSET   32u
#define TAKT_WAKE_TRACE_BEFORE_PENDSV_CYCLES_OFFSET 36u
#define TAKT_WAKE_TRACE_PENDSV_ENTRY_CYCLES_OFFSET 40u
#define TAKT_WAKE_TRACE_PENDSV_RETURN_CYCLES_OFFSET 44u

uint32_t TaktOSWakeTraceSupported(void);
void TaktOSWakeTraceReset(void);
void TaktOSWakeTraceEnable(uint32_t Enable);
const TaktOSWakeTrace_t *TaktOSWakeTraceSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __TAKTOSWAKETRACE_H__ */
