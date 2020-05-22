#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__
// engler, cs240lx: simplistic instruction encodings for r/pi ARMV6.
// this will compile both on our bare-metal r/pi and unix.

// bit better type checking to use enums.
enum {
    arm_r0 = 0, 
    arm_r1, 
    arm_r2,
    arm_r3,
    arm_r4,
    arm_r5,
    arm_r6,
    arm_r7,
    arm_r8,
    arm_r9,
    arm_r10,
    arm_r11,
    arm_r12,
    arm_r13,
    arm_r14,
    arm_r15,
    arm_sp = arm_r13,
    arm_lr = arm_r14,
    arm_pc = arm_r15
};
_Static_assert(arm_r15 == 15, "bad enum");


// condition code.
enum {
    arm_EQ = 0,
    arm_NE,
    arm_CS,
    arm_CC,
    arm_MI,
    arm_PL,
    arm_VS,
    arm_VC,
    arm_HI,
    arm_LS,
    arm_GE,
    arm_LT,
    arm_GT,
    arm_LE,
    arm_AL,
};
_Static_assert(arm_AL == 0b1110, "bad enum list");

// data processing ops.
enum {
    arm_and_op = 0, 
    arm_eor_op,
    arm_sub_op,
    arm_rsb_op,
    arm_add_op,
    arm_adc_op,
    arm_sbc_op,
    arm_rsc_op,
    arm_tst_op,
    arm_teq_op,
    arm_cmp_op,
    arm_cmn_op,
    arm_orr_op,
    arm_mov_op,
    arm_bic_op,
    arm_mvn_op,
};
_Static_assert(arm_mvn_op == 0b1111, "bad num list");

/************************************************************
 * instruction encodings.  should do:
 *      bx lr
 *      ld *
 *      st *
 *      cmp
 *      branch
 *      alu 
 *      alu_imm
 *      jump
 *      call
 */

// add instruction:
//      add rdst, rs1, rs2
//  - general add instruction: page A4-6 [armv6.pdf]
//  - shift operatnd: page A5-8 [armv6.pdf]
//
// we do not do any carries, so S = 0.
struct data_proc_instr {
    uint32_t shifter_operand:12,
             Rd:4,
             Rn:4,
             S:1,   // carry
             opcode:4,
             I:3,   // immediate
             cond:4;
};
typedef struct data_proc_instr data_proc_instr_t;

