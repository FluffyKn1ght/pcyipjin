#include "pce/cpu.h"
#include "callbacks.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include <assert.h>
#include <signal.h>
#include <stdcountof.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const u8 MPR_TMA_2I_VALUES[8] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

#define VEC_RESET 0xFFFE
#define VEC_NMI 0xFFFC
#define VEC_TIMER 0xFFFA
#define VEC_IRQ1 0xFFF8
#define VEC_IRQ2 0xFFF6 // also BRK

#ifdef _CPU_DEBUG
#define DBGPRINT(msg, ...) printf(FILEPOS msg "\n" __VA_OPT__(, ) __VA_ARGS__);
#else
#define DBGPRINT(msg, ...)
#endif

#define SYNC() ec->cpu_sync(ec->arg0);

#define READ_(dest, addr, id)                                                                      \
    SYNC();                                                                                        \
    u16 CONCAT(_read_temp_addr, id) = (addr);                                                      \
    DBGPRINT("Read from $%04X", CONCAT(_read_temp_addr, id));                                      \
    dest = mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, CONCAT(_read_temp_addr, id)));

#define READ(dest, addr) READ_(dest, addr, __COUNTER__);

#define WRITE(addr, value)                                                                         \
    SYNC();                                                                                        \
    DBGPRINT("Write to $%04X with value $%02X", (addr), (value));                                  \
    mem_write(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)), (value));

#define DUMMY_READ(addr)                                                                           \
    SYNC();                                                                                        \
    DBGPRINT("Dummy read from $%04X", (addr));                                                     \
    mem_read(mem, MEMACCESS_CPU, _phys_addr(cpu, (addr)));

#define CALC_ZP_ADDR(dest, low)                                                                    \
    SYNC();                                                                                        \
    dest = 0x2000 | ((low) & 0xFF);

#define LOAD_ZEROPAGE(zp_low)                                                                      \
    CALC_ZP_ADDR(u16 zp_addr, zp_low);                                                             \
    DBGPRINT("LOAD_ZEROPAGE: Read from zeropage");                                                 \
    READ(u8 zp8, zp_addr);

#define READZPX(dest)                                                                              \
    DBGPRINT("READZPX: Calculate zeropage address");                                               \
    SYNC();                                                                                        \
    u8 zpx_addr = 0x2000 + cpu->x;                                                                 \
    DBGPRINT("READZPX: Read from zeropage");                                                       \
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
    DBGPRINT("will now read imm8");                                                                \
    READ(u8 imm8, cpu->pc++);
#define ADDR_ZEROPAGE()                                                                            \
    DBGPRINT("will now read zp_low");                                                              \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);
#define ADDR_ZEROPAGE_X()                                                                          \
    DBGPRINT("will now read zp_low + x");                                                          \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->x);
#define ADDR_ZEROPAGE_Y()                                                                          \
    DBGPRINT("will now read zp_low + y");                                                          \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low + cpu->y);
#define ADDR_ZEROPAGE_REL()                                                                        \
    DBGPRINT("will now read zp_low");                                                              \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    LOAD_ZEROPAGE(zp_low);                                                                         \
    s8 rel8 = (s8)zp8;
#define ADDR_ZEROPAGE_IND()                                                                        \
    DBGPRINT("will now read zp_low");                                                              \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    CALC_ZP_ADDR(u16 zp_addr, zp_low);                                                             \
    DBGPRINT("will now read addr from zeropage (2 bytes)");                                        \
    SYNC();                                                                                        \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);                                                        \
    READ(u8 ind8, addr);
#define ADDR_ZEROPAGE_IND_X()                                                                      \
    DBGPRINT("will now read zp_low");                                                              \
    READ(u8 zp_low, cpu->pc++);                                                                    \
    CALC_ZP_ADDR(u16 zp_addr, zp_low + cpu->x);                                                    \
    DBGPRINT("will read addr from zeropage (2 bytes)");                                            \
    READ(u8 addr_low, zp_addr);                                                                    \
    READ(u8 addr_high, zp_addr + 1);                                                               \
    u16 addr = addr_low | (addr_high << 8);                                                        \
    READ(u8 ind8, addr);
