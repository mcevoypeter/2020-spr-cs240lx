#ifndef __SYS_CALL_H__
#define __SYS_CALL_H__

// assembly routine: run <fn> at user level with stack <sp>
//    XXX: visible here just so we can use it for testing.
int user_mode_run_fn(int (*fn)(void), uint32_t sp);

int sys_10_asm(void);

int shadow_mem_set(uint32_t addr, uint32_t n, uint32_t state);
#endif
