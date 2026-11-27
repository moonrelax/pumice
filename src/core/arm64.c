#include "arm64.h"
#include "log.h"
#include <string.h>

void arm64_init(arm64_cpu *cpu, address_space *as) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->as = as;
    cpu->running = true;
}

void arm64_set_entry(arm64_cpu *cpu, uint64_t pc, uint64_t sp) {
    cpu->pc = pc;
    cpu->sp = sp;
}

static uint64_t reg_get(arm64_cpu *cpu, int r) {
    if (r == 31) return cpu->sp;
    return cpu->x[r];
}

static void reg_set(arm64_cpu *cpu, int r, uint64_t val) {
    if (r == 31) { cpu->sp = val; return; }
    if (r < 31)  cpu->x[r] = val;
}

static void reg_set_zr(arm64_cpu *cpu, int r, uint64_t val) {
    if (r == 31) return;
    cpu->x[r] = val;
}

static void update_nzcv_64(arm64_cpu *cpu, uint64_t result) {
    cpu->nzcv = 0;
    if (result & (1ULL << 63)) cpu->nzcv |= NZCV_N;
    if (result == 0)           cpu->nzcv |= NZCV_Z;
}

static bool cond_holds(arm64_cpu *cpu, uint32_t cond) {
    uint32_t f = cpu->nzcv;
    bool result;
    switch (cond >> 1) {
    case 0: result = (f & NZCV_Z) != 0; break;
    case 1: result = (f & NZCV_C) != 0; break;
    case 2: result = (f & NZCV_N) != 0; break;
    case 3: result = (f & NZCV_V) != 0; break;
    case 4: result = (f & NZCV_C) && !(f & NZCV_Z); break;
    case 5: result = !!(f & NZCV_N) == !!(f & NZCV_V); break;
    case 6: result = !!(f & NZCV_N) == !!(f & NZCV_V) && !(f & NZCV_Z); break;
    case 7: result = true; break;
    default: result = false;
    }
    if ((cond & 1) && cond != 0xF) result = !result;
    return result;
}

static int64_t sign_extend(uint64_t val, int bits) {
    int shift = 64 - bits;
    return (int64_t)(val << shift) >> shift;
}

static uint64_t decode_bitmask(uint32_t n, uint32_t imms, uint32_t immr, int regsize) {
    int len = 0;
    uint32_t combined = (n << 6) | (~imms & 0x3F);
    for (int i = 6; i >= 0; i--) {
        if (combined & (1u << i)) { len = i; break; }
    }
    int esize = 1 << len;
    int levels = esize - 1;
    int s = imms & levels;
    int r = immr & levels;

    uint64_t welem = (1ULL << (s + 1)) - 1;
    uint64_t rotated = (welem >> r) | (welem << (esize - r));
    rotated &= ((esize < 64) ? ((1ULL << esize) - 1) : ~0ULL);

    uint64_t result = 0;
    for (int i = 0; i < regsize; i += esize)
        result |= rotated << i;
    return result;
}

