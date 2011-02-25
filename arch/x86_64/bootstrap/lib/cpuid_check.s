  .data
  .lcomm tmp_ebp, 4
  .lcomm tmp_ebx, 4
  
  .text
  .globl cpuid_check
  
cpuid_check:
  mov   %ebp, (tmp_ebp)       /* Save caller's registers */
  mov   %ebx, (tmp_ebx)
  mov   %esp, %ebp

  pushf
  pop   %eax
  mov   %eax, %ebx
  xor   $0x00200000, %eax
  push  %eax
  popf
  pushf
  pop   %eax
  cmp   %ebx, %eax
  jz    no_cpuid
  mov  $1, %eax

return:
  /* End of the function */
  mov   %ebp, %esp
  mov   (tmp_ebx), %ebx
  mov   (tmp_ebp), %ebp
  ret
  
no_cpuid:
  movl  $0, %eax
  jmp return