#define ADDR_ZEROPAGE_IND_Y()                                                                      \
    ADDR_ZEROPAGE_IND();                                                                           \
    addr += cpu->y;
#define ADDR_ABSOLUTE()                                                                            \
    DBGPRINT("will now read absolute addr (2 bytes)");                                             \
    READ(u8 addr_low, cpu->pc++);                                                                  \
    READ(u8 addr_high, cpu->pc++);                                                                 \
    SYNC();                                                                                        \
    u16 addr = addr_low | (addr_high << 8);
#define ADDR_ABSOLUTE_X()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->x;
#define ADDR_ABSOLUTE_Y()                                                                          \
    ADDR_ABSOLUTE();                                                                               \
    addr += cpu->y;
#define ADDR_ABSOLUTE_IND()                                                                        \
    ADDR_ABSOLUTE();                                                                               \
    DBGPRINT("will now read indir addr (2 bytes)");                                                \
    READ(u8 ind_addr_low, addr);                                                                   \
    READ(u8 ind_addr_high, addr + 1);                                                              \
    DBGPRINT("calculate indir address");                                                           \
    SYNC();                                                                                        \
    u16 ind_addr = ind_addr_low | (ind_addr_high << 8);
#define ADDR_ABSOLUTE_IND_X()                                                                      \
    ADDR_ABSOLUTE();                                                                               \
    DBGPRINT("will now read indir addr (2 bytes)");                                                \
    READ(u8 ind_addr_low, addr + cpu->x);                                                          \
    READ(u8 ind_addr_high, addr + cpu->x + 1);                                                     \
    DBGPRINT("calculate indir address");                                                           \
    SYNC();                                                                                        \
    u16 ind_addr = ind_addr_low | (ind_addr_high << 8);
#define ADDR_RELATIVE()                                                                            \
    DBGPRINT("will read relative");                                                                \
    READ(s8 rel8, cpu->pc++);
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

#define ALU_GET_OPRERAND_A()                                                                       \
    if (cpu->status.t) {                                                                           \
        READZPX(operand_a);                                                                        \
    } else {                                                                                       \
        operand_a = cpu->acc;                                                                      \
    }

#define ALU_SET_RESULT(result)                                                                     \
    if (!cpu->_alu_discard) {                                                                      \
        if (cpu->status.t) {                                                                       \
            WRITEZPX(result);                                                                      \
        } else {                                                                                   \
            cpu->acc = (result);                                                                   \
        }                                                                                          \
    }

#define STACK_PUSH(what)                                                                           \
    DBGPRINT("push $%0X2 to stack, sp=$%02X=>$%02X", what, cpu->sp, cpu->sp - 1);                  \
    WRITE(0x2100 | cpu->sp, (what));                                                               \
    SYNC();                                                                                        \
    cpu->sp--;

#define STACK_PULL(dest)                                                                           \
    DBGPRINT("pull from stack, sp=$%02X=>$%02X", cpu->sp, cpu->sp + 1);                            \
    SYNC();                                                                                        \
    cpu->sp++;                                                                                     \
    READ(dest, 0x2100 | cpu->sp);

#define STACK_PUSHPC()                                                                             \
    STACK_PUSH((cpu->pc & 0xFF00) >> 8);                                                           \
    STACK_PUSH(cpu->pc & 0xFF);

#define STACK_PULLPC()                                                                             \
    STACK_PULL(u8 pc_low);                                                                         \
    STACK_PULL(u8 pc_high);                                                                        \
    cpu->pc = pc_low | (pc_high << 8);

#define SWAPREGS(r1, r2)                                                                           \
    u8 reg1 = (r1);                                                                                \
    u8 reg2 = (r2);                                                                                \
    SYNC();                                                                                        \
    (r1) = reg2;                                                                                   \
    SYNC();                                                                                        \
    (r2) = reg1;

