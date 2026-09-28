#include "pce/cpu.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include "utils/yield.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum : u8 {
    TMA_MPR0 = 0x1,
    TMA_MPR1 = 0x2,
    TMA_MPR2 = 0x4,
    TMA_MPR3 = 0x8,
    TMA_MPR4 = 0x10,
    TMA_MPR5 = 0x20,
    TMA_MPR6 = 0x40,
    TMA_MPR7 = 0x80,
} TMAMPRReg;

#define VEC_RESET 0x1FFE
#define VEC_NMI 0xFFFC
#define VEC_TIMER 0xFFFA
#define VEC_IRQ1 0xFFF8
#define VEC_IRQ2 0xFFF6 // also BRK

#define READ(dest, addr)                                                                           \
    yield(0);                                                                                      \
    dest = mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)));

#define WRITE(addr, value)                                                                         \
    yield(0);                                                                                      \
    mem_write(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)), (value));

#define DUMMY_READ(addr)                                                                           \
    yield(0);                                                                                      \
    mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)));

#define LOAD_ZEROPAGE(zp_low)                                                                      \
    READ(u8 zp8, 0x2000 | (zp_low));                                                               \
    yield(0);

#define READZPX(dest)                                                                              \
    yield(0);                                                                                      \
    u8 zpx_addr = 0x2000 | cpu->x;                                                                 \
    READ(dest, zpx_addr);

#define ADDR_IMPLIED_TMA() READ(TMAMPRReg mpr, cpu->pc++);
#define ADDR_IMPLIED_TAI()                                                                         \
    READ(cpu->x, cpu->pc++);                                                                       \
    READ(cpu->sh, cpu->pc++);                                                                      \
    READ(cpu->y, cpu->pc++);                                                                       \
    READ(cpu->dh, cpu->pc++);                                                                      \
    READ(cpu->acc, cpu->pc++);                                                                     \
    READ(cpu->lh, cpu->pc++);
#define ADDR_IMMEDIATE() READ(u8 imm8, cpu->pc++);
#define ADDR_ZEROPAGE()                                                                            \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);
#define ADDR_ZEROPAGE_X()                                                                          \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->x);
#define ADDR_ZEROPAGE_Y()                                                                          \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->y);
#define ADDR_ZEROPAGE_REL()                                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);                                                                         \
    s8 rel8 = (s8)zp8;
#define ADDR_ZEROPAGE_IND()                                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    yield(0);                                                                                      \
    u16 zp_addr = 0x2000 | zp_low;                                                                 \
    yield(0);                                                                                      \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ZEROPAGE_IND_X()                                                                      \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    yield(0);                                                                                      \
    u16 zp_addr = 0x2000 | (zp_low + cpu->x);                                                      \
    yield(0);                                                                                      \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ZEROPAGE_IND_Y()                                                                      \
    ADDR_ZEROPAGE_IND();                                                                           \
    addr += cpu->y;
#define ADDR_ABSOLUTE()                                                                            \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
    yield(0);                                                                                      \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ABSOLUTE_X()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->x;
#define ADDR_ABSOLUTE_Y()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->y;
// todo: this is broken
#define ADDR_ABSOLUTE_IND()                                                                        \
    ADDR_ABSOLUTE();                                                                               \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
    yield(0);                                                                                      \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ABSOLUTE_IND_X()                                                                      \
    ADDR_ABSOLUTE_IND();                                                                           \
    addr += cpu->x;
#define ADDR_RELATIVE() READ(s8 offset, cpu->pc++);
#define ADDR_IMM_ZEROPAGE()                                                                        \
    ADDR_IMMEDIATE();                                                                              \
    ADDR_ZEROPAGE();
#define ADDR_IMM_ZEROPAGE_X()                                                                      \
    ADDR_IMMEDIATE();                                                                              \
    ADDR_ZEROPAGE_X();
#define ADDR_IMM_ABSOLUTE()                                                                        \
    ADDR_IMMEDIATE();                                                                              \
    ADDR_ABSOLUTE();
#define ADDR_IMM_ABSOLUTE_X()                                                                      \
    ADDR_IMMEDIATE();                                                                              \
    ADDR_ABSOLUTE_X();

