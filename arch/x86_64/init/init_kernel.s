  .text
  .globl kinit

kinit:
  /* Prepare stack smashing protection */
  call __stack_chk_guard_setup

  /* Run constructors */
  mov  $start_ctors, %ebx
  jmp  2f
1:
  call *(%ebx)
  add  $4, %ebx
2:
  cmp  $end_ctors, %ebx
  jb   1b

  /* call kernel */
  call kmain

  /* Run destructors */
  mov  $start_dtors, %ebx
  jmp  4f
3:
  call *(%ebx)
  add  $4, %ebx
4:
  cmp  $end_dtors, %ebx
  jb   3b

  /* Go back to bootstrap kernel, let it decide what to do */
  ret
