#ifndef __CP14_DEBUG_H__
#define __CP14_DEBUG_H__


// flush prefetch buffer: 3-79
static inline void prefetch_flush(void) {
    uint32_t r = 0;
    asm volatile("mcr p15, 0, %[val], c7, c5, 4" :: [val] "r" (r));
}

struct debug_id {
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register pairs
                                // add 1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// DIDR: 13-5, 13-26
static inline uint32_t cp14_didr_get(void) {
    uint32_t didr;
    asm volatile("mrc p14, 0, %[result], c0, c0, 0" : [result] "=r" (didr) ::);
    return didr;
}

// DSCR: 13-7, 13-26
static inline uint32_t cp14_dscr_get(void) {
    uint32_t dscr;
    asm volatile("mrc p14, 0, %[result], c0, c1, 0" : [result] "=r" (dscr) ::);
    return dscr;
}
static inline void cp14_dscr_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c1, 0" :: [val] "r" (r));
    prefetch_flush();
}

// WFAR: 13-12, 13-26
static inline uint32_t cp14_wfar_get(void) {
    uint32_t wfar;
    asm volatile("mrc p14, 0, %[result], c0, c6, 0" : [result] "=r" (wfar) ::);
    return wfar;
}

// WVR0: 13-20, 13-26
static inline uint32_t cp14_wvr0_get(void) { 
    uint32_t wvr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 6" : [result] "=r" (wvr0) ::);
    return wvr0;
}
static inline void cp14_wvr0_set(uint32_t r) { 
    asm volatile("mcr p14, 0, %[val], c0, c0, 6" :: [val] "r" (r));
    prefetch_flush();
}

// WCR0: 13-21, 13-26
static inline uint32_t cp14_wcr0_get(void) {
    uint32_t wcr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 7" : [result] "=r" (wcr0) ::);
    return wcr0;
}
static inline void cp14_wcr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 7" :: [val] "r" (r));
    prefetch_flush();
}

// BVR0: 13-16, 13-26
static inline uint32_t cp14_bvr0_get(void) { 
    uint32_t bvr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 4" : [result] "=r" (bvr0) ::);
    return bvr0;
}
static inline void cp14_bvr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 4" :: [val] "r" (r));
    prefetch_flush();
}

// BCR0: 13-17, 13-26
static inline uint32_t cp14_bcr0_get(void) {
    uint32_t bcr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 5" : [result] "=r" (bcr0) ::);
    return bcr0;
}
static inline void cp14_bcr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 5" :: [val] "r" (r));
    prefetch_flush();
}

// DFSR: 3-64
static inline uint32_t cp15_dfsr_get(void) {
    uint32_t dfsr;
    asm volatile("mrc p15, 0, %[result], c5, c0, 0" : [result] "=r" (dfsr) ::);
    return dfsr;
}

// 3-68: fault address register: hold the MVA that the fault occured at.
static inline uint32_t cp15_far_get(void) {
    uint32_t far;
    asm volatile("mrc p15, 0, %[result], c6, c0, 0" : [result] "=r" (far) ::); 
    return far;
}

// ISFR: 3-66 (hold source of last instruction)
static inline uint32_t cp15_ifsr_get(void) {
    uint32_t ifsr;
    asm volatile("mrc p15, 0, %[result], c5, c0, 1" : [result] "=r" (ifsr) ::);
    return ifsr;
}

// IFAR: 3-69 (hold address of function that caused prefetch fault)
static inline uint32_t cp15_ifar_get(void) {
    uint32_t ifar;
    asm volatile("mrc p15, 0, %[result], c6, c0, 1" : [result] "=r" (ifar) ::);
    return ifar;
}


#include "bit-support.h"

// reason for watchpoint debug fault: 3-64 
static inline unsigned datafault_from_ld(void) {
    uint32_t dfsr = cp15_dfsr_get();
    // TODO: also check `was_debug_datafault`
    return !bit_isset(dfsr, 11);
}
static inline unsigned datafault_from_st(void) {
    return !datafault_from_ld();
}

