#include "pce/cpu.h"
#include <assert.h>

inline static u21 _phys_addr(CPU* cpu, u16 logic_addr) {
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
