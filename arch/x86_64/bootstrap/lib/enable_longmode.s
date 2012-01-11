    .data
    .lcomm tmp_ebp, 4
    .lcomm tmp_ecx, 4
    .lcomm tmp_edx, 4

    .text
    .globl enable_longmode

enable_longmode:
    mov     %ebp, (tmp_ebp) /* Save caller's registers */
    mov     %ecx, (tmp_ecx)
    mov     %edx, (tmp_edx)
    mov     %esp, %ebp

    /* At this point, we know that long-mode support is available
         Step 1 : Enable PAE */
    mov     %cr4, %eax
    bts     $5,     %eax
    mov     %eax, %cr4

    /* Step 2 : Load CR3 value */
    mov     4(%ebp), %eax
    mov     %eax, %cr3

    /* Step 3 : Set LME and NXE bits in the EFER Model Specific Register */
    mov     $0xc0000080, %ecx
    rdmsr
    bts     $8,     %eax
    bts     $11,     %eax
    wrmsr

    /* Enable paging, then enable long mode (we'll be in 32-bit compatibility mode at this point)    */
    mov     %cr0, %eax
    bts     $31,    %eax
    mov     %eax, %cr0
    ljmp $24, $compatibility_mode
compatibility_mode:
    mov     $0, %eax

return:
    /* End of the function */
    mov     %ebp, %esp
    mov     (tmp_edx), %edx
    mov     (tmp_ecx), %ecx
    mov     (tmp_ebp), %ebp
    ret
