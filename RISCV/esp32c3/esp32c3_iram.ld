/* ---------------------------------------------------------------------------
 * esp32c3_iram.ld  —  TaktOS bare-metal linker script for ESP32-C3 / C6
 *
 * Memory map (ESP32-C3 TRM §2.3):
 *   IRAM   : 0x4037_8000  400 KB  (instruction + data, tightly coupled)
 *   DRAM   : 0x3FC8_0000  400 KB  (data RAM, accessible by CPU and DMA)
 *   ROM    : 0x4000_0000          (first-stage boot ROM, read-only)
 *   FLASH  : 0x4200_0000          (external flash via cache, 16 MB max)
 *   MMIO   : 0x6000_0000 …        (peripherals — not mapped here)
 *
 * Layout strategy (bare-metal, no IDF):
 *   Code and read-only data → IRAM  (no cache latency, deterministic timing)
 *   Mutable data            → DRAM
 *   Stack                   → DRAM (grows downward from top)
 *
 * The ROM boot stub (entry 0x40000004) copies .data from flash to DRAM and
 * zeroes .bss, then jumps to call_start_cpu0 in the application.  We provide
 * call_start_cpu0 in our startup; it calls TaktOSInit / TaktOSStart.
 *
 * On ESP32-C6 the memory map is the same; only the peripheral addresses
 * differ from ESP32-C3 (already handled in the port headers).
 *
 * Symbols exported for startup code:
 *   _data_start, _data_end, _data_load  — .data copy region
 *   _bss_start,  _bss_end               — .bss zero region
 *   _stack_top                          — initial stack pointer
 *
 * ---------------------------------------------------------------------------
 */

MEMORY {
    IRAM  (rwx) : ORIGIN = 0x40378000, LENGTH = 400K
    DRAM  (rw)  : ORIGIN = 0x3FC80000, LENGTH = 400K
}

ENTRY(call_start_cpu0)

SECTIONS {

    /* ── .text — executable code in IRAM ─────────────────────────────────── */
    .text : ALIGN(4) {
        /* Trap vector MUST be 256-byte aligned (mtvec direct mode requirement).
         * _takt_trap_rv32 is the first function; the linker places it here.    */
        . = ALIGN(256);
        KEEP(*(.text._takt_trap_rv32))
        KEEP(*(.text.TaktOSStartFirst))

        *(.text.unlikely .text.unlikely.*)   /* cold paths                    */
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    } > IRAM

    /* ── .rodata — read-only data in IRAM (avoids cache) ────────────────── */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
    } > IRAM

    /* ── .data — initialised mutable data in DRAM (LMA = IRAM end) ─────── */
    /* The ROM stub copies from LMA (load address in flash/IRAM) to VMA.     */
    _data_load = LOADADDR(.data);
    .data : ALIGN(4) {
        _data_start = .;
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        _data_end = .;
    } > DRAM AT > IRAM

    /* ── .bss — zero-initialised data in DRAM ───────────────────────────── */
    .bss (NOLOAD) : ALIGN(4) {
        _bss_start = .;
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > DRAM

    /* ── Heap (optional) — not used by TaktOS kernel ─────────────────────── */
    /* TaktOS uses static allocation only.  Reserve 0 bytes; extend if         */
    /* application code needs newlib malloc (not recommended for certified      */
    /* builds).                                                                 */
    .heap (NOLOAD) : ALIGN(8) {
        _heap_start = .;
        . += 0;       /* set to desired heap size (e.g. 4096) if needed      */
        _heap_end   = .;
    } > DRAM

    /* ── Stack ───────────────────────────────────────────────────────────── */
    /* Placed at the top of DRAM.  The startup code loads sp = _stack_top.   */
    /* Minimum recommended: 512 bytes for startup + interrupt nesting depth. */
    /* TaktOS per-task stacks are in static user-allocated arrays (not here). */
    _stack_top  = ORIGIN(DRAM) + LENGTH(DRAM);

    /* ── Debug / DWARF sections (not loaded on target) ──────────────────── */
    .debug_info     0 : { *(.debug_info)     }
    .debug_abbrev   0 : { *(.debug_abbrev)   }
    .debug_aranges  0 : { *(.debug_aranges)  }
    .debug_ranges   0 : { *(.debug_ranges)   }
    .debug_str      0 : { *(.debug_str)      }
    .debug_line     0 : { *(.debug_line)     }
    .debug_frame    0 : { *(.debug_frame)    }

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
