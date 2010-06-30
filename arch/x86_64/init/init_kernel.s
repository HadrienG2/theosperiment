  .data
  .lcomm tmp_rbp, 8
  .lcomm tmp_rsi, 8
  .lcomm tmp_rdi, 8
  .lcomm tmp_rbx, 8
  .lcomm tmp_rcx, 8
  .lcomm tmp_rdx, 8

  .text
  .globl kinit

kinit:
  mov   %rbp, tmp_rbp       /* Save caller's registers */
  mov   %rsi, tmp_rsi
  mov   %rdi, tmp_rdi
  mov   %rbx, tmp_rbx
  mov   %rcx, tmp_rcx
  mov   %rdx, tmp_rdx
  mov   %rsp, %rbp
  
  /* Run constructors */
  mov  $start_ctors, %rbx
  jmp  ctors_check
ctors_run:
  call *(%rbx)
  add  $8, %rbx
ctors_check:
  cmp  $end_ctors, %rbx
  jb   ctors_run

  /* call kernel */
  push tmp_rcx
  call kmain

  /* Run destructors */
  mov  $start_dtors, %rbx
  jmp  dtors_check
dtors_run:
  call *(%rbx)
  add  $8, %rbx
dtors_check:
  cmp  $end_dtors, %rbx
  jb   dtors_run

return:
  /* Go back to bootstrap kernel, let it decide what to do */
  mov   %rbp, %rsp
  mov   tmp_rdx, %rdx
  mov   tmp_rcx, %rcx
  mov   tmp_rbx, %rbx
  mov   tmp_rdi, %rdi
  mov   tmp_rsi, %rsi
  mov   tmp_rbp, %rbp
  ret
