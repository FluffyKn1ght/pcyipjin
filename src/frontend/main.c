#include "pce/cpu.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include "utils/yield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* MEM_DATA = "\x69\x26";

int main() {
    void* membuf = calloc(1, 0x1FFFFF);
    memcpy(membuf, MEM_DATA, strlen(MEM_DATA));

    Memory* mem = calloc(1, sizeof(Memory));
    mem_attachdev(mem, &(BusDevice){.start_addr = 0,
                                    .end_addr = 0x1FFFFF,
                                    .read = testmem_read,
                                    .write = testmem_write,
                                    .userdata = membuf});

    CPU* cpu = calloc(1, sizeof(CPU));
    cpu_reset(cpu);

    int cycles = 0;
    coroutine cputest = NULL;
    do {
        cycles++;
        cputest = coro_call(cputest, cpu_step, &(cpu_step_ctx){cpu, mem});
        printf("cycle %d\n", cycles);
    } while (cputest);
    printf("final cycle %d\n", cycles);
    printf("a is %d\n", cpu->acc);
    coro_free(cputest);
}