#define BRANCH(cond)                                                                               \
    ADDR_RELATIVE();                                                                               \
    SYNC();                                                                                        \
    if (cond) {                                                                                    \
        SYNC();                                                                                    \
        cpu->pc += rel8;                                                                           \
    }

static inline u8 _mpr_tma_2i_to_idx(u8 tma_2i) {
    for (int idx = 0; idx < countof(MPR_TMA_2I_VALUES); idx++) {
        if (MPR_TMA_2I_VALUES[idx] == tma_2i) {
            return idx;
        }
    }

    assert(false);
}

static u32 _phys_addr(CPU* cpu, u16 logic_addr) {
    // get MPR register number
    u8 mpr_idx = (logic_addr & 0xF000) >> 13;
    assert(mpr_idx >= 0 && mpr_idx <= 7);

    // convert physical memory block ID to physical address and return it
    return (logic_addr & 0x0FFF) + (cpu->mpr[mpr_idx] * 0x2000);
}

static void _alu_adc(CPU* cpu, Memory* mem, u8 operand_b, EmuCallbacks* ec) {
    u8 operand_a;
    ALU_GET_OPRERAND_A();

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

    ALU_SET_RESULT(final_result);
}

static void _alu_and(CPU* cpu, Memory* mem, u8 operand_b, EmuCallbacks* ec) {
    u8 operand_a;
    ALU_GET_OPRERAND_A();

    u8 result = operand_a & operand_b;

    SETZN(result);

    ALU_SET_RESULT(result);
}

static void _alu_ora(CPU* cpu, Memory* mem, u8 operand_b, EmuCallbacks* ec) {
    u8 operand_a;
    ALU_GET_OPRERAND_A();

    u8 result = operand_a | operand_b;

    SETZN(result);

    ALU_SET_RESULT(result);
}

static void _alu_eor(CPU* cpu, Memory* mem, u8 operand_b, EmuCallbacks* ec) {
    u8 operand_a;
    ALU_GET_OPRERAND_A();

    u8 result = operand_a ^ operand_b;

    SETZN(result);

    ALU_SET_RESULT(result);
}

static u8 _alu_asl(CPU* cpu, Memory* mem, u8 operand, EmuCallbacks* ec) {
    cpu->status.c = operand & 0x80;
    u8 result = operand << 1;

    SETZN(result);

    return result;
}

static u8 _alu_lsr(CPU* cpu, Memory* mem, u8 operand, EmuCallbacks* ec) {
    cpu->status.c = operand & 0x1;
    u8 result = operand >> 1;

    SETZN(result);

    return result;
}

static u8 _alu_rol(CPU* cpu, Memory* mem, u8 operand, EmuCallbacks* ec) {
    bool new_carry = operand & 0x80;
    u8 result = operand << 1;
    result |= cpu->status.c;
    cpu->status.c = new_carry;

    SETZN(result);

    return result;
}

static u8 _alu_ror(CPU* cpu, Memory* mem, u8 operand, EmuCallbacks* ec) {
    bool new_carry = operand & 0x1;
    u8 result = operand >> 1;
    result |= cpu->status.c << 7;
    cpu->status.c = new_carry;

    SETZN(result);

    return result;
}

void cpu_reset(CPU* cpu, Memory* mem, EmuCallbacks* ec) {
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
    // TODO: fill registers with garbage

    READ(u8 reset_routine_low, VEC_RESET);
    READ(u8 reset_routine_high, VEC_RESET + 1);
    cpu->pc = reset_routine_low | (reset_routine_high << 8);
    DBGPRINT("reset, jumped to $%04X", cpu->pc);
}

