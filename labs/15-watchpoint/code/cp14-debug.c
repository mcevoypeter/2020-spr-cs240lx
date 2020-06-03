// handle debug exceptions.
#include "rpi.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "cp14-debug.h"
#include "bit-support.h"

void dummy_interrupt_vector(unsigned pc) {
    panic("ERROR: undefined exception <interrupt> at PC=%x\n", pc);
}

void cp14_enable(void) {
    static int init_p = 0;

    if(!init_p) { 
        int_init();
        init_p = 1;
    }

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t dscr = cp14_dscr_get();
    dscr = bit_set(dscr, 15);  // set bit 15 to enable monitor debug mode
    dscr = bit_clr(dscr, 14);  // clear bit 14 to select monitor debug mode
    cp14_dscr_set(dscr);
    assert (cp14_dscr_get() == dscr);

    prefetch_flush();
    // assert(!cp14_watchpoint_occured());
}

// set the first watchpoint and call <handler> on debug fault: 13-47
static handler_t watchpt_handler0 = 0;
void watchpt_set0(uint32_t addr, handler_t handler) {
    // clear bit 0 of WCR to disable the watchpoint
    uint32_t wcr = cp14_wcr0_get();
    wcr = bit_clr(wcr, 0);
    cp14_wcr0_set(wcr); 

    // write the address to WVR
    cp14_wvr0_set(addr);
    assert(cp14_wvr0_get() == addr);
    
    // enable the watchpoint
    wcr = bit_clr(wcr, 20);             // set bit 20 of WCR to disable linking
    wcr = bits_set(wcr, 5, 8, 0b1111);  // set bits 5-8 of WCR to select addresses
    wcr = bits_set(wcr, 3, 4, 0b11);    // set bits 3-4 of WCR to allow loads and stores
    wcr = bits_set(wcr, 1, 2, 0b11);    // set bits 1-2 of WCR to allow user and privileged access
    wcr = bit_set(wcr, 0);              // set bit 0 of WCR to enable the watchpoint
    cp14_wcr0_set(wcr);
    assert(cp14_wcr0_get() == wcr);

    watchpt_handler0 = handler;
}

// check for watchpoint fault and call <handler> if so.
void data_abort_vector(unsigned pc) {
    static int nfaults = 0;
    printk("nfault=%d: data abort at %p\n", nfaults++, pc);
    if(datafault_from_ld())
        printk("was from a load\n");
    else
        printk("was from a store\n");
    if(!was_debug_datafault()) 
        panic("impossible: should get no other faults\n");

    // this is the pc
    printk("wfar address = %p, pc = %p\n", cp14_wfar_get()-8, pc);
    assert(cp14_wfar_get()-4 == pc);

    assert(watchpt_handler0);
    
    uint32_t addr = cp15_far_get();
    printk("far address = %p\n", addr);

    // should send all the registers so the client can modify.
    watchpt_handler0(0, pc, addr);
}

#define bvr_match (0b00 << 21)
#define bvr_mismatch (0b10 << 21)

static inline uint32_t brkpt_get_va0(void) {
    return cp14_bvr0_get();
}

static uint32_t brkpt_disable0(void) {
    // clear bit 0 of BCR to disable the breakpoint
    uint32_t bcr = cp14_bcr0_get();
    bcr = bit_clr(bcr, 0);
    cp14_bcr0_set(bcr); 
    assert(cp14_bcr0_get() == bcr);
    return bcr;
}

// 13-16
static handler_t brkpt_handler0 = 0;
void brkpt_set0(uint32_t addr, handler_t handler) {
    uint32_t bcr = brkpt_disable0();

    // write the address to BVR
    cp14_bvr0_set(addr);
    assert(cp14_bvr0_get() == addr);
    
    // enable the watchpoint
    bcr = bits_clr(bcr, 21, 22);            // set bits 21-22 of BCR to break on match
    bcr = bit_clr(bcr, 20);                 // set bit 20 of BCR to disable linking
    bcr = bits_clr(bcr, 14, 15);            // set bits 14-15 of BCR so breakpoint matches 
    bcr = bits_set(bcr, 5, 8, 0b1111);      // set bits 5-8 of BCR to select addresses
    bcr = bits_set(bcr, 1, 2, 0b11);        // set bits 1-2 of BCR to allow user and privileged access
    bcr = bit_set(bcr, 0);                  // set bit 0 of BCR to enable the watchpoint
    cp14_bcr0_set(bcr);
    assert(cp14_bcr0_get() == bcr);

    brkpt_handler0 = handler;
}


// if get a breakpoint call <brkpt_handler0>
void prefetch_abort_vector(unsigned pc) {
    printk("prefetch abort at %p\n", pc);
    if(!was_debug_instfault())
        panic("impossible: should get no other faults\n");
    assert(brkpt_handler0);
    brkpt_handler0(0, pc, cp15_ifar_get());
}
