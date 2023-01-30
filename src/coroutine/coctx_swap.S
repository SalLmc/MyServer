.globl coctx_swap
.type coctx_swap,@function

coctx_swap:
	leaq 8(%rsp),%rax	// [%rsp]==ret addr

	leaq 112(%rdi),%rsp // %rdi(first param)
	
	pushq %rax	// regs[13]
	pushq %rbx  
	pushq %rcx  
	pushq %rdx  

	pushq -8(%rax) // ret addr

	pushq %rsi   
	pushq %rdi  
	pushq %rbp  
	pushq %r8   
	pushq %r9   
	pushq %r12  
	pushq %r13  
	pushq %r14  
	pushq %r15  

	movq %rsi, %rsp	// %rsi(second param)

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r9
	popq %r8
	popq %rbp
	popq %rdi
	popq %rsi
	popq %rax 
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rsp //pop previously saved rsp+8 to rsp so that rsp remains the same after pushing the ret addr of next coroutine

	pushq %rax 
	
	xorl %eax, %eax
	ret