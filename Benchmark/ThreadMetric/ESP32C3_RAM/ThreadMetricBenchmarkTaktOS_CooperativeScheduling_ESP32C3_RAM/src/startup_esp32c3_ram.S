    .section .init, "ax"
    .global _start
    .global __taktos_trap_entry

_start:
    /* Disable machine interrupts during early startup */
    csrci mstatus, 0x8
    csrw mie, zero

    /* ABI setup */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, __stack_top

    /* Install direct trap entry */
    la t0, __taktos_trap_entry
    csrw mtvec, t0

    /* Clear .bss */
    la a0, __bss_start
    la a1, __bss_end
1:
    bgeu a0, a1, 2f
    sw zero, 0(a0)
    addi a0, a0, 4
    j 1b
2:

    call esp32c3_board_early_init
    call main

3:
    wfi
    j 3b

__taktos_trap_entry:
    addi sp, sp, -(31 * 4)
    sw ra,   0(sp)
    sw gp,   4(sp)
    sw tp,   8(sp)
    sw t0,  12(sp)
    sw t1,  16(sp)
    sw t2,  20(sp)
    sw s0,  24(sp)
    sw s1,  28(sp)
    sw a0,  32(sp)
    sw a1,  36(sp)
    sw a2,  40(sp)
    sw a3,  44(sp)
    sw a4,  48(sp)
    sw a5,  52(sp)
    sw a6,  56(sp)
    sw a7,  60(sp)
    sw s2,  64(sp)
    sw s3,  68(sp)
    sw s4,  72(sp)
    sw s5,  76(sp)
    sw s6,  80(sp)
    sw s7,  84(sp)
    sw s8,  88(sp)
    sw s9,  92(sp)
    sw s10, 96(sp)
    sw s11,100(sp)
    sw t3, 104(sp)
    sw t4, 108(sp)
    sw t5, 112(sp)
    sw t6, 116(sp)

    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call esp32c3_trap_dispatch

    lw ra,   0(sp)
    lw gp,   4(sp)
    lw tp,   8(sp)
    lw t0,  12(sp)
    lw t1,  16(sp)
    lw t2,  20(sp)
    lw s0,  24(sp)
    lw s1,  28(sp)
    lw a0,  32(sp)
    lw a1,  36(sp)
    lw a2,  40(sp)
    lw a3,  44(sp)
    lw a4,  48(sp)
    lw a5,  52(sp)
    lw a6,  56(sp)
    lw a7,  60(sp)
    lw s2,  64(sp)
    lw s3,  68(sp)
    lw s4,  72(sp)
    lw s5,  76(sp)
    lw s6,  80(sp)
    lw s7,  84(sp)
    lw s8,  88(sp)
    lw s9,  92(sp)
    lw s10, 96(sp)
    lw s11,100(sp)
    lw t3, 104(sp)
    lw t4, 108(sp)
    lw t5, 112(sp)
    lw t6, 116(sp)
    addi sp, sp, (31 * 4)
    mret
