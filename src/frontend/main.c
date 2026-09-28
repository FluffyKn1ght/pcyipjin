#include "pce/cpu.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include "utils/yield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const u8 TESTPROG[2] = {0x69, 0x81};

int main() {
    cpu_test(NULL, TESTPROG, 2,
             &(CPUTest){.check = (CPUTestChecks){.acc = true, .cycle_count = true},
                        .acc = 0x26,
                        .cycle_count = 2},
             true);
}
