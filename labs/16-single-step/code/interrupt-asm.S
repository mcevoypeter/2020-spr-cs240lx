/*
 * interrupt-asm.s
 *
 * Code for interrupt handling.  Refer to armisa.pdf in docs/ for what
 * the opcodes mean.
 */
#include "rpi-asm.h"

.globl _interrupt_table
.globl _interrupt_table_end
_interrupt_table:
  ldr pc, _reset_asm
  ldr pc, _undefined_instruction_asm
  ldr pc, _software_interrupt_asm
  ldr pc, _prefetch_abort_asm
  ldr pc, _data_abort_asm
  ldr pc, _reset_asm
  ldr pc, _interrupt_asm
fast_interrupt_asm:
  mov sp, #INT_STACK_ADDR 
  sub   lr, lr, #4
  bl    fast_interrupt_vector  

_reset_asm:                   .word reset_asm
_undefined_instruction_asm:   .word undefined_instruction_asm
_software_interrupt_asm:      .word software_interrupt_asm
_prefetch_abort_asm:          .word prefetch_abort_asm
_data_abort_asm:              .word data_abort_asm
_interrupt_asm:               .word interrupt_asm
_interrupt_table_end:


@ you need to look up in the arm pdf what the state of lr is for each.
@ sometimes you need to subtact 4, sometimes 8.
reset_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    reset_vector
undefined_instruction_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    undefined_instruction_vector
software_interrupt_asm:
  push {r4-r12, lr}
  mov r1, r0
  sub   r0, lr, #4
  bl    syscall_vector
  pop {r4-r12, lr}      @ don't pop r0 because we need the return value from syscall_vector
  movs pc, lr
prefetch_abort_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  push {r0-r12, lr}
  mov r0, lr
  bl    prefetch_abort_vector
  pop {r0-r12, lr}
  movs pc, lr
data_abort_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #8
  push {r0-r12, lr}
  mov r0, lr
  bl    data_abort_vector
  pop {r0-r12, lr}
  movs pc, lr
interrupt_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4
  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs
  mov   r0, lr              @ Pass old pc
  bl    irq_vector          @ C function
  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers
  @ return from interrupt handler: will re-enable general ints.
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.

@ arguments:
@   r0 = the cpsr value to set.
@   r1 = the routine to call
MK_FN(user_trampoline_no_ret)
    msr cpsr, r0    @ set cpsr - switches to user mode
    mov r3, #0
    mcr p15, 0, r3, c7, c5, 4   @ prefetch flush
    mov sp, #STACK_ADDR2        @ use separate stack for user mode 
    mov pc, r1      
    @ fail if they return.
    bl   brk_no_ret_error


@ arguments:
@   r0 = the cpsr value to set.
@   r1 = the routine to call
@   r2 = the handle pointer to pass to the call <r1>
@
@ also: move the current mode's stack pointer to the next mode's
@ sp
@
@ maybe we should just have a stack pointer switch routine?   the
@ trouble with this is that we need to do a system call to change
@ to super mode.  i don't know if this is the right thing.
MK_FN(user_trampoline_ret)
    push {r0-r12, lr}
    mrs r4, cpsr        @ save super mode cspr
    msr cpsr, r0        @ set cpsr - switches to user mode
    mov r5, #0
    mcr p15, 0, r5, c7, c5, 4   @ prefetch flush
    mov sp, #STACK_ADDR2        @ use separate stack for user mode

    mov r0, r2          @ pass routine the handle pointer
    blx r1              @ call routine
    mov r0, r4          @ give syscall handler the cpsr value to set
    swi 1 
    pop {r0-r12, lr}
    bx lr

 @ arguments:
 @   r0 = address of the lock 
 MK_FN(sys_try_lock_asm)
    push {r4-r12, lr}
    swi 2
    pop {r4-r12, lr}
    bx lr
