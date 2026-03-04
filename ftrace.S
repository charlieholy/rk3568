 * Gcc with -pg will put the following code in the beginning of each function:
 *      mov x0, x30
 *      bl _mcount
 *	[function's body ...]
 * "bl _mcount" may be replaced to "bl ftrace_caller" or NOP if dynamic
 * ftrace is enabled.



 静态 ftrace（老式，少用）
CONFIG_DYNAMIC_FTRACE=n 时生效：
asm
ENTRY(_mcount)
	mcount_enter        // 保存栈
	ldr_l	x2, ftrace_trace_function  // 加载追踪函数
	blr	x2              // 调用ftrace追踪逻辑
	mcount_exit         // 恢复栈，返回原函数
ENDPROC(_mcount)

劫持函数的返回地址（LR/x30）
调用函数时，把原 LR 改成 return_to_handler
函数执行完 ret，不会直接返回，而是先跑 return_to_handler
最后再跳回原 LR
asm
// 1. 劫持返回地址
ENTRY(ftrace_graph_caller)
	bl	prepare_ftrace_return  // 修改栈上的LR值
	mcount_exit
ENDPROC(ftrace_graph_caller)

// 2. 函数返回时先执行这里
ENTRY(return_to_handler)
	save_return_regs          // 保存x0-x7
	bl	ftrace_return_to_handler  // 执行返回追踪
	mov	x30, x0               // 恢复原LR
	restore_return_regs       // 恢复寄存器
	ret                       // 真正返回调用者
END(return_to_handler)

entry-ftrace.S = 内核函数的监控器
_mcount/ftrace_caller = 函数调用时的钩子
return_to_handler = 函数返回时的钩子
基于 GCC -pg 实现，动态 patch 保证高性能，不影响内核正常运行
