ENTRY(bootstrap)

SECTIONS {
  . = 0x00100000; /* Bootstrap kernel loaded at 1MB */
  sbs_kernel = .;

  .text : {
    bs_multiboot.o(.text) /* Have the multiboot header as early as possible */
    *(.text)
  }

  .rodata ALIGN (0x1000) : {
    *(.rodata)
  }

  .data ALIGN (0x1000) : {
    *(.data)
  }

  .bss ALIGN (0x1000) : {
    bs_multiboot.o(.bss) /* The easiest way to get a properly aligned stack */
    *(COMMON)
    *(.bss)
  }

  . =  ALIGN(0x1000);
  ebs_kernel = .;
}