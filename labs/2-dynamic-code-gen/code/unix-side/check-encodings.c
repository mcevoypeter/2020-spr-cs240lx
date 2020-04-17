#include <assert.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include "libunix.h"
#include <unistd.h>
#include "code-gen.h"
#include "armv6-insts.h"

/*
 *  1. emits <insts> into a temporary file.
 *  2. compiles it.
 *  3. reads back in.
 *  4. returns pointer to it.
 */
uint32_t *insts_emit(unsigned *nbytes, char *insts) {
    // check libunix.h --- create_file, write_exact, run_system, read_file.

    int fd = create_file("temp.s");
    write_exact(fd, insts, strlen(insts));
    write_exact(fd, "\n", 1);
    close(fd);
    
    if (system("arm-none-eabi-as --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6zk temp.s -o temp.o"))
        panic("insts_emit: failed to assemble temp.s into temp.o\n");
    if (system("arm-none-eabi-objcopy temp.o -O binary temp.bin") != 0)
        panic("insts_emit: failed to objcopy temp.o into temp.bin\n");

    return read_file(nbytes, "temp.bin");
}

/*
 * a cross-checking hack that uses the native GNU compiler/assembler to 
 * check our instruction encodings.
 *  1. compiles <insts>
 *  2. compares <code,nbytes> to it for equivalance.
 *  3. prints out a useful error message if it did not succeed!!
 */
void insts_check(char *insts, uint32_t *code, unsigned nbytes) {
    // make sure you print out something useful on mismatch!
    unsigned n;
    uint32_t *compiled_code = insts_emit(&n, insts);
    assert(n == nbytes);
    if (memcmp(code, compiled_code, nbytes) == 0)
        printf("success: correctly encoded <%s>! as [ 0x%x ]\n", insts, *code);
    else
        panic("insts_check: encoding [ 0x%x ] from `insts_emit` doesn't match the actual encoding [ 0x%x ]\n", *compiled_code, *code);
}

// check a single instruction.
void check_one_inst(char *insts, uint32_t inst) {
    return insts_check(insts, &inst, 4);
}

// helper function to make reverse engineering instructions a bit easier.
void insts_print(char *insts) {
    // emit <insts>
    unsigned gen_nbytes;
    uint32_t *gen = insts_emit(&gen_nbytes, insts);

    // print the result.
    output("getting encoding for: < %20s >\t= [", insts);
    unsigned n = gen_nbytes / 4;
    for(int i = 0; i < n; i++)
         output(" 0x%x ", gen[i]);
    output("]\n");
}

// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_rplus(const char *instr_format, const char *reg) {
    char buf[1024];
    sprintf(buf, instr_format, reg);
    
    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}

int32_t solve_rplus(const char *instr_format, uint32_t *opcode, const char **legal_regs) {
    uint32_t always_0 = ~0, always_1 = ~0;

    for (uint8_t i = 0; legal_regs[i]; i++) {
        uint32_t u = emit_rplus(instr_format, legal_regs[i]);

        // if a bit is always 0 then it will be 1 in always_0
        always_0 &= ~u;

        // if a bit is always 1 it will be 1 in always_1, otherwise 0
        always_1 &= u;
    }

    if(always_0 & always_1) 
        panic("impossible overlap: always_0 = %x, always_1 %x\n", 
            always_0, always_1);

    // bits that never changed
    uint32_t never_changed = always_0 | always_1;
    // bits that changed: these are the register bits.
    uint32_t changed = ~never_changed;

    // find the offset.  we assume register bits are contig and within 0xf
    int32_t offset = ffs(changed) - 1;

    // check that bits are contig and at most 4 bits are set.
    if(((changed >> offset) & ~0xf) != 0)
        panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);

    // refine the opcode.
    *opcode &= never_changed;

    return offset;
}

// print out the generated function
void print_rplus(const char *name, uint32_t opcode, int32_t d_off, int32_t s1_off, int32_t s2_off) {
    assert(d_off >= 0);
    // instruction with one register
    if (s1_off == -1 && s2_off == -1) {
        output("static int %s(uint32_t dst) {\n", name);
        output("    return %x | (dst << %d);\n",
                opcode,
                d_off);
    // instruction with two registers
    } else if (s2_off == -1) {
        output("static int %s(uint32_t dst, uint32_t src) {\n", name);
        output("    return %x | (dst << %d) | (src << %d);\n",
                opcode,
                d_off,
                s1_off);
    // instruction with three registers
    } else {
        output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
        output("    return %x | (dst << %d) | (src1 << %d) | (src2 << %d);\n",
                opcode,
                d_off,
                s1_off,
                s2_off);
    }
    output("}\n");
}

