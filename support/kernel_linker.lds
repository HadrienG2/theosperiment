ENTRY(kinit)

PHDRS {
  headers PT_PHDR PHDRS;
  r-x PT_LOAD FLAGS(5);
  r-- PT_LOAD FLAGS(4);
  rw- PT_LOAD FLAGS(6);
}

SECTIONS {
  . = 0x40000000; /* To be loaded at 1GB. Simplifies the loading procedure  */

  .text ALIGN(0x1000) :
  {
    knl_rx_start = .;
    
    *(.text*)
    *(.gnu.linkonce.t*)

    . =  ALIGN(0x1000);
    knl_rx_end = .;
  } : rx

  .rodata ALIGN(0x1000) :
  {
    knl_r_start = .;
    start_ctors = .;
    
    *(.ctor*)
    
    end_ctors = .;
    start_dtors = .;
    
    *(.dtor*)
    
    end_dtors = .;
    
    *(.rodata*)
    *(.gnu.linkonce.r*)
    
    . =  ALIGN(0x1000);
    knl_r_end = .;
  } : r
  
  .data ALIGN(0x1000) :
  {  
    knl_rw_start = .;
    
    *(.data*)
    *(.gnu.linkonce.d*)
  } : rw

  .bss ALIGN(0x1000) :
  {
    *(.COMMON*)
    *(.bss*)
    *(.gnu.linkonce.b*)
    
    . =  ALIGN(0x1000);
    knl_rw_end = .;
  } : rw
  
   /DISCARD/ :
   {
    *(.comment)
    *(.eh_frame) /* You should discard this unless you're implementing runtime support for C++ exceptions. */
   }
}
