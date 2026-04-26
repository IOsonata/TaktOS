// =============================================================================
// Thread-Metric console driver — SCI2 polling mode on Arduino Nano R4.
// =============================================================================
// The Arduino Nano R4 routes RA4M1 SCI2 to the D0 / D1 header pins via:
//   D1 (TX)  = P302 / TXD2   — SCI2 transmit data (what we drive)
//   D0 (RX)  = P301 / RXD2   — SCI2 receive data (not used here)
//
// Verified against the official Nano R4 schematic (ABX00142) net labels
// P302_SCI2_TXD and P301_SCI2_RXD, and against variants/NANOR4/variant.cpp
// in ArduinoCore-renesas which maps D1 → BSP_IO_PORT_03_PIN_02 (P302).
//
// This is the same SCI channel that Arduino calls "Serial1" on the UNO R4
// Minima and Nano R4. The Nano R4's native USB (Serial over USB-C) is
// driven by the RA4M1 on-chip USBFS peripheral and would require a USB-CDC
// stack — deliberately out of scope for a bare-metal benchmark port.
//
// IMPORTANT — 5V I/O: the Nano R4 runs the RA4M1 at 5 V VCC (within its
// 5.5 V abs max), so D1 drives 5 V TTL directly with no level shifters on
// the board. The USB-serial adapter you connect MUST be 5V-tolerant on
// its RX input. FT232RL, CP2102N, and CH340G are all 5V-tolerant. Purely
// 3.3V adapters either clamp the signal or risk damage — use a 2k2/3k3
// resistor divider on the TX line, or buy a 5V-capable adapter.
//
// To view benchmark output, connect adapter RX to D1, adapter GND to GND.
// Baud 115200 8N1.
//
// Registers are accessed directly — no Renesas FSP or Arduino BSP
// dependency. PCLKB feeds SCI, and after SystemInit() PCLKB = ICLK = 48 MHz
// so BRR = 12 gives 48e6 / (32 * 13) = 115384 baud (+0.16% error, well
// within 8-bit UART tolerance).
// =============================================================================

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t SystemCoreClock;

// -----------------------------------------------------------------------------
// Register base addresses (RA4M1 reference manual R01UH0886).
// -----------------------------------------------------------------------------

// Module stop control register B — SCI enable bits. MSTPB29 = SCI2 stop
// (0 = module running, 1 = module stopped). Default after reset = all 1.
#define MSTPCRB_ADDR        0x40047004UL
#define MSTPCRB             (*(volatile uint32_t*)MSTPCRB_ADDR)
#define MSTPCRB_MSTPB29     (1U << 29)  // SCI2

// PORT write-protect register (8-bit). Must be 0x40 (B0WI=0, PFSWE=1) to
// allow writing to PFS registers.
#define PWPR_ADDR           0x40040D03UL
#define PWPR                (*(volatile uint8_t*)PWPR_ADDR)
#define PWPR_B0WI           (1U << 7)
#define PWPR_PFSWE          (1U << 6)

// PmnPFS (32-bit) — pin function select register for port m pin n.
// Layout: 0x40040800 + m*0x40 + n*0x04
//   PSEL[28:24] = peripheral select (for SCI2: PSEL = 0b00100 = 0x04)
//   PMR[16]     = 1 → pin is peripheral, 0 → pin is GPIO
//   NCODR[6]    = n-channel open-drain (0 = CMOS output)
//   DSCR[10]    = drive strength (0 = low-drive, 1 = high-drive)
#define P301PFS_ADDR        (0x40040800UL + 0x40U*3U + 0x04U*1U) /* 0x400408C4 */
#define P302PFS_ADDR        (0x40040800UL + 0x40U*3U + 0x04U*2U) /* 0x400408C8 */
#define P301PFS             (*(volatile uint32_t*)P301PFS_ADDR)
#define P302PFS             (*(volatile uint32_t*)P302PFS_ADDR)

#define PFS_PSEL_SCI_OFFSET (24U)
#define PFS_PSEL_SCI        (0x04U << PFS_PSEL_SCI_OFFSET)  // SCI0..9 mux
#define PFS_PMR             (1U << 16)
#define PFS_DSCR_HIGH       (1U << 10)

// SCI2 register block (8-bit registers except for a few 16/32-bit ones).
#define SCI2_BASE           0x40070040UL
#define SCI2_SMR            (*(volatile uint8_t*)(SCI2_BASE + 0x00U))
#define SCI2_BRR            (*(volatile uint8_t*)(SCI2_BASE + 0x01U))
#define SCI2_SCR            (*(volatile uint8_t*)(SCI2_BASE + 0x02U))
#define SCI2_TDR            (*(volatile uint8_t*)(SCI2_BASE + 0x03U))
#define SCI2_SSR            (*(volatile uint8_t*)(SCI2_BASE + 0x04U))
#define SCI2_SCMR           (*(volatile uint8_t*)(SCI2_BASE + 0x06U))
#define SCI2_SEMR           (*(volatile uint8_t*)(SCI2_BASE + 0x07U))

// SCR bits
#define SCR_TIE             (1U << 7)
#define SCR_RIE             (1U << 6)
#define SCR_TE              (1U << 5)
#define SCR_RE              (1U << 4)

// SSR bits
#define SSR_TDRE            (1U << 7)  // TDR empty
#define SSR_RDRF            (1U << 6)
#define SSR_ORER            (1U << 5)
#define SSR_FER             (1U << 4)
#define SSR_PER             (1U << 3)
#define SSR_TEND            (1U << 2)

// SCMR reset-default should write 0xF2 to keep SMIF=0, SINV=0, SDIR=0
// (LSB first), and bit 1 (reserved) = 1.
#define SCMR_DEFAULT        0xF2U

