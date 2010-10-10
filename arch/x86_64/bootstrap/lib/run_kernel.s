  .text
  .globl run_kernel

run_kernel:
  /* No need to save caller registers here : we won't ever return... */
  mov   %esp, %ebp

  /* Reset the stack */
  movl $end_stack, %esp
  sub  $0x1000, %esp
  /* Kernel address is at 4(%ebp), kernel information at 8(%ebp) */
  mov 8(%ebp), %ecx
  mov 4(%ebp), %ebx
  ljmp $8, $trampoline
  .code64
trampoline:
  call *%rbx
  
kernel_returns:
  xchg %bx, %bx
  hlt
  jmp  kernel_returns
