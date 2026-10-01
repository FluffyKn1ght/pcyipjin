#include "pce/cpu.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef enum : u8 {
    OPR_NONE,        /**< No operand */
    OPR_U8,          /**< Unsigned 8-bit operand (1 byte) */
    OPR_S8,          /**< Signed 8-bit operand (1 byte) */
    OPR_U8_U8,       /**< 2 unsigned 16-bit operands (2 bytes) */
    OPR_U16,         /**< Unsigned 16-bit operand (2 bytes) */
    OPR_U16_U16_U16, /**< 3 unsigned 16-bit operands (6 bytes) */
} OperandType;

#define VEC_RESET 0xFFFE
#define VEC_NMI 0xFFFC
#define VEC_TIMER 0xFFFA
#define VEC_IRQ1 0xFFF8
#define VEC_IRQ2 0xFFF6 // also BRK

#ifdef _CPU_DEBUG
#define DBGPRINT(msg, a, b) printf(FILEPOS msg "\n", (a), (b));
#else
#define DBGPRINT(msg, a, b)
#endif

#define SYNC() sync_func(sync_arg);

#define READ(dest, addr)                                                                           \
    SYNC();                                                                                        \
    DBGPRINT("Read from $%04X", (addr), 0);                                                        \
    dest = mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)));

#define WRITE(addr, value)                                                                         \
    SYNC();                                                                                        \
    DBGPRINT("Write to $%04X with value $%04X", (addr), (value));                                  \
    mem_write(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)), (value));

#define DUMMY_READ(addr)                                                                           \
    SYNC();                                                                                        \
    DBGPRINT("Dummy read from $%04X", (addr), 0);                                                  \
    mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)));

#define LOAD_ZEROPAGE(zp_low)                                                                      \
    DBGPRINT("LOAD_ZEROPAGE: Calculate zeropage address", 0, 0);                                   \
    SYNC();                                                                                        \
    u16 zp_physaddr = 0x2000 + ((zp_low) & 0xFF);                                                  \
    DBGPRINT("LOAD_ZEROPAGE: Read from zeropage", 0, 0);                                           \
    READ(u8 zp8, zp_physaddr);

#define READZPX(dest)                                                                              \
    DBGPRINT("READZPX: Calculate zeropage address", 0, 0);                                         \
    SYNC();                                                                                        \
    u8 zpx_addr = 0x2000 + cpu->x;                                                                 \
    DBGPRINT("READZPX: Read from zeropage", 0, 0);                                                 \
    READ(dest, zpx_addr);

#define WRITEZPX(value)                                                                            \
    u8 zpx_addr = 0x2000 + cpu->x;                                                                 \
    WRITE(zpx_addr, (value));

#define ADDR_IMPLIED_TMA()                                                                         \
    DBGPRINT("will now read mpr register id");                                                     \
    READ(TMAMPRReg mpr, cpu->pc++);
#define ADDR_IMPLIED_TAI()                                                                         \
    DBGPRINT("will now read TAI source (2 bytes)");                                                \
    READ(cpu->x, cpu->pc++);                                                                       \
    READ(cpu->sh, cpu->pc++);                                                                      \
    DBGPRINT("will now read TAI dest (2 bytes)");                                                  \
    READ(cpu->y, cpu->pc++);                                                                       \
    READ(cpu->dh, cpu->pc++);                                                                      \
    DBGPRINT("will now read TAI length (2 bytes)");                                                \
    READ(cpu->acc, cpu->pc++);                                                                     \
    READ(cpu->lh, cpu->pc++);
#define ADDR_IMMEDIATE()                                                                           \
    DBGPRINT("will now read imm8", 0, 0);                                                          \
    READ(u8 imm8, cpu->pc++);
#define ADDR_ZEROPAGE()                                                                            \
    DBGPRINT("will now read zp_low", 0, 0);                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);