static inline unsigned arm_add(uint8_t rd, uint8_t rs1, uint8_t rs2) {
    assert(arm_add_op == 0b0100);
    data_proc_instr_t instr = {
        .cond = arm_AL,
        .I = 0,
        .opcode = arm_add_op,
        .S = 0,
        .Rn = rs1,
        .Rd = rd,
        .shifter_operand = rs2
    };
    unsigned encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// <add> of an immediate
static inline uint32_t arm_add_imm8(uint8_t rd, uint8_t rs1, uint8_t imm) {
    unimplemented();
}

static inline uint32_t arm_bx(uint8_t reg) {
    unimplemented();
}

// load an or immediate and rotate it.
static inline uint32_t 
arm_or_imm_rot(uint8_t rd, uint8_t rs1, uint8_t imm8, uint8_t rot_nbits) {
    unimplemented();
}

// A4-68
struct mov_instr {
    uint32_t shifter_operand:12,
             Rd:4,
             SBZ:4,
             S:1,
             fixed1:4,
             I:1,
             fixed2:2,
             cond:4;
};
typedef struct mov_instr mov_instr_t;

// A4-68
static inline unsigned arm_mov_reg(uint8_t rd, uint8_t rn) {
    mov_instr_t instr = {
        .cond = arm_AL,
        .fixed2 = 0b00,
        .I = 0,
        .fixed1 = 0b1101,
        .S = 0,
        .SBZ = 0b0000,
        .shifter_operand = rn
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// A4-68
static inline unsigned arm_mov_imm(uint8_t rd, uint8_t imm) {
    mov_instr_t instr = {
        .cond = arm_AL,
        .fixed2 = 0b00,
        .I = 1,
        .fixed1 = 0b1101,
        .S = 0,
        .SBZ = 0b0000,
        .shifter_operand = imm // this definitely needs to change
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}
// A3-21 
struct ldr_str_instr {
    uint32_t addressing_mode_specific:12, 
             Rd:4,
             Rn:4,
             L:1,
             W:1,
             B:1,
             U:1,
             P:1,
             I:1,
             fixed:2,
             cond:4;
};
typedef struct ldr_str_instr ldr_str_instr_t;

// load immediate - A5-19
static inline uint32_t arm_ldr_imm(uint8_t rd, uint8_t rn, int16_t imm) {
    ldr_str_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b01,
        .I = 0,
        .P = 1,
        .U = (imm < 0 ? 0 : 1),
        .B = 0,
        .W = 0,
        .L = 1,
        .Rn = rn,
        .Rd = rd,
        .addressing_mode_specific = (imm < 0 ? -imm : imm)
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// store immediate - A5-19
static inline uint32_t arm_str_imm(uint8_t rd, uint8_t rn, int16_t imm) {
    ldr_str_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b01,
        .I = 0,
        .P = 1,
        .U = (imm < 0 ? 0 : 1),
        .B = 0,
        .W = 0,
        .L = 0,
        .Rn = rn,
        .Rd = rd,
        .addressing_mode_specific = (imm < 0 ? -imm : imm)
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// A3-26
struct ldm_stm_instr {
    uint32_t reg_list:16,
             Rn:4,
             L:1,
             W:1,
             S:1,
             U:1,
             P:1,
             fixed:3,
             cond:4;
};
typedef struct ldm_stm_instr ldm_stm_instr_t;

// addressing modes for ldm and stm - A5-48
enum {
    arm_DA = 0,
    arm_IA,
    arm_DB,
    arm_IB
};
_Static_assert(arm_IB == 0b11, "bad_enum");

// values of W - A5-48
enum {
    arm_const_base = 0,
    arm_incr_base
};
_Static_assert(arm_incr_base == 1, "bad enum");
       

// load multiple - A5-42
static inline uint32_t arm_ldm(uint8_t addr_mode, uint8_t incr_base, uint8_t rn, uint16_t reg_list) {
    ldm_stm_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b100,
        .P = (addr_mode >> 1) & 1,
        .U = addr_mode & 1,
        .S = 0,     // unsure about this one
        .W = incr_base,
        .L = 1,
        .Rn = rn,
        .reg_list = reg_list
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;

}

// pop
static inline uint32_t arm_pop(uint8_t rn, uint16_t reglist) {
    return arm_ldm(arm_IA, arm_incr_base, rn, reglist);
}

// store multiple - A5-42
static inline uint32_t arm_stm(uint8_t addr_mode, uint8_t incr_base, uint8_t rn, uint16_t reg_list) {
    ldm_stm_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b100,
        .P = (addr_mode >> 1) & 1,
        .U = addr_mode & 1,
        .S = 0,     // unsure about this one
        .W = incr_base,
        .L = 0,
        .Rn = rn,
        .reg_list = reg_list
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// push
static inline uint32_t arm_push(uint8_t rn, uint16_t reglist) {
    return arm_stm(arm_DB, arm_incr_base, rn, reglist);
}

// A4-10
struct b_bl_instr {
    uint32_t signed_immed:24,
             L:1,
             fixed:3,
             cond:4;
};
typedef struct b_bl_instr b_bl_instr_t;

// branch and (optionally) link - A4-10
static inline uint32_t arm_b(int32_t src, int32_t dest) {
    // subtract pc+8 from the target and right shift 2
    int32_t signed_immed = (dest - (src + 8)) >> 2;
    int32_t target = src + signed_immed + 8;
    b_bl_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b101,
        .L = 0,
        .signed_immed = signed_immed
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}

// branch and link - A4-10
static inline uint32_t arm_bl(int32_t src, int32_t dest) {
    // subtract pc+8 from the target and right shift 2
    int32_t signed_immed = (dest - (src + 8)) >> 2;
    int32_t target = src + signed_immed + 8;
    b_bl_instr_t instr = {
        .cond = arm_AL,
        .fixed = 0b101,
        .L = 1,
        .signed_immed = signed_immed
    };
    uint32_t encoded_instr;
    memcpy(&encoded_instr, &instr, sizeof(unsigned));
    return encoded_instr;
}


#endif
