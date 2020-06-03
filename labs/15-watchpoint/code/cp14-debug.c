// handle debug exceptions.
#include "rpi.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "cp14-debug.h"
#include "bit-support.h"

/******************************************************************
 * part1 set a watchpoint on 0.
 )*/
static handler_t watchpt_handler0 = 0;

// set the first watchpoint and call <handler> on debug fault: 13-47
void watchpt_set0(void *_addr, handler_t handler) {
    // clear bit 0 of WCR to disable the watchpoint
    uint32_t wcr = cp14_wcr0_get();
    wcr = bit_clr(wcr, 0);
    cp14_wcr0_set(wcr); 

    // write the address to WVR
    cp14_wvr0_set((uint32_t)_addr << 2);
    // XXX: do I need a prefetch flush here?
    
    // enable the watchpoint
    wcr = bit_clr(wcr, 20);         // set bit 20 of WCR to disable linking
    wcr = bits_set(wcr, 5, 8, 0xf); // set bits 5-8 of WCR to select addresses
    wcr = bits_set(wcr, 3, 4, 0x3); // set bits 3-4 of WCR to allow loads and stores
    wcr = bits_set(wcr, 1, 2, 0x3); // set bits 1-2 of WCR to allow user and privileged access
    wcr = bit_set(wcr, 0);          // set bit 0 of WCR to enable the watchpoint
    cp14_wcr0_set(wcr);
    prefetch_flush();

    watchpt_handler0 = handler;
}

// check for watchpoint fault and call <handler> if so.
void data_abort_vector(unsigned pc) {
    printk("!!!\n");
    clean_reboot();
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
    
    uint32_t addr = far_get();
    printk("far address = %p\n", addr);

    // should send all the registers so the client can modify.
    watchpt_handler0(0, pc, addr);
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
    dscr = bit_set(dscr, 15);   // set bit 15 to enable monitor debug mode
    dscr = bit_clr(dscr, 14);   // clear bit 14 to select monitor debug mode

    prefetch_flush();
    // assert(!cp14_watchpoint_occured());

}

/**************************************************************
 * part 2
 */

static handler_t brkpt_handler0 = 0;

static inline uint32_t cp14_bvr0_get(void) { unimplemented(); }
static inline void cp14_bvr0_set(uint32_t r) { unimplemented(); }
static inline uint32_t cp14_bcr0_get(void) { unimplemented(); }
static inline void cp14_bcr0_set(uint32_t r) { unimplemented(); }

static unsigned bvr_match(void) { return 0b00 << 21; }
static unsigned bvr_mismatch(void) { return 0b10 << 21; }

static inline uint32_t brkpt_get_va0(void) {
    return cp14_bvr0_get();
}
static void brkpt_disable0(void) {
    unimplemented();
}

// 13-16
// returns the 
void brkpt_set0(uint32_t addr, handler_t handler) {
    unimplemented();
}

// if get a breakpoint call <brkpt_handler0>
void prefetch_abort_vector(unsigned pc) {
    printk("prefetch abort at %p\n", pc);
    if(!was_debug_instfault())
        panic("impossible: should get no other faults\n");
    assert(brkpt_handler0);
    brkpt_handler0(0, pc, ifar_get());
}
