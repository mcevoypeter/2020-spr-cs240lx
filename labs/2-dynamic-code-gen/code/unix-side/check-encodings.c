#include <assert.h>
#include <sys/types.h>
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

    // 1. 
    const char *temp_file = "temp.s";
    int fd = create_file(temp_file);
    write_exact(fd, insts, strlen(insts));
    write_exact(fd, "\n", 1);
    close(fd);
    
    // 2.
    if (system("arm-none-eabi-as --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6zk temp.s -o temp.o"))
        panic("insts_emit: failed to assemble temp.s into temp.o\n");
    if (system("arm-none-eabi-objcopy temp.o -O binary temp.bin") != 0)
        panic("insts_emit: failed to objcopy temp.o into temp.bin\n");

    // 3.
    void *file = read_file(nbytes, "temp.bin");

    // 4.
    return file;
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
uint32_t emit_rrr(const char *instr_format, const char *reg) {
    char buf[1024];
    sprintf(buf, instr_format, reg);
    
    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}
#if 0
uint32_t emit_rrr(const char *op, const char *d, const char *s1, const char *s2) {
    char buf[1024];
    sprintf(buf, "%s %s, %s, %s", op, d, s1, s2);

    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}
#endif

uint32_t solve_rrr(const char *name, const char *instr_format, const char *opcode, 
        const char **legal_regs, const char *fixed_reg_1, const char *fixed_reg_2, unsigned *op) {


    uint32_t always_0 = ~0, always_1 = ~0;

    for (unsigned i = 0; legal_regs[i]; i++) {
        /*uint32_t u = emit_rrr(opcode, legal_regs[i], fixed_reg_1, fixed_reg_2); */
        uint32_t u = emit_rrr(instr_format, legal_regs[i]);

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

    /*output("register dst are bits set in: %x\n", changed);*/
    // find the offset.  we assume register bits are contig and within 0xf
    uint32_t offset = ffs(changed) - 1;

    // check that bits are contig and at most 4 bits are set.
    if(((changed >> offset) & ~0xf) != 0)
        panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);

    // refine the opcode.
    *op &= never_changed;
    /*output("opcode is in = %x\n", *op);*/

    return offset;
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
void derive_op_rrr(const char *name, const char *opcode, 
        const char **dst, const char **src1, const char **src2) {

    
    // #MINE
    const char *s1 = src1[0];
    const char *s2 = src2[0];
    const char *d = dst[0];
    assert(d && s1);
    uint8_t has_third_reg = src2 ? 1 : 0;

    unsigned op = ~0;
    char instr_format[1024];

    // solve for dst reg
    sprintf(instr_format, "%s %%s, %s, %s", opcode, s1, s2);
    uint32_t d_off = solve_rrr(name, instr_format, opcode, dst, s1, s2, &op);

    // solve for first source reg
    sprintf(instr_format, "%s %s, %%s, %s", opcode, d, s2);
    uint32_t src1_off = solve_rrr(name, instr_format, opcode, src1, d, s2, &op);

    // dummy call to zero out non-opcode bits 
    /*op &= emit_rrr(instr_format, "r0"); */

    // solve for second source reg
    if (has_third_reg) { 
        sprintf(instr_format, "%s %s, %s, %%s", opcode, d, s1);
        uint32_t src2_off = solve_rrr(name, instr_format, opcode, src2, d, s1, &op);

        // dummy call to zero out non-opcode bits 
        op &= emit_rrr(instr_format, "r0"); 

        output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
        output("    return %x | (dst << %d) | (src1 << %d) | (src2 << %d)\n",
                    op,
                    d_off,
                    src1_off,
                    src2_off);
        output("}\n");
    } else {
        sprintf(instr_format, "%s %s, %s, %%s", opcode, d, s1);
        output("static int %s(uint32_t dst, uint32_t src1) {\n", name);
        output("    return %x | (dst << %d) | (src1 << %d)\n",
                    op,
                    d_off,
                    src1_off);
        output("}\n");
    }
}

    
        
#if 0
    const char *s1 = src1[0];
    const char *s2 = src2[0];
    const char *d = dst[0];
    assert(d && s1 && s2);

    unsigned d_off = 0, src1_off = 0, src2_off = 0, op = ~0;

    uint32_t always_0 = ~0, always_1 = ~0;

    // compute any bits that changed as we vary d.
    for(unsigned i = 0; dst[i]; i++) {
        uint32_t u = emit_rrr(opcode, dst[i], s1, s2);

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

    output("register dst are bits set in: %x\n", changed);

    // find the offset.  we assume register bits are contig and within 0xf
    d_off = ffs(changed);
    
    // check that bits are contig and at most 4 bits are set.
    if(((changed >> d_off) & ~0xf) != 0)
        panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);
    // refine the opcode.
    op &= never_changed;
    output("opcode is in =%x\n", op);

    // emit: NOTE: obviously, currently <src1_off>, <src2_off> are not 
    // defined (so solve for them) and opcode needs to be refined more.
    output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
    output("    return %x | (dst << %d) | (src1 << %d) | (src2 << %d)\n",
                op,
                d_off,
                src1_off,
                src2_off);
    output("}\n");
#endif

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
    output("success!\n");

    // part 4: implement the code so it will derive the add instruction.
    output("\n-----------------------------------------\n");
    output("part4: checking that we can reverse engineer an <add>\n");

    const char *all_regs[] = {
                "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                0 
    };
    // XXX: should probably pass a bitmask in instead.
    derive_op_rrr("arm_and", "and", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_eor", "eor", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_sub", "sub", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_rsb", "rsb", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_add", "add", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_adc", "adc", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_sbc", "sbc", all_regs, all_regs, all_regs);
    derive_op_rrr("arm_rsc", "rsc", all_regs, all_regs, all_regs);

    derive_op_rrr("arm_tst", "tst", all_regs, all_regs, 0);
    derive_op_rrr("arm_teq", "teq", all_regs, all_regs, 0);
    derive_op_rrr("arm_cmp", "cmp", all_regs, all_regs, 0);
    derive_op_rrr("arm_cmn", "cmn", all_regs, all_regs, 0);
    derive_op_rrr("arm_orr", "orr", all_regs, all_regs, 0);
    derive_op_rrr("arm_mov", "mov", all_regs, all_regs, 0);
    derive_op_rrr("arm_bic", "bic", all_regs, all_regs, 0);
    derive_op_rrr("arm_mvn", "mvn", all_regs, all_regs, 0);
    output("did something: now use the generated code in the checks above!\n");
    assert(0);

    // get encodings for other instructions, loads, stores, branches, etc.
    return 0;
}
