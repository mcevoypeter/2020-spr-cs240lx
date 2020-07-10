#ifndef __SYS_CALL_H__
#define __SYS_CALL_H__

int user_mode_run_fn(int (*fn)(void), unsigned user_stack);

int sys_10_asm(void);
#endif