// overly-specific.  some assumptions:
//  1. each register is encoded with its number (r0 = 0, r1 = 1)
//  2. related: all register fields are contiguous.
//
// NOTE: you should probably change this so you can emit all instructions 
// all at once, read in, and then solve all at once.
//
// For lab:
//  1. complete this code so that it solves for the other registers.
//  2. refactor so that you can reused the solving code vs cut and pasting it.
//  3. extend system_* so that it returns an error.
//  4. emit code to check that the derived encoding is correct.
//  5. emit if statements to checks for illegal registers (those not in <src1>,
//    <src2>, <dst>).
void derive_op_rplus(const char *name, const char *op, 
        uint32_t reg_cnt, ...) {

    if (reg_cnt < 1 || 3 < reg_cnt)
        panic("solve_rplus: an instruction must take between 1 and 3 registers\n");

    va_list regs;
    va_start(regs, reg_cnt);
    const char **dst = va_arg(regs, const char**);
    const char **src1 = (reg_cnt > 1 ? va_arg(regs, const char**) : 0);
    const char **src2 = (reg_cnt > 2 ? va_arg(regs, const char**) : 0);
    va_end(regs);

    const char *d = dst[0];
    const char *s1 = (src1 ? src1[0] : 0);
    const char *s2 = (src2 ? src2[0] : 0);
    assert(d);

    char instr_format[1024];
    uint32_t opcode = ~0;
    int32_t d_off = -1, s1_off = -1, s2_off = -1;
    switch (reg_cnt) {
        case 3:
            // solve for second source register
            sprintf(instr_format, "%s %s, %s, %%s", op, d, s1);
            s2_off = solve_rplus(instr_format, &opcode, src2);
        case 2:
            // solve for first source register
            if (reg_cnt == 2)
                sprintf(instr_format, "%s %s, %%s", op, d);
            else if (reg_cnt == 3)
                sprintf(instr_format, "%s %s, %%s, %s", op, d, s2);
            s1_off = solve_rplus(instr_format, &opcode, src1);
        case 1:
            // solve for destination register
            if (reg_cnt == 1)
                sprintf(instr_format, "%s %%s", op);
            else if (reg_cnt == 2)
                sprintf(instr_format, "%s %%s, %s", op, s1);
            else if (reg_cnt == 3)
                sprintf(instr_format, "%s %%s, %s, %s", op, s1, s2);
            d_off = solve_rplus(instr_format, &opcode, dst);
    }

    // dummy call to zero out non-opcode bits 
    opcode &= emit_rplus(instr_format, "r0"); 

    print_rplus(name, opcode, d_off, s1_off, s2_off);
}

/*
 * 1. we start by using the compiler / assembler tool chain to get / check
 *    instruction encodings.  this is sleazy and low-rent.   however, it 
 *    lets us get quick and dirty results, removing a bunch of the mystery.
 *
 * 2. after doing so we encode things "the right way" by using the armv6
 *    manual (esp chapters a3,a4,a5).  this lets you see how things are 
 *    put together.  but it is tedious.
 *
 * 3. to side-step tedium we use a variant of (1) to reverse engineer 
 *    the result.
 *
 *    we are only doing a small number of instructions today to get checked off
 *    (you, of course, are more than welcome to do a very thorough set) and focus
 *    on getting each method running from beginning to end.
 *
 * 4. then extend to a more thorough set of instructions: branches, loading
 *    a 32-bit constant, function calls.
 *
 * 5. use (4) to make a simple object oriented interface setup.
 *    you'll need: 
 *      - loads of 32-bit immediates
 *      - able to push onto a stack.
 *      - able to do a non-linking function call.
 */
