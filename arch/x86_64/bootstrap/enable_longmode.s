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
  /* C support code */
  push  %ebp
  mov   %esp, %ebp
  sub   $12,  %esp /* Sets up storage space for esi, edi, and the handled variables */
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

  /* Step 2 : Set Long Mode Enable bit in the EFER Model Specific Register*/
  mov   $0xc0000080, %ecx
  rdmsr
  bts   $8,   %eax
  wrmsr

  /* TODO : Enable paging (replace pml4_base)
  mov   pml4_base,    %eax
  mov   %eax, %cr3
  mov   %cr0, %eax
  bts   $31,  %eax
  mov   %eax, %cr0
  */

  /* Return 0 (This is a kernel replacement) */
  mov   $0, %eax

  /* End of the function */
  pop   %edi
  pop   %esi
  add   $12, %esp
  pop   %ebp
  ret

no_longmode:
  /* return -1 */
  pop   %edi
  pop   %esi
  add   $12, %esp
  mov   $-1, %eax
  pop   %ebp
  ret