#define ADDR_ZEROPAGE_X()                                                                          \
    DBGPRINT("will now read zp_low + x", 0, 0);                                                    \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->x);
#define ADDR_ZEROPAGE_Y()                                                                          \
    DBGPRINT("will now read zp_low + y", 0, 0);                                                    \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->y);
#define ADDR_ZEROPAGE_REL()                                                                        \
    DBGPRINT("will now read zp_low", 0, 0);                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);                                                                         \
    s8 rel8 = (s8)zp8;
#define ADDR_ZEROPAGE_IND()                                                                        \
    DBGPRINT("will now read zp_low", 0, 0);                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    DBGPRINT("calculate zeropage addr", 0, 0);                                                     \
    SYNC();                                                                                        \
    u16 zp_addr = 0x2000 | zp_low;                                                                 \
    DBGPRINT("will now read addr from zeropage (2 bytes)", 0, 0);                                  \
    SYNC();                                                                                        \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);                                                        \
    READ(u8 ind8, addr);
#define ADDR_ZEROPAGE_IND_X()                                                                      \
    DBGPRINT("will now read zp_low", 0, 0);                                                        \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    DBGPRINT("calculate zeropage addr (+ x)", 0, 0);                                               \
    SYNC();                                                                                        \
    u16 zp_addr = 0x2000 | (zp_low + cpu->x);                                                      \
    DBGPRINT("will read addr from zeropage (2 bytes)", 0, 0);                                      \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);                                                        \
    READ(u8 ind8, addr);
#define ADDR_ZEROPAGE_IND_Y()                                                                      \
    ADDR_ZEROPAGE_IND();                                                                           \
    addr += cpu->y;
#define ADDR_ABSOLUTE()                                                                            \
    DBGPRINT("will now read absolute addr (2 bytes)", 0, 0);                                       \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
    SYNC();                                                                                        \
    u16 addr = addr_low | (addr_high << 8);                                                        \
    READ(u8 abs8, addr);
#define ADDR_ABSOLUTE_X()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->x;
#define ADDR_ABSOLUTE_Y()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->y;
#define ADDR_ABSOLUTE_IND()                                                                        \
    ADDR_ABSOLUTE();                                                                               \
    DBGPRINT("will now read absolute addr (2 bytes)", 0, 0);                                       \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
    DBGPRINT("calculate indir address");                                                           \
    SYNC();                                                                                        \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ABSOLUTE_IND_X()                                                                      \
    ADDR_ABSOLUTE_IND();                                                                           \
    addr += cpu->x;
// todo: might need extra cycle
#define ADDR_RELATIVE()                                                                            \
    DBGPRINT("read relative", 0, 0);                                                               \
    READ(s8 offset, cpu->pc++);
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

#define SETZN(value)                                                                               \
    cpu->status.n = (value) & 0x80;                                                                \
    cpu->status.z = (value) == 0;

#define GET_OPERAND_A()                                                                            \
    if (cpu->status.t) {                                                                           \
        READZPX(operand_a);                                                                        \
    } else {                                                                                       \
        operand_a = cpu->acc;                                                                      \
    }

static u32 _phys_addr(CPU* cpu, u16 logic_addr) {
    // get MPR register number
    u8 mpr_idx = (logic_addr & 0xF000) >> 13;
    assert(mpr_idx >= 0 && mpr_idx <= 7);

    // convert physical memory block ID to physical address and return it
    return (logic_addr & 0x0FFF) + (cpu->mpr[mpr_idx] * 0x2000);
}

