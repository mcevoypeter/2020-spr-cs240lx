/*************************************************************************
 * engler, cs240lx: Purify/Boehm style leak checker/gc starter code.
 *
 * We've made a bunch of simplifications.  Given lab constraints:
 * people talk about trading space for time or vs. but the real trick 
 * is how to trade space and time for less IQ needed to see something 
 * is correct. (I haven't really done a good job of that here, but
 * it's a goal.)
 *
 */
#include "rpi.h"
#include "rpi-constants.h"
#include "ckalloc-internal.h"

/****************************************************************
 * code you implement is below.
 */

static struct heap_info info;

// returns pointer to the first header block.
hdr_t *ck_first_hdr(void) {
    hdr_t *h = info.heap_start;
    assert(h);
    assert(check_hdr(h));
    return h;
}

// returns pointer to next hdr or 0 if none.
hdr_t *ck_next_hdr(hdr_t *p) {
    hdr_t *next = (hdr_t *)((char *)b_rz2_ptr(p) + b_rz2_nbytes(p));
    if ((void *)next >= info.heap_end)
        return 0;
    assert(check_hdr(next));
    return next;
}

// quick check that the pointer is between the start of
// the heap and the last allocated heap pointer.  saves us 
// walk through all heap blocks.
//
// we could warn if the pointer is within some amount of slop
// so that you can detect some simple overruns?
static int in_heap(void *p) {
    return info.heap_start <= p && p <= info.heap_end;
}

// given potential address <addr>, returns:
//  - 0 if <addr> does not correspond to an address range of 
//    any allocated block.
//  - the associated header otherwise (even if freed: caller should
//    check and decide what to do in that case).
//
// XXX: you'd want to abstract this some so that you can use it with
// other allocators.  our leak/gc isn't really allocator specific.
static hdr_t *is_ptr(uint32_t addr) {
    void *p = (void*)addr;
    
    if(!in_heap(p))
        return 0;

    hdr_t *h = ck_first_hdr();
    while ((void *)h < p) {
        void *payload_start = b_alloc_ptr(h);
        void *payload_end = b_rz2_ptr(h);
        if (payload_start <= p && p < payload_end)
            return h;
        h = ck_next_hdr(h);
    }
    // output("could not find pointer %p\n", addr);
    return 0;
}

// mark phase:
//  - iterate over the words in the range [p,e], marking any block 
//    potentially referenced.
//  - if we mark a block for the first time, recurse over its memory
//    as well.
//
// NOTE: if we have lots of words, could be faster with shadow memory / a lookup
// table.  however, given our small sizes, this stupid search may well be faster :)
// If you switch: measure speedup!
//
#include "libc/helper-macros.h"
static void mark(uint32_t *p, uint32_t *e) {
    assert(p<e);
    // maybe keep this same thing?
    assert(aligned(p,4));
    assert(aligned(e,4));

    hdr_t *h;
    for (uint32_t *w = p; w <= e; w++) {
        // first time marking this block
        if ((h = is_ptr(*w)) && h->mark == 0) {
            h->mark = 1;
            // recursively mark all pointers in this block
            // we'll be conservative and include the first word of the redzone
            // just to be safe
            mark(b_alloc_ptr(h), (uint32_t *)b_rz2_ptr(h));
        }
    }
}


// do a sweep, warning about any leaks.
//
//
static unsigned sweep_leak(int warn_no_start_ref_p) {
	unsigned nblocks = 0, errors = 0, maybe_errors=0;
	output("---------------------------------------------------------\n");
	output("checking for leaks:\n");

    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h), nblocks++) {
        // this block wasn't leaked/can't be collected
        if (h->mark) {
            h->mark = 0;
        } else if (h->state == ALLOCED) {
            ckfree(b_alloc_ptr(h));  
        }
    }

	trace("\tGC:Checked %d blocks.\n", nblocks);
	if(!errors && !maybe_errors)
		trace("\t\tGC:SUCCESS: No leaks found!\n");
	else
		trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n", 
						errors, maybe_errors);
	output("----------------------------------------------------------\n");
	return errors + maybe_errors;
}

// write the assembly to dump all registers.
// need this to be in a seperate assembly file since gcc 
// seems to be too smart for its own good.
void dump_regs(uint32_t *v, ...);

// a very slow leak checker.
static void mark_all(void) {
    // get start and end of heap so we can do quick checks
    info = heap_info();

    // slow: should not need this: remove after your code
    // works.
    for(hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        h->mark = h->refs_start = h->refs_middle = 0;
    }

	// pointers can be on the stack, in registers, or in the heap itself.

    // get all the registers.
    uint32_t regs[16];
    // should kill caller-saved registers
    dump_regs(regs);
    assert(regs[0] == (uint32_t)&regs[0]);

    mark(regs, &regs[14]);

    // mark the stack: we are assuming only a single
    // stack.  note: stack grows down.
    uint32_t *stack_top = (void*)STACK_ADDR;
    // conservatively assume that the stack pointer points to the bottom-most
    // word on the stack rather than one word below the bottom-most word
    uint32_t *stack_bottom = (uint32_t *)regs[13];
    mark(stack_bottom, stack_top);

    // these symbols are defined in our memmap
	extern uint32_t __bss_start__, __bss_end__;
	mark(&__bss_start__, &__bss_end__);

	extern uint32_t __data_start__, __data_end__;
	mark(&__data_start__, &__data_end__);
}


/***********************************************************
 * testing code: we give you this.
 */

// return number of bytes allocated?  freed?  leaked?
// how do we check people?
unsigned ck_find_leaks(int warn_no_start_ref_p) {
    mark_all();
    return sweep_leak(warn_no_start_ref_p);
}

// used for tests.  just keep it here.
void check_no_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");
    if(ck_find_leaks(1))
        panic("GC: should have no leaks!\n");
    else
        trace("GC: SUCCESS: no leaks!\n");
    gcc_mb();
}

// used for tests.  just keep it here.
unsigned check_should_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");

    unsigned nleaks = ck_find_leaks(1);
    if(!nleaks)
        panic("GC: should have leaks!\n");
    else
        trace("GC: SUCCESS: found %d leaks!\n", nleaks);
    gcc_mb();
    return nleaks;
}

void implement_this(void) { 
    panic("did not implement dump_regs!\n");
}

// similar to sweep_leak: but mark unreferenced blocks as FREED.
static unsigned sweep_free(void) {
	unsigned nblocks = 0, nfreed=0, nbytes_freed = 0;
	output("---------------------------------------------------------\n");
	output("compacting:\n");

    unimplemented();

    // i used this when freeing.
    // trace("GC:FREEing ptr=%x\n", ptr);
    // ckfree(ptr);

	trace("\tGC:Checked %d blocks, freed %d, %d bytes\n", nblocks, nfreed, nbytes_freed);

    struct heap_info info = heap_info();
	trace("\tGC:total allocated = %d, total deallocated = %d\n", 
                                info.nbytes_alloced, info.nbytes_freed);
	output("----------------------------------------------------------\n");
    return nbytes_freed;
}

unsigned ck_gc(void) {
    mark_all();
    unsigned nbytes = sweep_free();

    // perhaps coalesce these and give back to heap.  will have to modify last.

    return nbytes;
}
