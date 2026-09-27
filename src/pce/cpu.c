#include "pce/cpu.h"
#include "pce/memory.h"
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

#define LOAD_ZEROPAGE(zp_low)                                                                      \
    READ(u8 zp8, 0x2000 | (zp_low));                                                               \
    yield(0);

#define ZPX (0x2000 | cpu->x)

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
    s8 rel8 = (u8)zp8;
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
#define ADDR_ABSOLUTE_IND()                                                                        \
    ADDR_ZEROPAGE();                                                                               \
    u16 ind_addr = ind_addr_low | (ind_addr_high << 8);                                            \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
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

// TODO: affect cpu flags
#define INSTR_ADC(operand2)                                                                        \
    if (cpu->status.t) {                                                                           \
        READ(u8 zpx, ZPX);                                                                         \
        zpx += (operand2) + cpu->status.c;                                                         \
        yield(0);                                                                                  \
        WRITE(ZPX, zpx);                                                                           \
    } else {                                                                                       \
        cpu->acc += (operand2) + cpu->status.c;                                                    \
    }

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
