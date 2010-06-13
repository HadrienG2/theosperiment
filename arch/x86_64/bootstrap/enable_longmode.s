/* TODO :
    In current 32-bit code, switch to 64-bit mode and run kernel...
      -> Create a 64-bit page table and use it
      -> Long jump to 64-bit code.
    In kernel startup code, make sure that everything is ready for 64-bit, including...
      -> Descriptor table hell : GDT, LDT, TSS
      -> Stack-related GPRs : RBP, RSP (hmmm... nan, en fait je pense qu'il faut les laisser où ils sont)
      -> Segment registers : CS, FS, GS */
  .text
  .globl run_kernel

run_kernel:
  push  %ebp       /* Save caller's stack frame and establish a new one */
  mov   %esp, %ebp 
  sub   $32,  %esp /* Sets up storage space for 32 bytes */
  push  %esi
  push  %edi

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
  mov   8(%ebp), %eax
  mov   %eax, %cr3

  /* Step 3 : Set LME and NXE bits in the EFER Model Specific Register */
  mov   $0xc0000080, %ecx
  rdmsr
  bts   $8,   %eax
  bts   $11,   %eax
  wrmsr

  /* Prepare CR0 value in EAX and entry point adress in EBX */
  mov   12(%ebp), %ebx
  mov   %cr0, %eax
  bts   $31,  %eax
 
  /* Start the kernel */
  mov   %eax, %cr0
  ljmp   *(%ebx)

return:
  /* End of the function */
  pop   %edi
  pop   %esi
  mov   %ebp, %esp
  pop   %ebp
  ret

no_longmode:
  /* return -1 */
  mov   $-1, %eax
  jmp return