// SEMR: NFEN=0, BGDM=0, ABCS=0, ABCSE=0, BRME=0 → standard async.
#define SEMR_DEFAULT        0x00U

#define UART_BAUDRATE       115200u

// -----------------------------------------------------------------------------
// BRR computation — standard async mode (SMR.CKS=0, SEMR.BGDM=0, SEMR.ABCS=0):
//   BRR = (PCLKB / (64 * 2^(2*n - 1) * baud)) - 1, with n = CKS = 0
//       = (PCLKB / (32 * baud)) - 1
// At 48 MHz, baud 115200: BRR = 48e6/(32*115200) - 1 = 13.02 - 1 ≈ 12 → 0.16% error.
// -----------------------------------------------------------------------------
static inline uint8_t tm_sci_brr(uint32_t pclkb_hz, uint32_t baud)
{
    uint32_t divisor = 32u * baud;
    uint32_t n = (pclkb_hz + (divisor / 2u)) / divisor;
    if (n == 0u) return 0u;
    n -= 1u;
    if (n > 255u) n = 255u;
    return (uint8_t)n;
}

// -----------------------------------------------------------------------------
// Thread-Metric console hooks. Strong symbols — tm_port_taktos.cpp declares
// its stubs weak so this file wins at link time.
// -----------------------------------------------------------------------------

// Register write protection — MSTPCRB is protected by PRCR.PRC1.
// Without unlocking it, the MSTPCRB write is silently discarded and the
// subsequent SCI2 access BusFaults on an unclocked peripheral.
#define PRCR_ADDR           0x4001E3FEUL
#define PRCR                (*(volatile uint16_t*)PRCR_ADDR)
#define PRCR_KEY            (0xA500U)
#define PRCR_PRC1           (1U << 1)

// Helper: data-memory barrier — ensure the preceding store has left the CPU
// write buffer and the peripheral has observed it before we proceed.
#define DMB_DSB()   __asm__ volatile ("dsb" ::: "memory")

extern "C" void tm_hw_console_init(void)
{
    // ── Unlock PRC1 so we can write MSTPCRB. ─────────────────────────────
    PRCR = PRCR_KEY | PRCR_PRC1;
    DMB_DSB();

    // ── Release SCI2 from module-stop. ──────────────────────────────────
    // Read-modify-write MSTPCRB with PRC1 unlocked.
    uint32_t mstp = MSTPCRB;
    mstp &= ~MSTPCRB_MSTPB29;
    MSTPCRB = mstp;
    DMB_DSB();

    // Dummy read: forces the CPU to stall until the MSTPCRB write reaches
    // the peripheral bus, AND gives SCI2 its first clocked cycle before we
    // access any of its registers. Required on Renesas RA/RX/RZ parts.
    volatile uint32_t dummy = MSTPCRB;
    (void)dummy;
    DMB_DSB();

    // ── Relock PRC1. ────────────────────────────────────────────────────
    PRCR = PRCR_KEY;
    DMB_DSB();

    // ── Unlock the PFS registers (PWPR: clear B0WI first, then set PFSWE). ──
    PWPR  = 0x00u;
    DMB_DSB();
    PWPR  = PWPR_PFSWE;
    DMB_DSB();

    // ── Route P301 / P302 to SCI2. ──────────────────────────────────────
    // Renesas pattern: set PSEL with PMR=0 first, THEN set PMR=1 in a
    // separate write. When PMR is already 1, writes to PSEL are locked
    // out. Since PMR is 0 after reset we could do it in one write, but
    // splitting matches the documented sequence and is more robust.
    P301PFS = PFS_PSEL_SCI | PFS_DSCR_HIGH;          // PSEL=4, PMR=0
    DMB_DSB();
    P301PFS = PFS_PSEL_SCI | PFS_DSCR_HIGH | PFS_PMR; // now assert PMR=1
    DMB_DSB();

    P302PFS = PFS_PSEL_SCI;                           // PSEL=4, PMR=0
    DMB_DSB();
    P302PFS = PFS_PSEL_SCI | PFS_PMR;                 // now assert PMR=1
    DMB_DSB();

    // ── Relock PFS. ─────────────────────────────────────────────────────
    PWPR  = PWPR_B0WI;
    DMB_DSB();

    // ── Configure SCI2 async 8N1. ───────────────────────────────────────
    SCI2_SCR  = 0x00u;
    DMB_DSB();
    while ((SCI2_SCR & (SCR_TE | SCR_RE)) != 0u) {}
    SCI2_SCMR = SCMR_DEFAULT;
    SCI2_SEMR = SEMR_DEFAULT;
    SCI2_SMR  = 0x00u;
    SCI2_BRR  = tm_sci_brr(SystemCoreClock, UART_BAUDRATE);
    DMB_DSB();

    // ── Bit-time delay before enabling TE, per the SCI init sequence. ──
    for (uint32_t i = 0; i < 1000u; ++i) { __asm__ volatile ("nop"); }

    SCI2_SCR  = SCR_TE;
    DMB_DSB();
}

extern "C" void tm_putchar(int c)
{
    // Safety timeout so a misconfigured SCI cannot hang the benchmark
    // indefinitely. At 115200 baud one byte takes ~87 us = ~4200 cycles @
    // 48 MHz; 1,000,000 cycles is >200 ms of headroom.
    uint32_t timeout = 1000000u;
    while ((SCI2_SSR & SSR_TDRE) == 0u) {
        if (--timeout == 0u) return;
    }
    SCI2_TDR = (uint8_t)(c & 0xFFu);
}
