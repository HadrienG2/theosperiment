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

segment32_descriptor gdt32[3];
segment64_descriptor gdt64[3];

void replace_32b_gdt() {
  uint64_t gdtr;
  
  //Fill in the GDT
  //Null segment
  gdt32[0] = 0;
  //Code segment : setting all base bits to 0 and all limit bits to 1, then setting appropriate bits
  gdt32[1] = 0x000f00000000ffff;
  gdt32[1] += SGT32_DBIT_READABLE + SGT32_DBIT_CODEDATA + SGT32_DBIT_SBIT + SGT32_DBIT_PRESENT + SGT32_DBIT_DEFAULT_32OPSZ + SGT32_DBIT_GRANULARITY;
  //Data segment
  gdt32[2] = 0x000f00000000ffff;
  gdt32[2] += SGT32_DBIT_WRITABLE + SGT32_DBIT_SBIT + SGT32_DBIT_PRESENT + SGT32_DBIT_DEFAULT_32OPSZ + SGT32_DBIT_GRANULARITY;
  
  //Now that the GDT is complete, setup GDTR value and load it
  gdtr = (uint32_t) gdt32;
  gdtr <<= 16;
  gdtr += 24;
  //Code descriptor is 8, data descriptor is 16
  __asm__ volatile ("lgdt %0;\
                     mov $16, %%ax;\
                     mov %%ax, %%ds;\
                     mov %%ax, %%es;\
                     mov %%ax, %%fs;\
                     mov %%ax, %%gs;\
                     mov %%ax, %%ss;\
                     ljmp $8, $end;\
                   end:"
             :
             : "m" (gdtr)
             : "%ax"
             );

}

uint64_t gen_64b_gdt() {
  uint64_t gdtr;
  
  //Fill in the GDT
  //Null segment
  gdt64[0] = 0;
  //Code segment
  gdt64[1] = SGT64_DBIT_CODEDATA + SGT64_DBIT_SBIT + SGT64_DBIT_PRESENT + SGT64_DBIT_LONG + SGT64_DBIT_GRANULARITY;
  //Data segment
  gdt64[2] = SGT64_DBIT_SBIT + SGT64_DBIT_PRESENT + SGT64_DBIT_LONG + SGT64_DBIT_GRANULARITY;
  
  //Now that the GDT is complete, setup GDTR value and return it
  gdtr = (uint32_t) gdt64;
  gdtr <<= 16;
  gdtr += 24;
  return gdtr;
}