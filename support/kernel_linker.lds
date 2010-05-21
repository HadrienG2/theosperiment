ENTRY(kinit)

PHDRS {
  headers PT_PHDR PHDRS;
  rx PT_LOAD FLAGS(5);
  r PT_LOAD FLAGS(4);
  rw PT_LOAD FLAGS(6);
}

SECTIONS {
  . = 0x1001000; /* Loaded at 16MB+4KB in order to prevent erasing GRUB-provided modules */

  .text ALIGN(4096) :
  {
    *(.text*)
    *(.gnu.linkonce.t*)
  } : rx

  .rodata ALIGN(4096) :
  {
    *(.rodata*)
    *(.gnu.linkonce.r*)
  } : r
  
  .data ALIGN(4096) :
  {
    start_ctors = .;
    *(.ctor*)
    end_ctors = .;

    start_dtors = .;
    *(.dtor*)
    end_dtors = .;
  
    *(.data*)
    *(.gnu.linkonce.d*)
  } : rw

  .bss ALIGN(4096) :
  {
    *(.COMMON*)
    *(.bss*)
    *(.gnu.linkonce.b*)
  } : rw
  
   /DISCARD/ :
   {
    *(.comment)
    *(.eh_frame) /* You should discard this unless you're implementing runtime support for C++ exceptions. */
   }
}
