/* ---------------------------------------------------------------------------
 * startup_esp32c3.S  —  TaktOS bare-metal startup for ESP32-C3 / C6
 *
 * The ESP32-C3 ROM boot stub (call_start_cpu0 alias in ROM) copies .data
 * from flash/IRAM to DRAM, zeroes .bss, and jumps to the application's
 * call_start_cpu0 symbol.  We provide that symbol here.
 *
 * Startup sequence:
 *   1. Set up sp and gp (global pointer, GP-relaxation base).
 *   2. Copy .data from LMA → VMA (ROM may already do this; we redo it for
 *      robustness in case startup is entered directly via debugger).
 *   3. Zero .bss.
 *   4. Set mtvec to _takt_trap_rv32 in direct mode (0x00 suffix).
 *      Note: TaktOSTickInit() overwrites mtvec; this is a safe default.
 *   5. Call main() (C++ global constructors called from __libc_init_array
 *      if the newlib CRT is linked; omit for freestanding builds).
 *   6. If main() returns, spin forever.
 *
 * Stack pointer:  _stack_top (defined in linker script, top of DRAM).
 * Global pointer: __global_pointer$ (defined by linker when GP-relaxation
 *                 is enabled; harmless no-op if not used).
 *
 * @license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
 * ---------------------------------------------------------------------------
 */

        .section .text, "ax"
        .option norelax        /* suppress GP-relative relaxation during startup */
        .global call_start_cpu0
        .type   call_start_cpu0, %function
        .align  4

call_start_cpu0:

        /* ── 1. Set global pointer ────────────────────────────────────────── */
        /* Must be done with relaxation OFF so the la pseudo-op emits a full
         * auipc+addi pair rather than a GP-relative load.                     */
.option push
.option norelax
        la      gp, __global_pointer$
.option pop

        /* ── 2. Set stack pointer ─────────────────────────────────────────── */
        la      sp, _stack_top
        /* Ensure 16-byte alignment (RISC-V ABI) */
        andi    sp, sp, ~15

        /* ── 3. Copy .data from LMA to VMA ───────────────────────────────── */
        /* _data_load = load address (in IRAM, set by linker AT clause)        */
        /* _data_start/_data_end = virtual addresses in DRAM                   */
        la      a0, _data_start
        la      a1, _data_end
        la      a2, _data_load
        beq     a0, a1, .Lbss_zero     /* nothing to copy if .data is empty   */
        beq     a0, a2, .Lbss_zero     /* LMA == VMA: already in place        */
.Ldata_copy:
        lw      t0, 0(a2)
        sw      t0, 0(a0)
        addi    a0, a0, 4
        addi    a2, a2, 4
        bltu    a0, a1, .Ldata_copy

        /* ── 4. Zero .bss ─────────────────────────────────────────────────── */
.Lbss_zero:
        la      a0, _bss_start
        la      a1, _bss_end
        beq     a0, a1, .Lstart_done
.Lbss_loop:
        sw      zero, 0(a0)
        addi    a0, a0, 4
        bltu    a0, a1, .Lbss_loop

        /* ── 5. Set a safe default mtvec (direct mode) ───────────────────── */
        /* TaktOSTickInit() sets the real mtvec; this handles any spurious
         * interrupt that might arrive before TaktOSStart() is called.         */
        la      t0, _takt_trap_rv32
        andi    t0, t0, ~3             /* direct mode: mtvec[1:0] = 00        */
        csrw    mtvec, t0

        /* ── 6. Call main() ───────────────────────────────────────────────── */
        /* For freestanding bare-metal, link your application entry as main().
         * C++ global constructors: call __libc_init_array before main() if
         * using newlib's CRT (add .weak __libc_init_array below if unsure). */
.Lstart_done:
        call    main

        /* ── 7. Spin if main() returns ────────────────────────────────────── */
.Lhalt:
        j       .Lhalt

        .size call_start_cpu0, . - call_start_cpu0


        /* ── Default weak symbols for newlib CRT compatibility ──────────── */
        /* These resolve the reference without pulling in the full CRT when    */
        /* newlib is not linked.                                                */
        .weak   __libc_init_array
        .weak   __libc_fini_array

        /* ── External reference needed by startup ─────────────────────────── */
        .extern _takt_trap_rv32
        .extern _data_start
        .extern _data_end
        .extern _data_load
        .extern _bss_start
        .extern _bss_end
        .extern _stack_top

        .end
