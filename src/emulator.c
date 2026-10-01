#include "emulator.h"
#include "pce/cpu.h"
#include "pce/memory.h"
#include <stdio.h>
#include <stdlib.h>

static void _emu_cpusync(void* emuptr) {
    Emulator* emu = (Emulator*)emuptr;

    u16 clock_adjust = emu->cpu->high_speed ? CPU_CLOCKDIV_HIGH : CPU_CLOCKDIV_LOW;

    emu->cycles++;
#ifdef _CPU_DEBUG
    printf(FILEPOS "cycle %lu\n", emu->cycles);
#endif

    emu->vce_clock -= clock_adjust;
    emu->timer_clock -= clock_adjust;

    if (emu->vce_clock <= 0) {
        emu->vce_clock = emu->vce_clock_modulo;
        // TODO: Update VCE/VDC
    }

    if (emu->vce_clock <= 0) {
        emu->timer_clock = TIMER_CLOCKDIV;
        // TODO: Update timer
    }
}

Emulator* emu_create() {
    Emulator* emu = calloc(1, sizeof(Emulator));

    emu->cpu = calloc(1, sizeof(CPU));
    emu->mem = calloc(1, sizeof(Memory) + sizeof(BusDevice) * BUS_DEVICE_COUNT);

    return emu;
}

void emu_destroy(Emulator* emu) {
    mem_free(emu->mem);
    free(emu->cpu);

    free(emu);
}

void emu_reset(Emulator* emu, bool hard) {
#ifdef _CPU_DEBUG
    printf(FILEPOS "reset (hard=%u)\n", hard);
#endif

    cpu_reset(emu->cpu, emu->mem, _emu_cpusync, (void*)emu);

    // todo: finish implementing
    emu->vce_clock_modulo = VCE_CLOCKDIV_5MHZ;
    emu->vce_clock = emu->vce_clock_modulo;

    emu->timer_clock = TIMER_CLOCKDIV;

    emu->cycles = 0;
}

void emu_step(Emulator* emu) {
    // todo: finish implementing
    cpu_step(emu->cpu, emu->mem, _emu_cpusync, (void*)emu);
}
