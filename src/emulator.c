#include "emulator.h"
#include "memory.h"
#include "pce/cpu.h"
#include "pce/vce.h"
#include "pce/vdc.h"
#include <stdio.h>
#include <stdlib.h>

static void _emu_cpu_sync(void* emuptr) {
    Emulator* emu = (Emulator*)emuptr;

    u16 clocks = emu->cpu->high_speed ? CPU_CLOCKDIV_HIGH : CPU_CLOCKDIV_LOW;

    emu->cycles++;

#ifdef _CPU_DEBUG
    printf(FILEPOS "cycle %lu\n", emu->cycles);
#endif

    vce_tick(emu->vce, clocks, vdc_step, (void*)emu->vdc);
}

Emulator* emu_create() {
    Emulator* emu = calloc(1, sizeof(Emulator));

    emu->cpu = calloc(1, sizeof(CPU));
    emu->mem = calloc(1, sizeof(Memory) + sizeof(BusDevice) * BUS_DEVICE_COUNT);
    emu->vce = calloc(1, sizeof(VCE));
    emu->vdc = calloc(1, sizeof(VDC));

    return emu;
}

void emu_destroy(Emulator* emu) {
    mem_free(emu->mem);
    free(emu->vdc);
    free(emu->vce);
    free(emu->cpu);

    free(emu);
}

void emu_reset(Emulator* emu, bool hard) {
#ifdef _CPU_DEBUG
    printf(FILEPOS "reset (hard=%u)\n", hard);
#endif

    vce_reset(emu->vce);
    vdc_reset(emu->vdc);

    cpu_reset(emu->cpu, emu->mem, _emu_cpu_sync, (void*)emu);

    emu->cycles = 0;
}

void emu_step(Emulator* emu) { cpu_step(emu->cpu, emu->mem, _emu_cpu_sync, (void*)emu); }
