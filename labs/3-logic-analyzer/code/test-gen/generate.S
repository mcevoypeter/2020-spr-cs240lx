.global generate

generate:
    push {r0-r5, lr}
    mov r3, r1  // N
    mov r4, r2  // ncycle
    mov r1, #0  // bit
    outer_loop:
        eor r1, r1, #1
        inner_loop:
            subs r4, r4, #1
            bne inner_loop
        mov r4, r2  // reset ncycle
        // bl gpio_write
        subs r3, r3, #1
        bne outer_loop
    pop {r0-r5, lr}
    bx lr