int main(void) {
    // part 1: implement the code to do this.
    output("-----------------------------------------\n");
    output("part1: checking: correctly generating assembly.\n");
    insts_print("add r0, r0, r1");
    insts_print("bx lr");
    insts_print("mov r0, #1");
    insts_print("nop");
    output("\n");
    output("success!\n");

    // part 2: implement the code so these checks pass.
    // these should all pass.
    output("\n-----------------------------------------\n");
    output("part 2: checking we correctly compare asm to machine code.\n");
    check_one_inst("add r0, r0, r1", 0xe0800001);
    check_one_inst("bx lr", 0xe12fff1e);
    check_one_inst("mov r0, #1", 0xe3a00001);
    check_one_inst("nop", 0xe320f000);
    output("success!\n");

    // part 3: check that you can correctly encode an add instruction.
    output("\n-----------------------------------------\n");
    output("part3: checking that we can generate an <add> by hand\n");
    check_one_inst("add r0, r1, r2", arm_add(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r3, r4, r5", arm_add(arm_r3, arm_r4, arm_r5));
    check_one_inst("add r6, r7, r8", arm_add(arm_r6, arm_r7, arm_r8));
    check_one_inst("add r9, r10, r11", arm_add(arm_r9, arm_r10, arm_r11));
    check_one_inst("add r12, r13, r14", arm_add(arm_r12, arm_r13, arm_r14));
    check_one_inst("add r15, r7, r3", arm_add(arm_r15, arm_r7, arm_r3));
    check_one_inst("mov r0, r1", arm_mov_reg(arm_r0, arm_r1));
    check_one_inst("mov r0, #9", arm_mov_imm(arm_r0, 9));
    check_one_inst("ldr r0, [pc, #0]", arm_ldr_imm(arm_r0, arm_r15, 0));
    check_one_inst("ldr r0, [pc, #256]", arm_ldr_imm(arm_r0, arm_r15, 256));
    check_one_inst("ldr r0, [pc, #-256]", arm_ldr_imm(arm_r0, arm_r15, -256));
    check_one_inst("str pc, [r0, #0]", arm_str_imm(arm_r15, arm_r0, 0));
    check_one_inst("str pc, [r0, #256]", arm_str_imm(arm_r15, arm_r0, 256));
    check_one_inst("str pc, [r0, #-256]", arm_str_imm(arm_r15, arm_r0, -256));
    check_one_inst("b label; label: ", arm_b(0, 4));
    check_one_inst("bl label; label: ", arm_bl(0, 4));
    check_one_inst("label: b label; ", arm_b(0, 0));
    check_one_inst("label: bl label; ", arm_bl(0, 0));
    uint16_t reg_list = (1 << arm_r3 | 1 << arm_r2 | 1 << arm_r1);
    check_one_inst("stmia r0, {r1, r2, r3}", arm_stm(arm_IA, arm_const_base, arm_r0, reg_list));
    check_one_inst("stmib r0, {r1, r2, r3}", arm_stm(arm_IB, arm_const_base, arm_r0, reg_list));
    check_one_inst("stmda r0, {r1, r2, r3}", arm_stm(arm_DA, arm_const_base, arm_r0, reg_list));
    check_one_inst("stmdb r0, {r1, r2, r3}", arm_stm(arm_DB, arm_const_base, arm_r0, reg_list));
    check_one_inst("stmia r0!, {r1, r2, r3}", arm_stm(arm_IA, arm_incr_base, arm_r0, reg_list));
    check_one_inst("stmib r0!, {r1, r2, r3}", arm_stm(arm_IB, arm_incr_base, arm_r0, reg_list));
    check_one_inst("stmda r0!, {r1, r2, r3}", arm_stm(arm_DA, arm_incr_base, arm_r0, reg_list));
    check_one_inst("stmdb r0!, {r1, r2, r3}", arm_stm(arm_DB, arm_incr_base, arm_r0, reg_list));
    check_one_inst("ldmia r0, {r1, r2, r3}", arm_ldm(arm_IA, arm_const_base, arm_r0, reg_list));
    check_one_inst("ldmib r0, {r1, r2, r3}", arm_ldm(arm_IB, arm_const_base, arm_r0, reg_list));
    check_one_inst("ldmda r0, {r1, r2, r3}", arm_ldm(arm_DA, arm_const_base, arm_r0, reg_list));
    check_one_inst("ldmdb r0, {r1, r2, r3}", arm_ldm(arm_DB, arm_const_base, arm_r0, reg_list));
    check_one_inst("ldmia r0!, {r1, r2, r3}", arm_ldm(arm_IA, arm_incr_base, arm_r0, reg_list));
    check_one_inst("ldmib r0!, {r1, r2, r3}", arm_ldm(arm_IB, arm_incr_base, arm_r0, reg_list));
    check_one_inst("ldmda r0!, {r1, r2, r3}", arm_ldm(arm_DA, arm_incr_base, arm_r0, reg_list));
    check_one_inst("ldmdb r0!, {r1, r2, r3}", arm_ldm(arm_DB, arm_incr_base, arm_r0, reg_list));
    check_one_inst("push {r3, lr}", arm_stm(arm_DB, arm_incr_base, arm_sp, (1 << arm_lr | 1 << arm_r3)));
    check_one_inst("pop {r3, pc}", arm_ldm(arm_IA, arm_incr_base, arm_sp, (1 << arm_pc | 1 << arm_r3)));
    output("success!\n");

    // part 4: implement the code so it will derive the add instruction.
    output("\n-----------------------------------------\n");
    output("part4: checking that we can reverse engineer data processing instructions\n");

    const char *all_regs[] = {
                "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                0 
    };
    // XXX: should probably pass a bitmask in instead.
    derive_op_rplus("arm_and", "and", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_eor", "eor", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_sub", "sub", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_rsb", "rsb", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_add", "add", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_adc", "adc", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_sbc", "sbc", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_rsc", "rsc", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_tst", "tst", 2, all_regs, all_regs);
    derive_op_rplus("arm_teq", "teq", 2, all_regs, all_regs);
    derive_op_rplus("arm_cmp", "cmp", 2, all_regs, all_regs);
    derive_op_rplus("arm_cmn", "cmn", 2, all_regs, all_regs);
    derive_op_rplus("arm_orr", "orr", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_mov", "mov", 2, all_regs, all_regs);
    derive_op_rplus("arm_bic", "bic", 3, all_regs, all_regs, all_regs);
    derive_op_rplus("arm_mvn", "mvn", 2, all_regs, all_regs);
    output("did something: now use the generated code in the checks above!\n");

    // get encodings for other instructions, loads, stores, branches, etc.
    return 0;
}
