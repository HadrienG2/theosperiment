ENTRY(bootstrap)

SECTIONS {
  . = 0x00100000;
  sbs_kernel = .;

  .text : {
    *(.text)
  }

  .rodata ALIGN (0x1000) : {
    *(.rodata)
  }

  .data ALIGN (0x1000) : {
    *(.data)
  }

  .bss : {
    *(COMMON)
    *(.bss)
  }

  ebs_kernel = ALIGN(0x1000);
}