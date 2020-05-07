// fake memory.
//
// for the moment: simply print what put32 is writing, and return a random value for
// get32
#include "fake-pi.h"
#include "rpi.h"

unsigned get32(const volatile void *addr) {
    // you won't need this if you write fake versions of the timer code.
    // however, if you link in the raw timer.c from rpi, thne you'll need
    // to recognize the time address and returns something sensible.
    if ((uintptr_t)addr == 0x20003004) {
        fake_time_inc(1);
        trace("GET32(%p)=0x%x\n", addr, fake_time_usec);
        return fake_time_usec;
    }
    unsigned u = fake_random();
    trace("GET32(%p)=0x%x\n", addr, u);
    return u;
}

// for today: simply print (addr,val) using trace()
void put32(volatile void *addr, unsigned val) {
    trace("PUT32(%p)=0x%x\n", addr, val);
}
