 /* Functions used to replace GRUB's GDT

    Copyright (C) 2010  Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */
 
#include <die.h>
#include "gdt_generation.h"
#include <gen_kernel_info.h>

segment_descriptor gdt[4];
uint32_t tss[26];

void replace_32b_gdt() {
  unsigned int tss_entry;
  uint64_t gdtr;
  
  //Fill in the GDT
  //Null segment
  gdt[0] = 0;
  //Code segment : setting all base bits to 0 and all limit bits to 1, then setting appropriate bits
  gdt[1] = 0x000f00000000ffff;
  gdt[1] += DBIT_READABLE + DBIT_CODEDATA + DBIT_SBIT + DBIT_PRESENT + DBIT_DEFAULT_32OPSZ + DBIT_GRANULARITY;
  //Data segment
  gdt[2] = 0x000f00000000ffff;
  gdt[2] += DBIT_WRITABLE + DBIT_SBIT + DBIT_PRESENT + DBIT_DEFAULT_32OPSZ + DBIT_GRANULARITY;
  
  //Make a TSS with SS0 = 16 (Kernel data is 2nd GDT entry), ESP0 = 0 (bad practice, but system calls won't occur), and IOPB = 104
  //(because we don't need this io bitmap)
  for(tss_entry=0; tss_entry<26; ++tss_entry) {
    if(tss_entry==2) tss[tss_entry]=16;
    else if(tss_entry==25) tss[tss_entry]=104 << 16;
    else tss[tss_entry]=0;
  }
  //Add a TSS descriptor. We assume that the TSS is located in the first 2^24 bytes of the address space, which is reasonable here.
  gdt[3] = ((uint32_t) tss) * (1 << LIMIT_CHUNK1_SIZE) + 104;
  gdt[3] += IS_A_TSS + DBIT_PRESENT;
  
  //Now that the GDT is complete, setup GDTR value and load it
  gdtr = (uint32_t) gdt;
  gdtr <<= 16;
  gdtr += 32;
  //Code descriptor is 8, data descriptor is 16, TSS descriptor is 24
  __asm__ volatile ("lgdt %0;\
                     mov $16, %%ax;\
                     mov %%ax, %%ds;\
                     mov %%ax, %%es;\
                     mov %%ax, %%fs;\
                     mov %%ax, %%gs;\
                     mov %%ax, %%ss;\
                     ljmp $8, $bp;\
                   bp:\
                     mov $24, %%ax;\
                     ltr %%ax"
             :
             : "m" (gdtr)
             : "%ax", "%bx"
             );

}