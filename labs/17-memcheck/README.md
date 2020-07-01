Today is going to be more of a pre-lab to try to get pieces in place
so we can do the valgrind checker we keep talking about.

You'll be doing two things:
  1. Refactoring your debugging code so you have a simple, clean function
     you can call to run a single routine in single-step mode.
  2. Setting up a virtual memory handler that can detect when you get 
     a domain fault and resume.


Next time we'll combine these two so we can automatically trap when a 
memory location is referenced.

Unfortunately, I mis-estimated how long this was going to take, so I'm 
not finished with the code.  Hopefully since this is mostly refectoring
we can work around that.  

### Part 1: implement stand-alone single stepping.

Make two routines (in `part1/single-step.h`)

    // initialize single step mode.
    int single_step_init(void);

    // run a single routine <user_fn> in single step mode using
    // stack <sp>
    int single_step_run(int user_fn(void), uint32_t sp);

You'll have to pull in your single-stepping code from lab 16.

I'll check in a couple of tests in part1.

### Part 2: implement virtual memory trapping

The idea here it to register a region of memory so we get a trap on any load or
store and then in the fault handle, remove the trap.  We do this by associating
this memory with its own domain (`track_id`, which is 2) and setting the pemissions
for this id to no-access to get traps, and to all access to not get traps.

You'll have to modify 'part2-vm-trap/memcheck.c'.
Implement:
  1. `dom_perm_set` (easy).
  2. `data_abort_vecto` involves computing the fault reason and then acting.

Just chase through the four tests.

You'll need to pull in:
   - cp14-debug.h
   - cpsr-util.h
   - interrupts-asm.S