static void _alu_adc(CPU* cpu, Memory* mem, u8 operand_b, void (*sync_func)(void*),
                     void* sync_arg) {
    u8 operand_a;
    GET_OPERAND_A();

    u16 inter_result = operand_a + operand_b + cpu->status.c;
    u8 final_result = inter_result;
    if (cpu->status.d) {
        SYNC(); // waste extra cycle

        cpu->status.c = false;
        if ((inter_result & 0xF) > 0x9) {
            inter_result += 0x6;
            cpu->status.c = true;
        }

        if (cpu->status.c) {
            inter_result += 0x10;
        }

        if ((inter_result & 0xF0) > 0x90) {
            inter_result += 0x60;
            cpu->status.c = true;
        } else {
            cpu->status.c = false;
        }

        final_result = inter_result;
    } else {
        cpu->status.c = inter_result & 0xFF00;

        final_result = inter_result;

        if (final_result) {
            if ((s8)operand_a * (s8)operand_b <= -1) {
                cpu->status.v = false;
            } else {
                cpu->status.v = final_result & 0x80;

                if (operand_a < 0 && operand_b < 0) {
                    cpu->status.v = !cpu->status.v;
                }
            }
        } else {
            cpu->status.v = false;
        }
    }

    SETZN(final_result);

    if (cpu->status.t) {
        WRITEZPX(final_result);
    } else {
        cpu->acc = final_result;
    }
}

void cpu_reset(CPU* cpu, Memory* mem, void (*sync_func)(void*), void* sync_arg) {
    cpu->status.i = true;
    cpu->status.d = false;
    cpu->mpr[7] = 0;
    // TODO: Stop timer
    // TODO: Clear interrupt disable register in memory
    // TODO: Clear TIQ
    cpu->high_speed = false;
    // TODO: Output H to port O
    cpu->status.t = false;
    // TODO: something is said about "ready state being cleared"
    // TODO: SYNC pin goes low
    // TODO: system clock is output to SX pin
    // TODO: HSM pin goes low

    READ(u8 reset_routine_low, VEC_RESET);
    READ(u8 reset_routine_high, VEC_RESET + 1);
    cpu->pc = reset_routine_low | (reset_routine_high << 8);
    DBGPRINT("reset, jumped to $%04X", cpu->pc, 0);
}

