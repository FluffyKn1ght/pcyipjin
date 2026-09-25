#include "pce/cpu.h"
#include <assert.h>

#define CPU_TMA_MPR0 0x1
#define CPU_TMA_MPR1 0x2
#define CPU_TMA_MPR2 0x4
#define CPU_TMA_MPR3 0x8
#define CPU_TMA_MPR4 0x10
#define CPU_TMA_MPR5 0x20
#define CPU_TMA_MPR6 0x40
#define CPU_TMA_MPR7 0x80

#define CPU_VEC_RESET 0x1FFE
#define CPU_VEC_NMI 0xFFFC
#define CPU_VEC_TIMER 0xFFFA
#define CPU_VEC_IRQ1 0xFFF8
#define CPU_VEC_IRQ2 0xFFF6 // also BRK

inline static u32 _phys_addr(CPU* cpu, u16 logic_addr) {
    // get MPR register number
    u8 mpr_idx = (logic_addr & 0xF000) >> 12;
    assert(mpr_idx >= 0 && mpr_idx <= 7);

    // convert physical memory block ID to physical address and return it
    return (logic_addr & 0x0FFF) + (cpu->mpr[mpr_idx] * 0x2000);
}

void cpu_reset(CPU* cpu) {
    cpu->status.i = true;
    cpu->status.d = false;
    cpu->mpr[7] = 0;
    // TODO: Stop timer
    // TODO: Clear interrupt disable register in memory
    // TODO: Clear TIQ
    // TODO: Set low speed mode
    // TODO: Output H to port O
    cpu->status.t = false;
    // TODO: something is said about "ready state being cleared"
    // TODO: SYNC pin goes low
    // TODO: system clock is output to SX pin
    // TODO: HSM pin goes low
}

void cpu_step(CPU* cpu, Memory* mem) {
    // TODO: maybe actually emulate the damn CPU?
    return;
}