int arm64_step(arm64_cpu *cpu) {
    if (!cpu->running) return CPU_HALTED;

    uint32_t insn = mem_read32(cpu->as, cpu->pc);
    if (insn == 0) {
        LOG_ERR("null instruction at 0x%lx", cpu->pc);
        cpu->running = false;
        return CPU_FAULT;
    }

    /* SVC */
    if ((insn & 0xFFE0001F) == 0xD4000001) {
        cpu->svc_id = (insn >> 5) & 0xFFFF;
        cpu->pc += 4;
        return CPU_SVC;
    }

    /* BRK */
    if ((insn & 0xFFE0001F) == 0xD4200000) {
        cpu->running = false;
        return CPU_BREAK;
    }

    /* B (unconditional) */
    if ((insn >> 26) == 0x05) {
        int64_t off = sign_extend(insn & 0x3FFFFFF, 26) << 2;
        cpu->pc += off;
        return CPU_OK;
    }

    /* BL */
    if ((insn >> 26) == 0x25) {
        int64_t off = sign_extend(insn & 0x3FFFFFF, 26) << 2;
        cpu->x[30] = cpu->pc + 4;
        cpu->pc += off;
        return CPU_OK;
    }

    /* B.cond */
    if ((insn & 0xFF000010) == 0x54000000) {
        uint32_t cond = insn & 0xF;
        int64_t off = sign_extend((insn >> 5) & 0x7FFFF, 19) << 2;
        cpu->pc += cond_holds(cpu, cond) ? off : 4;
        return CPU_OK;
    }

    /* CBZ/CBNZ */
    if ((insn & 0x7E000000) == 0x34000000) {
        bool sf = (insn >> 31) & 1;
        bool nz = (insn >> 24) & 1;
        int rt = insn & 0x1F;
        int64_t off = sign_extend((insn >> 5) & 0x7FFFF, 19) << 2;
        uint64_t val = reg_get(cpu, rt);
        if (!sf) val &= 0xFFFFFFFF;
        bool taken = nz ? (val != 0) : (val == 0);
        cpu->pc += taken ? off : 4;
        return CPU_OK;
    }

    /* TBZ/TBNZ */
    if ((insn & 0x7E000000) == 0x36000000) {
        bool nz = (insn >> 24) & 1;
        int bit = ((insn >> 31) << 5) | ((insn >> 19) & 0x1F);
        int rt = insn & 0x1F;
        int64_t off = sign_extend((insn >> 5) & 0x3FFF, 14) << 2;
        bool set = (reg_get(cpu, rt) >> bit) & 1;
        bool taken = nz ? set : !set;
        cpu->pc += taken ? off : 4;
        return CPU_OK;
    }

    /* BR/BLR/RET */
    if ((insn & 0xFE1FFC1F) == 0xD61F0000) {
        int opc = (insn >> 21) & 3;
        int rn = (insn >> 5) & 0x1F;
        uint64_t target = reg_get(cpu, rn);
        if (opc == 1) cpu->x[30] = cpu->pc + 4;
        cpu->pc = target;
        return CPU_OK;
    }

    /* MOV (wide immediate): MOVZ/MOVK/MOVN */
    if ((insn & 0x1F800000) == 0x12800000) {
        bool sf = (insn >> 31) & 1;
        int opc = (insn >> 29) & 3;
        int hw = (insn >> 21) & 3;
        uint64_t imm16 = (insn >> 5) & 0xFFFF;
        int rd = insn & 0x1F;
        int shift = hw * 16;

        uint64_t result;
        if (opc == 0) {
            result = ~(imm16 << shift);
            if (!sf) result &= 0xFFFFFFFF;
        } else if (opc == 2) {
            result = imm16 << shift;
        } else if (opc == 3) {
            result = reg_get(cpu, rd);
            result &= ~(0xFFFFULL << shift);
            result |= imm16 << shift;
        } else {
            goto unhandled;
        }
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* ADD/SUB (immediate) */
    if ((insn & 0x1F000000) == 0x11000000) {
        bool sf = (insn >> 31) & 1;
        bool sub = (insn >> 30) & 1;
        bool setflags = (insn >> 29) & 1;
        int sh = (insn >> 22) & 1;
        uint64_t imm = (insn >> 10) & 0xFFF;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        if (sh) imm <<= 12;
        uint64_t op1 = reg_get(cpu, rn);
        uint64_t result;

        if (sub) result = op1 - imm;
        else     result = op1 + imm;

        if (!sf) result &= 0xFFFFFFFF;
        if (setflags) update_nzcv_64(cpu, sf ? result : (result & 0xFFFFFFFF));

        if (setflags) reg_set_zr(cpu, rd, result);
        else          reg_set(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* ADD/SUB (shifted register) */
    if ((insn & 0x1F200000) == 0x0B000000) {
        bool sf = (insn >> 31) & 1;
        bool sub = (insn >> 30) & 1;
        bool setflags = (insn >> 29) & 1;
        int shift_type = (insn >> 22) & 3;
        int rm = (insn >> 16) & 0x1F;
        int imm6 = (insn >> 10) & 0x3F;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        uint64_t op1 = reg_get(cpu, rn);
        uint64_t op2 = reg_get(cpu, rm);
        if (!sf) { op1 &= 0xFFFFFFFF; op2 &= 0xFFFFFFFF; }

        switch (shift_type) {
        case 0: op2 <<= imm6; break;
        case 1: op2 >>= imm6; break;
        case 2: op2 = (uint64_t)((int64_t)op2 >> imm6); break;
        default: break;
        }

        uint64_t result = sub ? op1 - op2 : op1 + op2;
        if (!sf) result &= 0xFFFFFFFF;
        if (setflags) update_nzcv_64(cpu, result);

        if (setflags) reg_set_zr(cpu, rd, result);
        else          reg_set(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* Logical (immediate): AND/ORR/EOR/ANDS */
    if ((insn & 0x1F800000) == 0x12000000) {
        bool sf = (insn >> 31) & 1;
        int opc = (insn >> 29) & 3;
        int n = (insn >> 22) & 1;
        int immr = (insn >> 16) & 0x3F;
        int imms = (insn >> 10) & 0x3F;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;
        int regsize = sf ? 64 : 32;

        uint64_t imm = decode_bitmask(n, imms, immr, regsize);
        uint64_t op1 = reg_get(cpu, rn);
        uint64_t result;

        switch (opc) {
        case 0: result = op1 & imm; break;
        case 1: result = op1 | imm; break;
        case 2: result = op1 ^ imm; break;
        case 3: result = op1 & imm; break;
        default: result = 0;
        }
        if (!sf) result &= 0xFFFFFFFF;
        if (opc == 3) update_nzcv_64(cpu, result);

        if (opc == 3) reg_set_zr(cpu, rd, result);
        else          reg_set(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* Logical (shifted register): AND/ORR/EOR/ANDS */
    if ((insn & 0x1F000000) == 0x0A000000) {
        bool sf = (insn >> 31) & 1;
        int opc = (insn >> 29) & 3;
        int shift_type = (insn >> 22) & 3;
        bool n = (insn >> 21) & 1;
        int rm = (insn >> 16) & 0x1F;
        int imm6 = (insn >> 10) & 0x3F;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        uint64_t op1 = reg_get(cpu, rn);
        uint64_t op2 = reg_get(cpu, rm);
        if (!sf) { op1 &= 0xFFFFFFFF; op2 &= 0xFFFFFFFF; }

        switch (shift_type) {
        case 0: op2 <<= imm6; break;
        case 1: op2 >>= imm6; break;
        case 2: op2 = (uint64_t)((int64_t)op2 >> imm6); break;
        case 3: op2 = (op2 >> imm6) | (op2 << (64 - imm6)); break;
        }
        if (n) op2 = ~op2;

        uint64_t result;
        switch (opc) {
        case 0: result = op1 & op2; break;
        case 1: result = op1 | op2; break;
        case 2: result = op1 ^ op2; break;
        case 3: result = op1 & op2; break;
        default: result = 0;
        }
        if (!sf) result &= 0xFFFFFFFF;
        if (opc == 3) update_nzcv_64(cpu, result);

        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* ADRP */
    if ((insn & 0x9F000000) == 0x90000000) {
        int rd = insn & 0x1F;
        int64_t immhi = (insn >> 5) & 0x7FFFF;
        int64_t immlo = (insn >> 29) & 3;
        int64_t imm = sign_extend((immhi << 2) | immlo, 21) << 12;
        reg_set_zr(cpu, rd, (cpu->pc & ~0xFFFULL) + imm);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* ADR */
    if ((insn & 0x9F000000) == 0x10000000) {
        int rd = insn & 0x1F;
        int64_t immhi = (insn >> 5) & 0x7FFFF;
        int64_t immlo = (insn >> 29) & 3;
        int64_t imm = sign_extend((immhi << 2) | immlo, 21);
        reg_set_zr(cpu, rd, cpu->pc + imm);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* LDR/STR (unsigned offset) */
    if ((insn & 0x3B000000) == 0x39000000) {
        int size = (insn >> 30) & 3;
        bool is_load = (insn >> 22) & 1;
        uint64_t imm12 = (insn >> 10) & 0xFFF;
        int rn = (insn >> 5) & 0x1F;
        int rt = insn & 0x1F;
        int scale = size;

        uint64_t offset = imm12 << scale;
        uint64_t addr = reg_get(cpu, rn) + offset;

        if (is_load) {
            uint64_t val = 0;
            mem_read(cpu->as, addr, &val, 1 << scale);
            reg_set_zr(cpu, rt, val);
        } else {
            uint64_t val = (rt == 31) ? 0 : cpu->x[rt];
            mem_write(cpu->as, addr, &val, 1 << scale);
        }
        cpu->pc += 4;
        return CPU_OK;
    }

    /* LDP/STP (signed offset) */
    if ((insn & 0x3E000000) == 0x28000000 || (insn & 0x3E000000) == 0x2C000000) {
        bool is_load = (insn >> 22) & 1;
        int opc_hi = (insn >> 30) & 1;
        int64_t imm7 = sign_extend((insn >> 15) & 0x7F, 7);
        int rt2 = (insn >> 10) & 0x1F;
        int rn = (insn >> 5) & 0x1F;
        int rt = insn & 0x1F;
        int scale = opc_hi ? 3 : 2;

        uint64_t offset = imm7 << scale;
        uint64_t addr = reg_get(cpu, rn) + offset;
        int bytes = 1 << scale;

        if (is_load) {
            uint64_t v1 = 0, v2 = 0;
            mem_read(cpu->as, addr, &v1, bytes);
            mem_read(cpu->as, addr + bytes, &v2, bytes);
            reg_set_zr(cpu, rt, v1);
            reg_set_zr(cpu, rt2, v2);
        } else {
            uint64_t v1 = (rt == 31) ? 0 : cpu->x[rt];
            uint64_t v2 = (rt2 == 31) ? 0 : cpu->x[rt2];
            mem_write(cpu->as, addr, &v1, bytes);
            mem_write(cpu->as, addr + bytes, &v2, bytes);
        }
        cpu->pc += 4;
        return CPU_OK;
    }

    /* MRS (tpidr_el0 only) */
    if ((insn & 0xFFFFFFE0) == 0xD53BD040) {
        int rt = insn & 0x1F;
        reg_set_zr(cpu, rt, cpu->tpidr_el0);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* MSR (tpidr_el0 only) */
    if ((insn & 0xFFFFFFE0) == 0xD51BD040) {
        int rt = insn & 0x1F;
        cpu->tpidr_el0 = reg_get(cpu, rt);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* NOP */
    if (insn == 0xD503201F) {
        cpu->pc += 4;
        return CPU_OK;
    }

    /* MADD/MSUB (includes MUL/MNEG) */
    if ((insn & 0x1FE00000) == 0x1B000000) {
        bool sf = (insn >> 31) & 1;
        int rm = (insn >> 16) & 0x1F;
        int o0 = (insn >> 15) & 1;
        int ra = (insn >> 10) & 0x1F;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        uint64_t a = reg_get(cpu, rn);
        uint64_t b = reg_get(cpu, rm);
        uint64_t acc = (ra == 31) ? 0 : cpu->x[ra];
        uint64_t result = o0 ? acc - a * b : acc + a * b;
        if (!sf) result &= 0xFFFFFFFF;
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* UDIV/SDIV */
    if ((insn & 0x1FE0FC00) == 0x1AC00800) {
        bool sf = (insn >> 31) & 1;
        int rm = (insn >> 16) & 0x1F;
        bool is_signed = (insn >> 10) & 1;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        uint64_t a = reg_get(cpu, rn);
        uint64_t b = reg_get(cpu, rm);
        if (!sf) { a &= 0xFFFFFFFF; b &= 0xFFFFFFFF; }

        uint64_t result;
        if (b == 0) {
            result = 0;
        } else if (is_signed) {
            result = (uint64_t)((int64_t)a / (int64_t)b);
        } else {
            result = a / b;
        }
        if (!sf) result &= 0xFFFFFFFF;
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* Shift register: LSL/LSR/ASR/ROR */
    if ((insn & 0x1FE0F000) == 0x1AC02000) {
        bool sf = (insn >> 31) & 1;
        int rm = (insn >> 16) & 0x1F;
        int op2 = (insn >> 10) & 3;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;
        int regsize = sf ? 64 : 32;

        uint64_t val = reg_get(cpu, rn);
        uint64_t amt = reg_get(cpu, rm) % regsize;
        if (!sf) val &= 0xFFFFFFFF;

        uint64_t result;
        switch (op2) {
        case 0: result = val << amt; break;
        case 1: result = val >> amt; break;
        case 2: result = (uint64_t)((int64_t)val >> amt); break;
        case 3: result = (val >> amt) | (val << (regsize - amt)); break;
        default: result = 0;
        }
        if (!sf) result &= 0xFFFFFFFF;
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* CSEL/CSINC/CSINV/CSNEG */
    if ((insn & 0x1FE00000) == 0x1A800000) {
        bool sf = (insn >> 31) & 1;
        bool op = (insn >> 30) & 1;
        int rm = (insn >> 16) & 0x1F;
        int cond = (insn >> 12) & 0xF;
        int o2 = (insn >> 10) & 1;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;

        uint64_t vn = reg_get(cpu, rn);
        uint64_t vm = reg_get(cpu, rm);

        uint64_t result;
        if (cond_holds(cpu, cond)) {
            result = vn;
        } else {
            if (!op && !o2) result = vm;
            else if (!op && o2) result = vm + 1;
            else if (op && !o2) result = ~vm;
            else result = ~vm + 1;
        }
        if (!sf) result &= 0xFFFFFFFF;
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

    /* Bitfield: SBFM/BFM/UBFM (includes LSL/LSR/ASR imm, SXTB/SXTH/SXTW, UXTB/UXTH) */
    if ((insn & 0x1F800000) == 0x13000000) {
        bool sf = (insn >> 31) & 1;
        int opc = (insn >> 29) & 3;
        int immr = (insn >> 16) & 0x3F;
        int imms = (insn >> 10) & 0x3F;
        int rn = (insn >> 5) & 0x1F;
        int rd = insn & 0x1F;
        int regsize = sf ? 64 : 32;

        uint64_t src = reg_get(cpu, rn);
        if (!sf) src &= 0xFFFFFFFF;

        uint64_t wmask = decode_bitmask(sf, imms, immr, regsize);
        uint64_t rotated = (src >> immr) | (src << (regsize - immr));
        if (!sf) rotated &= 0xFFFFFFFF;

        uint64_t result;
        if (opc == 0) {
            result = rotated & wmask;
            if (imms < immr) {
                if (rotated & (1ULL << imms))
                    result |= ~wmask & (sf ? ~0ULL : 0xFFFFFFFF);
            } else {
                result = (uint64_t)sign_extend(result, imms + 1);
                if (!sf) result &= 0xFFFFFFFF;
            }
        } else if (opc == 1) {
            uint64_t dst = (rd == 31) ? 0 : cpu->x[rd];
            result = (dst & ~wmask) | (rotated & wmask);
        } else {
            result = rotated & wmask;
        }
        if (!sf) result &= 0xFFFFFFFF;
        reg_set_zr(cpu, rd, result);
        cpu->pc += 4;
        return CPU_OK;
    }

unhandled:
    LOG_ERR("unhandled insn 0x%08X at 0x%lx", insn, cpu->pc);
    cpu->running = false;
    return CPU_FAULT;
}

int arm64_run(arm64_cpu *cpu, uint64_t max_steps) {
    for (uint64_t i = 0; i < max_steps && cpu->running; i++) {
        int r = arm64_step(cpu);
        if (r != CPU_OK) return r;
    }
    return cpu->running ? CPU_OK : CPU_HALTED;
}
