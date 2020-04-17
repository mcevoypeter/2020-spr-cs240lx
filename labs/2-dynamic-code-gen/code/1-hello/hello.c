#include "rpi.h"
#include "../unix-side/armv6-insts.h"

void hello(void) { 
    printk("hello world! my name is peter\n");
}

// i would call this instead of printk if you have problems getting
// ldr figured out.
void foo(int x) { 
    printk("foo was passed %d\n", x);
}

void notmain(void) {
    // generate a dynamic call to hello world.
    // 1. you'll have to save/restore registers
    // 2. load the string address [likely using ldr]
    // 3. call printk
    static uint32_t code[16];
    unsigned n = 0;
    // `push {r3, lr}`
    code[n++] = arm_stm(arm_DB, arm_incr_base, arm_sp, (1 << arm_lr | 1 << arm_r3)); 

    // `mov r0, #9` - for `foo`
    // code[n++] = arm_mov_imm(arm_r0, 9);
    
    // `ldr r0, [pc, #4]` - for `printk`
    code[n] = arm_ldr_imm(arm_r0, arm_pc, 4);
    n++;
    // `bl 0 <printk>`
    code[n] = arm_bl((int32_t)&code[n], (int32_t)printk);  
    n++;
    // `pop {r3, pc}`
    code[n++] = arm_ldm(arm_IA, arm_incr_base, arm_sp, (1 << arm_pc | 1 << arm_r3));
    // any string
    code[n++] = (uint32_t)"hello, world! this is a string\n\0";

    printk("emitted code:\n");
    for(int i = 0; i < n; i++) 
        printk("code[%d]=0x%x\n", i, code[i]);

    void (*fp)(void) = (typeof(fp))code;
    printk("about to call: %x\n", fp);
    printk("--------------------------------------\n");
    fp();
    printk("--------------------------------------\n");
    printk("success!\n");
    clean_reboot();
}
