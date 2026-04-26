// =============================================================================
// Thread-Metric benchmark entry — Arduino Nano R4 (RA4M1 Cortex-M4 @ 48 MHz).
// =============================================================================
// main() is called from ResetEntry after HOCO/PCLKB clock bring-up, .data copy,
// and .bss zero. The flow is:
//
//   1. tm_hw_console_init()      — bring up SCI2 on D1 (P302) at 115200 8N1
//   2. tm_initialize(test_initialize) — hand over to the TM framework; that
//      function creates the counter/report threads and hands control to the
//      TaktOS scheduler.
//
// test_initialize() is provided by the benchmark translation unit (for this
// project: basic_processing.c). tm_initialize() never returns under normal
// operation — it enters the TaktOS scheduler.
//
// UART viewing: connect a 5V-tolerant USB-serial adapter (FT232RL, CP2102N,
// CH340G, …) with adapter RX → D1 (TX) and adapter GND → GND. 115200 8N1.
// See README.md for the 5V-on-3.3V-adapter caveat.
// =============================================================================

extern "C" {
    void tm_hw_console_init(void);
    void tm_initialize(void (*test_initialization_function)(void));
    void test_initialize(void);
}

int main(void)
{
    tm_hw_console_init();
    tm_initialize(test_initialize);

    // Fallback: tm_initialize normally transfers control to the TaktOS
    // scheduler and never returns. If we somehow land back here, halt
    // cleanly so a debugger can observe the state.
    for (;;) { }
    return 0;
}