#define INSTR_ADC(operand2)                                                                        \
    u8 old, new;                                                                                   \
    if (cpu->status.t) {                                                                           \
        READZPX(u8 zpx);                                                                           \
        old = zpx;                                                                                 \
        zpx += (operand2) + cpu->status.c;                                                         \
        new = zpx;                                                                                 \
        if (cpu->status.d) {                                                                       \
            yield(0);                                                                              \
            if ((zpx & 0xF) > 0x9) {                                                               \
                zpx += 0x6;                                                                        \
            }                                                                                      \
            if ((zpx & 0xF0) > 0x90) {                                                             \
                zpx += 0x60;                                                                       \
            }                                                                                      \
        }                                                                                          \
        WRITE(zpx_addr, zpx);                                                                      \
    } else {                                                                                       \
        old = cpu->acc;                                                                            \
        cpu->acc += (operand2) + cpu->status.c;                                                    \
        new = cpu->acc;                                                                            \
        if (cpu->status.d) {                                                                       \
            yield(0);                                                                              \
            if ((cpu->acc & 0xF) > 0x9) {                                                          \
                cpu->acc += 0x6;                                                                   \
            }                                                                                      \
            if ((cpu->acc & 0xF0) > 0x90) {                                                        \
                cpu->acc += 0x60;                                                                  \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    cpu->status.c = old > (operand2);                                                              \
    cpu->status.z = (operand2) == 0;                                                               \
    if (((s8)old < 0 && (s8)(operand2) >= 0) || ((s8)old >= 0 && (s8)(operand2) < 0)) {            \
        cpu->status.v = false;                                                                     \
    } else {                                                                                       \
        if ((s8)old >= 0 && (s8)(operand2) >= 0) {                                                 \
            cpu->status.v = new & 0x80;                                                            \
        } else {                                                                                   \
            cpu->status.v = !(new & 0x80);                                                         \
        }                                                                                          \
    }                                                                                              \
    cpu->status.n = new & 0x80;

static u32 _phys_addr(CPU* cpu, u16 logic_addr) {
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

coroutine cpu_step(void* ctxptr) {
    cpu_step_ctx* ctx = (cpu_step_ctx*)ctxptr;
    CPU* cpu = ctx->cpu;
    Memory* mem = ctx->mem;

    u8 opcode = mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, cpu->pc++));

    switch (opcode) {
    case 0x69: { // adc #xx
        ADDR_IMMEDIATE();
        INSTR_ADC(imm8);
        break;
    }
    case 0x65: { // adc zz
        ADDR_ZEROPAGE();
        INSTR_ADC(zp8);
        break;
    }
    case 0x75: { // adc zz, x
        ADDR_ZEROPAGE_X();
        INSTR_ADC(zp8);
        break;
    }
    case 0x72: { // adc (zz)
        ADDR_ZEROPAGE_IND();
        READ(u8 ind_value, addr);
        INSTR_ADC(ind_value);
        break;
    }
    case 0x61: { // adc (zz, x)
        ADDR_ZEROPAGE_IND_X();
        READ(u8 ind_value, addr);
        INSTR_ADC(ind_value);
        break;
    }
    case 0x71: { // adc (zz), y
        ADDR_ZEROPAGE_IND_Y();
        READ(u8 ind_value, addr);
        INSTR_ADC(ind_value);
        break;
    }
    case 0x6D: { // adc hell
        ADDR_ABSOLUTE();
        READ(u8 abs_value, addr);
        INSTR_ADC(abs_value);
        break;
    }
    case 0x7D: { // adc hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs_value, addr);
        INSTR_ADC(abs_value);
        break;
    }
    case 0x79: { // adc hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs_value, addr);
        INSTR_ADC(abs_value);
        break;
    }
    default: {
        printf("unknown opcode\n");
        abort();
        break;
    }
    }

    stop();
}

bool cpu_test(CPU* cpu, const u8* program, u32 program_size, CPUTest* test, bool log_cycles) {
    void* membuf = malloc(0x200000);
    memcpy(membuf, program, program_size);

    Memory* mem = calloc(1, sizeof(Memory));
    mem_attachdev(mem, &(BusDevice){.start_addr = 0,
                                    .end_addr = 0x1FFFFF,
                                    .read = testmem_read,
                                    .write = testmem_write,
                                    .userdata = membuf});

    if (!cpu) {
        cpu = calloc(1, sizeof(CPU));
        cpu_reset(cpu);
    }

    printf("\n==================== [ STARTING TEST ] ====================\n\n");

    int cycles = 0;
    coroutine step_coro = NULL;
    do {
        cycles++;
        step_coro = coro_call(step_coro, cpu_step, &(cpu_step_ctx){cpu, mem});

        if (log_cycles) {
            printf(PRINT_FILEPOS "cycle %d\n", cycles);
        }
    } while (step_coro);
    coro_free(step_coro);

    if (log_cycles) {
        printf(PRINT_FILEPOS "TOTAL CYCLES TAKEN: %d\n", cycles);
    }

    bool test_ok = true;

    if (cpu->acc != test->acc && test->check.acc) {
        printf(PRINT_FILEPOS "bad ACC: expected $%X vs $%X\n", test->acc, cpu->acc);
        test_ok = false;
    }

    if (cpu->x != test->x && test->check.x) {
        printf(PRINT_FILEPOS "bad X: expected $%X vs $%X\n", test->x, cpu->x);
        test_ok = false;
    }

    if (cpu->y != test->y && test->check.y) {
        printf(PRINT_FILEPOS "bad X: expected $%X vs $%X\n", test->y, cpu->y);
        test_ok = false;
    }

    if (cpu->sp != test->sp && test->check.sp) {
        printf(PRINT_FILEPOS "bad SP: expected $%X vs $%X\n", test->sp, cpu->sp);
        test_ok = false;
    }

    if (cpu->pc != test->pc && test->check.pc) {
        printf(PRINT_FILEPOS "bad PC: expected $%X vs $%X\n", test->pc, cpu->pc);
        test_ok = false;
    }

    if (cycles != test->cycle_count && test->check.cycle_count) {
        printf(PRINT_FILEPOS "bad cycle count: expected %u vs %u\n", test->cycle_count, cycles);
        test_ok = false;
    }

    if (cpu->p != test->p && test->check.flags) {
        printf(PRINT_FILEPOS "bad status/flags: expected %%%b vs %%%b [NVTBDIZC]\n", test->p,
               cpu->p);
        test_ok = false;
    }

    if (!*(u8*)(&test->check)) {
        printf(PRINT_FILEPOS "BAD TEST: no checks enabled\n");
        test_ok = false;
    }

    printf("\n==================== [ %s ] ====================\n\n",
           test_ok ? "Passed! :3" : "FAILED :<");

    free(cpu);
    free(mem);
    free(membuf);

    return test_ok;
}
