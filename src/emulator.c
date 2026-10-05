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

    vce_tick(emu->vce, clocks, vdc_tick, (void*)emu->vdc);
}

Emulator* emu_create() {
    Emulator* emu = calloc(1, sizeof(Emulator));

    emu->cpu = calloc(1, sizeof(CPU));
    emu->vce = calloc(1, sizeof(VCE));
    emu->vdc = calloc(1, sizeof(VDC));

    emu->mem = calloc(1, sizeof(Memory) + sizeof(BusDevice) * BUS_DEVICE_COUNT);

    mem_attachdev(emu->mem, &(BusDevice){.userdata = emu->vce,
                                         .start_addr = MEM_HUC6260_START,
                                         .end_addr = MEM_HUC6260_END,
                                         .read = vce_read,
                                         .write = vce_write,
                                         .free = free});

    mem_attachdev(emu->mem, &(BusDevice){.userdata = emu->vdc,
                                         .start_addr = MEM_HUC6270_START,
                                         .end_addr = MEM_HUC6270_END,
                                         .read = vce_read,
                                         .write = vce_write,
                                         .free = free});

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