// was this an instruction debug event fault?: 3-65
static inline unsigned was_debugfault_r(uint32_t r) {
    return !bit_isset(r, 10) && bits_get(r, 0, 3) == 0b10;
}

// was this a debug data fault?
static inline unsigned was_debug_datafault(void) {
    unsigned dfsr = cp15_dfsr_get();
    if(!was_debugfault_r(dfsr))
        return 0;
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
    uint32_t dscr = cp14_dscr_get();
    uint32_t source = bits_get(dscr, 2, 5);
    return source == 0b10;
}

// was this a debug instruction fault?
static inline unsigned was_debug_instfault(void) {
    uint32_t ifsr = cp15_ifsr_get();
    if(!was_debugfault_r(ifsr))
        panic("impossible: should only get datafaults\n");
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
    uint32_t dscr = cp14_dscr_get();
    uint32_t source = bits_get(dscr, 2, 5);
    return source == 0b1;
}

// client supplied fault handler: give a pointer to the registers so 
// the client can modify them (for the moment pass NULL)
//  - <pc> is where the fault happened, 
//  - <addr> is the fault address.
typedef void (*handler_t)(uint32_t regs[16], uint32_t pc, uint32_t addr);

// enable the co-processor.
void cp14_enable(void);

// set a watchpoint at <addr>: calls <handler> with a pointer to the registers.
void watchpt_set0(uint32_t addr, handler_t watchpt_handle);

// set a breakpoint at <addr>: call handler when the fault happens.
void brkpt_set0(uint32_t addr, handler_t brkpt_handler);


/********************************************************************
 ********************************************************************
 * lab-16: single-stepping.
 */


/********************************************************************
 * 
 * part 1: trivial single step.
 *  16-part1-test1. 16-part1-test2.c 16-part1-test3.c
 */

#include "cpsr-util.h"

// simple debug macro: can turn it off/on by calling <brk_verbose({0,1})>
#define brk_debug(args...) if(brk_verbose_p) debug(args)

extern int brk_verbose_p;
static inline void brk_verbose(int on_p) { brk_verbose_p = on_p; }

// Set:
//  - cpsr to <cpsr> 
//  - sp to <stack_addr>
// and then call <fp>.
//
// Does not return.
//
// used to get around the problem that if we switch to USER_MODE in C code,
// it will not have a stack pointer setup, which will cause all sorts of havoc.
void user_trampoline_no_ret(uint32_t cpsr, void (*fp)(void));

// set a mismatch on <addr> --- call <handler> on mismatch.
// NOTE:
//  - an easy way to mismatch the next instruction is to call with
//    use <addr>=0.
//  - you cannot get mismatches in "privileged" modes (all modes other than
//    USER_MODE)
//  - once you are in USER_MODE you cannot switch modes on your own since the 
//    the required "msr" instruction will be ignored.  if you do want to 
//    return from user space you'd have to do a system call ("swi") that switches.
void brkpt_mismatch_set0(uint32_t addr, handler_t handler);

/********************************************************************
 * 
 * part 2: concurrency checking.
 *  16-part2-test1. 16-part2-test2.c 16-part2-test3.c
 */


// disable mismatch breakpoint <addr>: error if doesn't exist.
void brkpt_mismatch_disable0(uint32_t addr);

// get the saved status register.
static inline uint32_t spsr_get(void) {
    uint32_t spsr;
    asm volatile("msr spsr, %[result]" : [result] "=r" (spsr) ::);
    return spsr;
}
// set the saved status register.
static inline void spsr_set(uint32_t spsr) {
    asm volatile("msr spsr, %[val]" :: [val] "r" (spsr));
    prefetch_flush();
}


// same as user_trampoline_no_ret except:
//  -  it returns to the caller with the cpsr set correctly.
//  - it calls <fp> with a user-supplied pointer.
//  16-part2-test1.c
void user_trampoline_ret(uint32_t cpsr, void (*fp)(void *handle), void *handle);

#endif
