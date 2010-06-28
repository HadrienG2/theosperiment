  .data
  .lcomm tmp_ebp, 4
  .lcomm tmp_esi, 4
  .lcomm tmp_edi, 4
  .lcomm tmp_ebx, 4
  .lcomm tmp_ecx, 4
  .lcomm tmp_edx, 4
  
  .text
  .globl enable_compatibility
  .globl run_kernel

enable_compatibility:
  mov   %ebp, tmp_ebp       /* Save caller's registers */
  mov   %esi, tmp_esi
  mov   %edi, tmp_edi
  mov   %ebx, tmp_ebx
  mov   %ecx, tmp_ecx
  mov   %edx, tmp_edx
  mov   %esp, %ebp

  /* Check if the CPU is CPUID-compatible by trying to toggle ID bit in EFLAGS */
  pushfl                  /* Copy EFLAGS to EAX */
  pop   %eax       
  mov   %eax, %ebx        /* Make a copy of EFLAGS to EBX */
  xor   $0x00200000, %eax /* Toggle bit 21 (ID) and save EAX in EFLAGS */
  push  %eax
  popfl
  pushfl                  /* Copy EFLAGS to EAX again and compare it with EBX */
  pop   %eax
  cmp   %ebx, %eax
  jz no_longmode          /* It bit 21 did not change, CPUID is not available, and longmode isn't either */ 
  
  /* Check long-mode support through CPUID (code from the AMD manual) */
  mov   $0x80000000, %eax
  cpuid
  cmp   $0x80000000, %eax
  jbe   no_longmode
  mov   $0x80000001, %eax
  cpuid
  bt    $29, %edx
  jnc   no_longmode 

  /* At this point, we know that long-mode support is available
     Step 1 : Enable PAE */
  mov   %cr4, %eax
  bts   $5,   %eax
  mov   %eax, %cr4

  /* Step 2 : Load CR3 value for future paging activation */
  mov   4(%ebp), %eax
  mov   %eax, %cr3

  /* Step 3 : Set LME and NXE bits in the EFER Model Specific Register */
  mov   $0xc0000080, %ecx
  rdmsr
  bts   $8,   %eax
  bts   $11,   %eax
  wrmsr

  /* Prepare CR0 value in EAX, then enable long mode (we'll be in 32-bit compatibility mode at this point)  */
  mov   %cr0, %eax
  bts   $31,  %eax
  mov   %eax, %cr0
  ljmp $8, $compatibility_mode
compatibility_mode:
  mov   $0, %eax
  jmp   return

return:
  /* End of the function */
  mov   %ebp, %esp
  mov   tmp_edx, %edx
  mov   tmp_ecx, %ecx
  mov   tmp_ebx, %ebx
  mov   tmp_edi, %edi
  mov   tmp_esi, %esi
  mov   tmp_ebp, %ebp
  ret

no_longmode:
  /* return -1 */
  mov   $-1, %eax
  jmp return

run_kernel:
  mov   %ebp, tmp_ebp       /* Save caller's registers */
  mov   %esi, tmp_esi
  mov   %edi, tmp_edi
  mov   %ebx, tmp_ebx
  mov   %ecx, tmp_ecx
  mov   %edx, tmp_edx
  mov   %esp, %ebp

  /* Long-jump to kernel at 4(%ebp),
     while giving it access to information at 8(%ebp) */
  mov 8(%ebp), %ecx
  mov 4(%ebp), %ebx

  ljmp $24, $trampoline
.code64
trampoline:
  mov $32, %dx
  mov %dx, %ds
  mov %dx, %es
  mov %dx, %fs
  mov %dx, %gs
  mov %dx, %ss
  call *%rbx
  
kernel_returns:
  xchg %bx, %bx
  jmp  kernel_returns
  