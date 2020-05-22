// starter code for trivial heap checking using interrupts.
#include "rpi.h"
#include "rpi-internal.h"
#include "ckalloc-internal.h"
#include "timer-interrupt.h"

#define VERBOSE 0
#define NCYCLES 0x4

// you'll need to pull your code from lab 2 here so you
// can fabricate jumps
 #include "armv6-insts.h"

typedef uint32_t instr_t;
typedef struct {
    instr_t instr0;
    instr_t instr1;
    instr_t instr2;
    instr_t instr3;
    instr_t instr4;
    instr_t instr5;
} trampoline_t;
static volatile trampoline_t *trampolines;
static volatile unsigned char *rewritten_instructions;
static volatile instr_t *instructions = (void *)&__code_start__;

// used to check initialization.
static volatile int init_p, check_on;

// allow them to limit checking to a range.  for simplicity we 
// only check a single contiguous range of code.  initialize to 
// the entire program.
static uint32_t 
    start_check = (uint32_t)&__code_start__, 
    end_check = (uint32_t)&__code_end__,
    // you will have to define these functions.
    start_nocheck = (uint32_t)ckalloc_start,
    end_nocheck = (uint32_t)ckalloc_end;


static int in_range(uint32_t addr, uint32_t b, uint32_t e) {
    assert(b<e);
    return addr >= b && addr < e;
}

// if <pc> is in the range we want to check and not in the 
// range we cannot check, return 1.
int (ck_mem_checked_pc)(uint32_t pc) {
    // XXX
    return in_range(pc, start_check, end_check) 
        && !in_range(pc, start_nocheck, end_nocheck);
}

// useful variables to track: how many times we did 
// checks, how many times we skipped them b/c <ck_mem_checked_pc>
// returned 0 (skipped)
static volatile unsigned checked = 0, skipped = 0;

unsigned ck_mem_stats(int clear_stats_p) { 
    unsigned s = skipped, c = checked, n = s+c;
#if VERBOSE
    printk("total interrupts = %d, checked instructions = %d, skipped = %d\n",
        n,c,s);
#endif
    if(clear_stats_p)
        skipped = checked = 0;
    return c;
}

void trampoline(uint32_t pc) {
    printk("pc=%x: we did it!\n", pc);
}

// note: lr = the pc that we were interrupted at.
// longer term: pass in the entire register bank so we can figure
// out more general questions.
enum { need_to_rewrite = 0, rewritten };
void ck_mem_interrupt(uint32_t pc) {

    // we don't know what the user was doing.
    dev_barrier();

    // XXX
#if VERBOSE
    trace("interrupt triggered: pc=%x\n", pc);
#endif
    unsigned pending = GET32(IRQ_basic_pending);

    // play it safe and panic if we get a non-timer interrupt
    if ((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        panic("non-timer interrupt triggered\n");

    // clear the interrupt
    PUT32(arm_timer_IRQClear, 1);

    // we don't know what the user was doing.
    dev_barrier();

    // we interrupted checkable code
    int should_rewrite = ck_mem_checked_pc(pc);
    unsigned offset = (pc - (uint32_t)&__code_start__) / sizeof(instr_t);
    int already_rewritten = rewritten_instructions[offset];
    if (should_rewrite && !already_rewritten) {
        printk("rewriting\n");
        instr_t original_instr = instructions[offset];
        // push r0-r12, r14 onto stack
        uint16_t reglist = (uint16_t)~(1 << arm_pc | 1 << arm_sp);
        uint32_t bl_src = (uint32_t)((instr_t *)(trampolines+offset) + 2);
        uint32_t b_src = (uint32_t)((instr_t *)(trampolines+offset) + 5);
        trampolines[offset] = (trampoline_t) {
            .instr0 = arm_push(arm_sp, reglist),
            .instr1 = arm_mov_reg(arm_r0, pc),
            .instr2 = arm_bl(bl_src, (uint32_t)trampoline),
            .instr3 = arm_pop(arm_sp, reglist),
            .instr4 = original_instr,
            .instr5 = arm_b(b_src, pc+4)
        };

        instructions[offset] = arm_b((uint32_t)(instructions+offset), (uint32_t)trampolines+offset);
        
        rewritten_instructions[offset] = rewritten;     
        checked++;
    } else if (!should_rewrite)
        skipped++;
}


// do any interrupt init you need, etc.
void ck_mem_init(void) { 
    assert(!init_p);
    init_p = 1;

    assert(in_range((uint32_t)ckalloc, start_nocheck, end_nocheck));
    assert(in_range((uint32_t)ckfree, start_nocheck, end_nocheck));
    assert(!in_range((uint32_t)printk, start_nocheck, end_nocheck));

    // XXX
    int_init();
    trace("setting up timer interrupts: period=%u\n", NCYCLES);
    timer_interrupt_init(NCYCLES);
}

// only check pc addresses [start,end)
void ck_mem_set_range(void *start, void *end) {
    assert(start < end);

    // XXX
    start_check = (uint32_t)start;
    end_check = (uint32_t)end;
}

// maybe should always do the heap check at the begining
void ck_mem_on(void) {
    assert(init_p && !check_on);
    check_on = 1;

    // set up memory for trampolines
    unsigned instr_count = __code_end__ - __code_start__;
    trampolines = kmalloc(instr_count*sizeof(trampoline_t));
    rewritten_instructions = kmalloc(instr_count);
    memset((void *)rewritten_instructions, need_to_rewrite, instr_count);

    // XXX
#if VERBOSE
    trace("gonna enable ints globally!\n");
    system_enable_interrupts();
    trace("enabled!\n");
#else
    system_enable_interrupts();
#endif
}

// maybe should always do the heap check at the end.
void ck_mem_off(void) {
    assert(init_p && check_on);

    // XXX
#if VERBOSE
    trace("gonna disable ints globally!\n");
    system_disable_interrupts();
    trace("disabled!\n");
#else
    system_disable_interrupts();
#endif

    check_on = 0;
}