void cpu_step(CPU* cpu, Memory* mem, void (*sync_func)(void*), void* sync_arg) {
    READ(u8 opcode, cpu->pc);
    cpu->pc++;

    switch (opcode) {
    case 0x69: { // adc #nn
        ADDR_IMMEDIATE();
        _alu_adc(cpu, mem, imm8, sync_func, sync_arg);
        break;
    }
    case 0x65: { // adc zz
        ADDR_ZEROPAGE();
        _alu_adc(cpu, mem, zp8, sync_func, sync_arg);
        break;
    }
    case 0x75: { // adc zz, x
        ADDR_ZEROPAGE_X();
        _alu_adc(cpu, mem, zp8, sync_func, sync_arg);
        break;
    }
    case 0x72: { // adc (zz)
        ADDR_ZEROPAGE_IND();
        _alu_adc(cpu, mem, ind8, sync_func, sync_arg);
        break;
    }
    case 0x61: { // adc (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_adc(cpu, mem, ind8, sync_func, sync_arg);
        break;
    }
    case 0x71: { // adc (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_adc(cpu, mem, ind8, sync_func, sync_arg);
        break;
    }
    case 0x6D: { // adc hell
        ADDR_ABSOLUTE();
        READ(u8 abs_value, addr);
        _alu_adc(cpu, mem, abs_value, sync_func, sync_arg);
        break;
    }
    case 0x7D: { // adc hhll, x
        ADDR_ABSOLUTE_X();
        _alu_adc(cpu, mem, abs8, sync_func, sync_arg);
        break;
    }
    case 0x79: { // adc hhll, y
        ADDR_ABSOLUTE_Y();
        _alu_adc(cpu, mem, abs8, sync_func, sync_arg);
        break;
    }

    case 0xE9: { // sbc #nn
        ADDR_IMMEDIATE();
        _alu_adc(cpu, mem, imm8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xE5: { // sbc zz
        ADDR_ZEROPAGE();
        _alu_adc(cpu, mem, zp8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xF5: { // sbc zz, x
        ADDR_ZEROPAGE_X();
        _alu_adc(cpu, mem, zp8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xF2: { // sbc (zz)
        ADDR_ZEROPAGE_IND();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xE1: { // sbc (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xF1: { // sbc (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xED: { // sbc hhll
        ADDR_ABSOLUTE();
        _alu_adc(cpu, mem, abs8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xFD: { // sbc hhll, x
        ADDR_ABSOLUTE_X();
        _alu_adc(cpu, mem, abs8 ^ 0xFF, sync_func, sync_arg);
        break;
    }
    case 0xF9: { // sbc hhll, y
        ADDR_ABSOLUTE_Y();
        _alu_adc(cpu, mem, abs8 ^ 0xFF, sync_func, sync_arg);
        break;
    }

    case 0xA9: { // lda #nn
        ADDR_IMMEDIATE();
        cpu->acc = imm8;
        SETZN(imm8);
        break;
    }
    case 0xA5: { // lda zz
        ADDR_ZEROPAGE();
        cpu->acc = zp8;
        SETZN(zp8);
        break;
    }
    case 0xB5: { // lda zz, x
        ADDR_ZEROPAGE_X();
        cpu->acc = zp8;
        SETZN(zp8);
        break;
    }
    case 0xB2: { // lda (zz)
        ADDR_ZEROPAGE_IND();
        cpu->acc = ind8;
        SETZN(ind8);
        break;
    }
    case 0xA1: { // lda (zz, x)
        ADDR_ZEROPAGE_IND_X();
        cpu->acc = ind8;
        SETZN(ind8);
        break;
    }
    case 0xB1: { // lda (zz), y
        ADDR_ZEROPAGE_IND_Y();
        cpu->acc = ind8;
        SETZN(ind8);
        break;
    }
    case 0xAD: { // lda hhll
        ADDR_ABSOLUTE();
        cpu->acc = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBD: { // lda hhll, x
        ADDR_ABSOLUTE_X();
        cpu->acc = abs8;
        SETZN(abs8);
        break;
    }
    case 0xB9: { // lda hhll, y
        ADDR_ABSOLUTE_Y();
        cpu->acc = abs8;
        SETZN(abs8);
        break;
    }

    case 0xA2: { // ldx #nn
        ADDR_IMMEDIATE();
        cpu->x = imm8;
        SETZN(imm8);
        break;
    }
    case 0xA6: { // ldx zz
        ADDR_ZEROPAGE();
        cpu->x = zp8;
        SETZN(zp8);
        break;
    }
    case 0xB6: { // ldx zz, y
        ADDR_ZEROPAGE_Y();
        cpu->x = zp8;
        SETZN(zp8);
        break;
    }
    case 0xAE: { // ldx hhll
        ADDR_ABSOLUTE();
        cpu->x = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBE: { // ldx hhll, y
        ADDR_ABSOLUTE_Y();
        cpu->x = abs8;
        SETZN(abs8);
        break;
    }

    case 0xA0: { // ldy #nn
        ADDR_IMMEDIATE();
        cpu->y = imm8;
        SETZN(imm8);
        break;
    }
    case 0xA4: { // ldy zz
        ADDR_ZEROPAGE();
        cpu->y = zp8;
        SETZN(zp8);
        break;
    }
    case 0xB4: { // ldy zz, x
        ADDR_ZEROPAGE_Y();
        cpu->y = zp8;
        SETZN(zp8);
        break;
    }
    case 0xAC: { // ldy hhll
        ADDR_ABSOLUTE();
        cpu->y = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBC: { // ldy hhll, x
        ADDR_ABSOLUTE_Y();
        cpu->y = abs8;
        SETZN(abs8);
        break;
    }

    default: {
        // TODO: not crash the entire program with abort()
        printf(FILEPOS "unknown opcode $%02x\n", opcode);
        abort();
        break;
    }
    }

    cpu->status.t = 0;
}
