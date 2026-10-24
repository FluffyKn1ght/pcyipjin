#include "memory.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PCEMemory* pce_mem_create() {
    return calloc(1, sizeof(PCEMemory) + sizeof(PCEBusDevice) * MEM_BUS_DEVICE_COUNT);
}

void pce_mem_attachdev(PCEMemory* mem, PCEBusDevice* device) {
    mem->device_count++;
    memcpy(mem->devices + (mem->device_count - 1), device, sizeof(PCEBusDevice));
}

u8 pce_mem_read(PCEMemory* mem, u32 addr) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            if (mem->devices[i].read) {
                return mem->devices[i].read(mem->devices[i].userdata, addr & PHYSADDR_MASK);
            }

            return MEM_UNDEFINED;
        }
    }

    return MEM_UNDEFINED;
}

void pce_mem_write(PCEMemory* mem, u32 addr, u8 value) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            if (mem->devices[i].write) {
                mem->devices[i].write(mem->devices[i].userdata, addr & PHYSADDR_MASK, value);
            }
            return;
        }
    }
}

void pce_mem_free(PCEMemory* mem) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices->free) {
            mem->devices[i].free(mem->devices + i);
        }
    }

    free(mem);
}
