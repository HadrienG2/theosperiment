ENTRY(bootstrap)

SECTIONS {
  . = 0x00100000; /* Bootstrap kernel loaded at 1MB */
  sbs_kernel = .;

  .text : {
    bootstrap/arch/i686/bs_multiboot.bsasm.o(.text) /* Multiboot header put as early as possible */
    *(.text)
  }

  .rodata ALIGN (0x1000) : {
    *(.rodata)
  }

  .data ALIGN (0x1000) : {
    *(.data)
  }

  .bss ALIGN (0x1000) : {
    bootstrap/arch/i686/bs_multiboot.bsasm.o(.bss) /* Easiest way to get an aligned stack */
    *(COMMON)
    *(.bss)
  }

  . =  ALIGN(0x1000);
  ebs_kernel = .;
}