void cpu_step(CPU* cpu, Memory* mem, EmuCallbacks* ec) {
    READ(u8 opcode, cpu->pc);
    cpu->pc++;

    switch (opcode) {
    case 0x69: { // adc #nn
        ADDR_IMMEDIATE();
        _alu_adc(cpu, mem, imm8, ec);
        break;
    }
    case 0x65: { // adc zz
        ADDR_ZEROPAGE();
        _alu_adc(cpu, mem, zp8, ec);
        break;
    }
    case 0x75: { // adc zz, x
        ADDR_ZEROPAGE_X();
        _alu_adc(cpu, mem, zp8, ec);
        break;
    }
    case 0x72: { // adc (zz)
        ADDR_ZEROPAGE_IND();
        _alu_adc(cpu, mem, ind8, ec);
        break;
    }
    case 0x61: { // adc (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_adc(cpu, mem, ind8, ec);
        break;
    }
    case 0x71: { // adc (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_adc(cpu, mem, ind8, ec);
        break;
    }
    case 0x6D: { // adc hell
        ADDR_ABSOLUTE();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8, ec);
        break;
    }
    case 0x7D: { // adc hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8, ec);
        break;
    }
    case 0x79: { // adc hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8, ec);
        break;
    }

    case 0x29: { // and #nn
        ADDR_IMMEDIATE();
        _alu_and(cpu, mem, imm8, ec);
        break;
    }
    case 0x25: { // and zz
        ADDR_ZEROPAGE();
        _alu_and(cpu, mem, zp8, ec);
        break;
    }
    case 0x35: { // and zz, x
        ADDR_ZEROPAGE_X();
        _alu_and(cpu, mem, zp8, ec);
        break;
    }
    case 0x32: { // and (zz)
        ADDR_ZEROPAGE_IND();
        _alu_and(cpu, mem, ind8, ec);
        break;
    }
    case 0x21: { // and (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_and(cpu, mem, ind8, ec);
        break;
    }
    case 0x31: { // and (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_and(cpu, mem, ind8, ec);
        break;
    }
    case 0x2D: { // and hell
        ADDR_ABSOLUTE();
        READ(u8 abs8, addr);
        _alu_and(cpu, mem, abs8, ec);
        break;
    }
    case 0x3D: { // and hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        _alu_and(cpu, mem, abs8, ec);
        break;
    }
    case 0x39: { // and hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        _alu_and(cpu, mem, abs8, ec);
        break;
    }

    case 0x49: { // eor #nn
        ADDR_IMMEDIATE();
        _alu_eor(cpu, mem, imm8, ec);
        break;
    }
    case 0x45: { // eor zz
        ADDR_ZEROPAGE();
        _alu_eor(cpu, mem, zp8, ec);
        break;
    }
    case 0x55: { // eor zz, x
        ADDR_ZEROPAGE_X();
        _alu_eor(cpu, mem, zp8, ec);
        break;
    }
    case 0x52: { // eor (zz)
        ADDR_ZEROPAGE_IND();
        _alu_eor(cpu, mem, ind8, ec);
        break;
    }
    case 0x41: { // eor (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_eor(cpu, mem, ind8, ec);
        break;
    }
    case 0x51: { // eor (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_eor(cpu, mem, ind8, ec);
        break;
    }
    case 0x4D: { // eor hell
        ADDR_ABSOLUTE();
        READ(u8 abs8, addr);
        _alu_eor(cpu, mem, abs8, ec);
        break;
    }
    case 0x5D: { // eor hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        _alu_eor(cpu, mem, abs8, ec);
        break;
    }
    case 0x59: { // eor hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        _alu_eor(cpu, mem, abs8, ec);
        break;
    }

    case 0x09: { // ora #nn
        ADDR_IMMEDIATE();
        _alu_ora(cpu, mem, imm8, ec);
        break;
    }
    case 0x05: { // ora zz
        ADDR_ZEROPAGE();
        _alu_ora(cpu, mem, zp8, ec);
        break;
    }
    case 0x15: { // ora zz, x
        ADDR_ZEROPAGE_X();
        _alu_ora(cpu, mem, zp8, ec);
        break;
    }
    case 0x12: { // ora (zz)
        ADDR_ZEROPAGE_IND();
        _alu_ora(cpu, mem, ind8, ec);
        break;
    }
    case 0x01: { // ora (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_ora(cpu, mem, ind8, ec);
        break;
    }
    case 0x11: { // ora (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_ora(cpu, mem, ind8, ec);
        break;
    }
    case 0x0D: { // ora hell
        ADDR_ABSOLUTE();
        READ(u8 abs8, addr);
        _alu_ora(cpu, mem, abs8, ec);
        break;
    }
    case 0x1D: { // ora hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        _alu_ora(cpu, mem, abs8, ec);
        break;
    }
    case 0x19: { // ora hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        _alu_ora(cpu, mem, abs8, ec);
        break;
    }

    case 0xE9: { // sbc #nn
        ADDR_IMMEDIATE();
        _alu_adc(cpu, mem, imm8 ^ 0xFF, ec);
        break;
    }
    case 0xE5: { // sbc zz
        ADDR_ZEROPAGE();
        _alu_adc(cpu, mem, zp8 ^ 0xFF, ec);
        break;
    }
    case 0xF5: { // sbc zz, x
        ADDR_ZEROPAGE_X();
        _alu_adc(cpu, mem, zp8 ^ 0xFF, ec);
        break;
    }
    case 0xF2: { // sbc (zz)
        ADDR_ZEROPAGE_IND();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, ec);
        break;
    }
    case 0xE1: { // sbc (zz, x)
        ADDR_ZEROPAGE_IND_X();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, ec);
        break;
    }
    case 0xF1: { // sbc (zz), y
        ADDR_ZEROPAGE_IND_Y();
        _alu_adc(cpu, mem, ind8 ^ 0xFF, ec);
        break;
    }
    case 0xED: { // sbc hhll
        ADDR_ABSOLUTE();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8 ^ 0xFF, ec);
        break;
    }
    case 0xFD: { // sbc hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8 ^ 0xFF, ec);
        break;
    }
    case 0xF9: { // sbc hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        _alu_adc(cpu, mem, abs8 ^ 0xFF, ec);
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
        READ(u8 abs8, addr);
        cpu->acc = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBD: { // lda hhll, x
        ADDR_ABSOLUTE_X();
        READ(u8 abs8, addr);
        cpu->acc = abs8;
        SETZN(abs8);
        break;
    }
    case 0xB9: { // lda hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
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
        READ(u8 abs8, addr);
        cpu->x = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBE: { // ldx hhll, y
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
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
        READ(u8 abs8, addr);
        cpu->y = abs8;
        SETZN(abs8);
        break;
    }
    case 0xBC: { // ldy hhll, x
        ADDR_ABSOLUTE_Y();
        READ(u8 abs8, addr);
        cpu->y = abs8;
        SETZN(abs8);
        break;
    }

    case 0x85: { // sta zz
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8);
        WRITE(addr, cpu->acc);
        break;
    }
    case 0x95: { // sta zz, x
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8 + cpu->x);
        WRITE(addr, cpu->acc);
        break;
    }
    case 0x92: { // sta (zz)
        ADDR_ZEROPAGE();
        SYNC();
        CALC_ZP_ADDR(u16 addr, zp8);
        WRITE(zp_addr, cpu->acc);
        break;
    }
    case 0x81: { // sta (zz, x)
        ADDR_ZEROPAGE_X();
        SYNC();
        CALC_ZP_ADDR(u16 addr, zp8);
        WRITE(zp_addr, cpu->acc);
        break;
    }
    case 0x91: { // sta (zz), y
        ADDR_ZEROPAGE();
        SYNC();
        CALC_ZP_ADDR(u16 addr, zp8 + cpu->y);
        WRITE(zp_addr, cpu->acc);
        break;
    }
    case 0x8D: { // sta hhll
        ADDR_ABSOLUTE();
        WRITE(addr, cpu->acc);
        break;
    }
    case 0x9D: { // sta hhll, x
        ADDR_ABSOLUTE_X();
        WRITE(addr, cpu->acc);
        break;
    }
    case 0x99: { // sta hhll, y
        ADDR_ABSOLUTE_Y();
        WRITE(addr, cpu->acc);
        break;
    }

    case 0x86: { // stx zz
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8);
        WRITE(addr, cpu->x);
        break;
    }
    case 0x96: { // stx zz, y
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8 + cpu->y);
        WRITE(addr, cpu->x);
        break;
    }
    case 0x8E: { // stx hhll
        ADDR_ABSOLUTE();
        WRITE(addr, cpu->x);
        break;
    }

    case 0x84: { // sty zz
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8);
        WRITE(addr, cpu->y);
        break;
    }
    case 0x94: { // sty zz, x
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 addr, imm8 + cpu->x);
        WRITE(addr, cpu->y);
        break;
    }
    case 0x8C: { // sty hhll
        ADDR_ABSOLUTE();
        WRITE(addr, cpu->y);
        break;
    }

    case 0x64: { // stz zz
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 zp_addr, imm8);
        WRITE(zp_addr, 0);
        break;
    }
    case 0x74: { // stz zz, x
        ADDR_IMMEDIATE();
        CALC_ZP_ADDR(u16 zp_addr, imm8 + cpu->x);
        WRITE(zp_addr, 0);
        break;
    }
    case 0x9C: { // stz hhll
        ADDR_ABSOLUTE();
        WRITE(addr, 0);
        break;
    }
    case 0x9E: { // stz hhll, x
        ADDR_ABSOLUTE_X();
        WRITE(addr, 0);
        break;
    }

    case 0x02: { // sxy
        SWAPREGS(cpu->x, cpu->y);
        break;
    }
    case 0x22: { // sax
        SWAPREGS(cpu->acc, cpu->x);
        break;
    }
    case 0x42: { // say
        SWAPREGS(cpu->acc, cpu->y);
        break;
    }

    case 0x03: { // st0
        ADDR_IMMEDIATE();
        SYNC();
        SYNC();
        ec->vdc_write(ec->arg0, 0, imm8);
        break;
    }
    case 0x13: { // st1
        ADDR_IMMEDIATE();
        SYNC();
        SYNC();
        ec->vdc_write(ec->arg0, 1, imm8);
        break;
    }
    case 0x23: { // st2
        ADDR_IMMEDIATE();
        SYNC();
        SYNC();
        ec->vdc_write(ec->arg0, 2, imm8);
        break;
    }

    case 0xAA: { // tax
        cpu->x = cpu->acc;
        SYNC();
        SETZN(cpu->acc);
        break;
    }
    case 0xA8: { // tay
        cpu->y = cpu->acc;
        SYNC();
        SETZN(cpu->acc);
        break;
    }

    case 0x8A: { // txa
        cpu->acc = cpu->x;
        SYNC();
        SETZN(cpu->x);
        break;
    }
    case 0x9A: { // txs
        cpu->sp = cpu->x;
        SYNC();
        SETZN(cpu->x);
        break;
    }
    case 0xBA: { // tsx
        cpu->x = cpu->sp;
        SYNC();
        SETZN(cpu->sp);
        break;
    }
    case 0x98: { // tya
        cpu->acc = cpu->y;
        SYNC();
        SETZN(cpu->y);
        break;
    }

    case 0x43: { // tmai
        ADDR_IMMEDIATE();
        SYNC();
        u8 mpr_idx = _mpr_tma_2i_to_idx(imm8);
        SYNC();
        cpu->acc = cpu->mpr[mpr_idx];
        break;
    }

    case 0x53: { // tami
        ADDR_IMMEDIATE();
        SYNC();
        u8 mpr_idx = _mpr_tma_2i_to_idx(imm8);
        SYNC();
        cpu->mpr[mpr_idx] = cpu->acc;
        break;
    }

    case 0x48: { // pha
        STACK_PUSH(cpu->acc);
        break;
    }
    case 0x08: { // php
        STACK_PUSH(cpu->p);
        break;
    }
    case 0xDA: { // phx
        STACK_PUSH(cpu->x);
        break;
    }
    case 0x5A: { // phy
        STACK_PUSH(cpu->y);
        break;
    }

    case 0x68: { // pla
        STACK_PULL(cpu->acc);
        SYNC();
        break;
    }
    case 0x28: { // plp
        STACK_PULL(cpu->p);
        SYNC();
        break;
    }
    case 0xFA: { // plx
        STACK_PULL(cpu->x);
        SYNC();
        break;
    }
    case 0x7A: { // ply
        STACK_PULL(cpu->y);
        SYNC();
        break;
    }

    case 0x90: { // bcc rr
        BRANCH(!cpu->status.c);
        break;
    }
    case 0xB0: { // bcs rr
        BRANCH(cpu->status.c);
        break;
    }

    case 0xD0: { // bne rr
        BRANCH(!cpu->status.z);
        break;
    }
    case 0xF0: { // beq rr
        BRANCH(cpu->status.z);
        break;
    }

    case 0x30: { // bmi rr
        BRANCH(!cpu->status.n);
        break;
    }
    case 0x10: { // bpl rr
        BRANCH(cpu->status.n);
        break;
    }

    case 0x80: { // bra rr
        BRANCH(true);
        break;
    }

    case 0x50: { // bvc rr
        BRANCH(!cpu->status.v);
        break;
    }
    case 0x70: { // bvs rr
        BRANCH(cpu->status.v);
        break;
    }

    case 0x44: { // bsr rr
        ADDR_RELATIVE();
        SYNC();
        STACK_PUSHPC();
        SYNC();
        cpu->pc += rel8;
        break;
    }

    case 0x4C: { // jmp hhll
        ADDR_ABSOLUTE();
        cpu->pc = addr;
        break;
    }

    case 0x6C: { // jmp (hhll)
        ADDR_ABSOLUTE_IND();
        SYNC();
        cpu->pc = ind_addr;
        break;
    }

    case 0x7C: { // jmp (hhll, x)
        ADDR_ABSOLUTE_IND_X();
        SYNC();
        cpu->pc = ind_addr;
        break;
    }

    case 0x20: { // jsr hhll
        READ(u8 pc_low, cpu->pc);
        STACK_PUSHPC();
        READ(u8 pc_high, cpu->pc);
        cpu->pc = pc_low | (pc_high << 8);
        break;
    }

    case 0x40: { // rti
        STACK_PULL(cpu->p);
        STACK_PULLPC();
        break;
    }

    case 0x60: { // rts
        STACK_PULLPC();
        SYNC();
        cpu->pc++;
        SYNC();
        break;
    }

    case 0x18: { // clc
        SYNC();
        cpu->status.c = false;
        break;
    }
    case 0xD8: { // cld
        SYNC();
        cpu->status.d = false;
        break;
    }
    case 0x58: { // cli
        SYNC();
        cpu->status.i = false;
        break;
    }
    case 0xB8: { // clv
        SYNC();
        cpu->status.v = false;
        break;
    }

    case 0x38: { // sec
        SYNC();
        cpu->status.c = true;
        break;
    }
    case 0xF8: { // sed
        SYNC();
        cpu->status.d = true;
        break;
    }
    case 0x78: { // sei
        SYNC();
        cpu->status.i = true;
        break;
    }
    case 0xF4: { // set
        SYNC();
        cpu->status.t = true;
        break;
    }

    default: {
        if ((opcode <= 0x7F) && ((opcode & 0xF) == 0xF)) { // bbri zz, rr
            u8 bit = opcode >> 4;

            ADDR_IMMEDIATE();
            LOAD_ZEROPAGE(imm8);

            BRANCH(!(zp8 & (0b1 << bit)));
        } else if ((opcode >= 0x8F) && ((opcode & 0xF) == 0xF)) { // bbsi zz, rr
            u8 bit = opcode >> 4;

            ADDR_IMMEDIATE();
            LOAD_ZEROPAGE(imm8);

            BRANCH(zp8 & (0b1 << bit));
        } else {
            // TODO: not crash the entire program with abort()
            printf(FILEPOS "unknown opcode $%02x\n", opcode);
            assert(false);
            break;
        }
    }
    }

    if (opcode != 0xF4) {
        cpu->status.t = 0;
    }
}
