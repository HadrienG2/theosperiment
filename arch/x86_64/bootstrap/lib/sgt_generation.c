 /* Functions used to setup segmentation properly

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
 
#include "sgt_generation.h"
#include <align.h>
#include <kinfo_handling.h>

const int SGT32_LIMIT_CHUNK1_SIZE = 16;
const int SGT32_BASE_CHUNK1_SIZE = 24;
const uint64_t SGT32_IS_A_TSS = 0x0000090000000000; //Defines the segment as an unused 32-bit TSS
const uint64_t SGT32_DBIT_ACCESSED = 0x0000010000000000; //Bit 40. For code and data segments
const uint64_t SGT32_DBIT_READABLE = 0x0000020000000000; //Bit 41. For code segments
const uint64_t SGT32_DBIT_CONFORMING = 0x0000040000000000; //Bit 42. For code segments.
                                                           //A lower-privilege task accessing this
                                                           //segment remains at its PL
const uint64_t SGT32_DBIT_WRITABLE = 0x0000020000000000; //Bit 41. For data segments
const uint64_t SGT32_DBIT_EXPANDDOWN = 0x0000040000000000; //Bit 42. For data segments
const uint64_t SGT32_DBIT_CODEDATA = 0x0000080000000000; //Bit 43. 1 for code, 0 for data
const uint64_t SGT32_DBIT_SBIT = 0x0000100000000000; //Bit 44. 1 for code/data, 0 for TSS
const unsigned int SGT32_DPL_BITSHIFT = 45; //How many bits to the left the DPL must be shifted
const uint64_t SGT32_DPL_USERMODE = 0x0000600000000000; //Segment is accessible from user mode
const uint64_t SGT32_DBIT_PRESENT = 0x0000800000000000; //Bit 47. The segment is present in memory
const unsigned int SGT32_LIMIT_CHUNK2_SIZE = 4;
const uint64_t SGT32_DBIT_DEFAULT_32OPSZ = 0x0040000000000000; //Bit 54. Default operand size is 16
                                                               //or 32 bit. 1 = 32 bit.
const uint64_t SGT32_DBIT_GRANULARITY = 0x0080000000000000; //Bit 55. Specifies if limit is in bytes
                                                            //or pages. Set it for identity mapping
const unsigned int SGT32_BASE_CHUNK2_SIZE = 8;

const uint64_t SGT64_IS_A_TSS = 0x0000090000000000; //Defines the segment as an unused 64-bit TSS
const uint64_t SGT64_DBIT_CONFORMING = 0x0000040000000000; //Bit 42. For code segments.
                                                           //A lower-privilege task accessing this
                                                           //segment remains at its PL
const uint64_t SGT64_DBIT_CODEDATA = 0x0000080000000000; //Bit 43. 1 for code, 0 for data
const uint64_t SGT64_DBIT_SBIT = 0x0000100000000000; //Bit 44. 1 for code/data, 0 for TSS
const uint64_t SGT64_DPL_USERMODE = 0x0000600000000000; //Segment is accessible from user mode
const uint64_t SGT64_DBIT_PRESENT = 0x0000800000000000; //Bit 47. The segment is present in memory
const uint64_t SGT64_DBIT_LONG = 0x0020000000000000; //Bit 53. Defines this as a 64-bit segment

const unsigned int TSS64_SIZE = 0x68; //Size of a 64-bit Task State Segment in bytes
const unsigned int IOMAP_BASE = 0x68; //Position of the IOPB relative to the TSS (=TSS64_SIZE)
const unsigned int IOPB64_SIZE = 0x2000; //Maximum size of an IO Permission Bitmap in bytes

const char* GDT_NAME = "Kernel GDT";
const char* TSS_NAME = "Kernel TSS";

void replace_sgt(KernelInformation* kinfo) {
  uint64_t gdtr, ldtr, gdt_location, gdt_length, tss_location, tss_length;
  uint32_t tss_base, tss_limit;
  unsigned int current_core, index;
  uint16_t* iomap_base;
  uint32_t* iopb;
  codedata_descriptor* gdt1;
  system_descriptor* gdt2;
  
  //Allocate GDT space for 7 code/data entries and as much TSS descriptors as there are CPU cores
  gdt_length = 7*sizeof(codedata_descriptor)+kinfo->cpu_info.core_amount*sizeof(system_descriptor);
  gdt_location = kmmap_alloc_pgalign(kinfo, gdt_length, NATURE_KNL, GDT_NAME);
  
  //Fill in the GDT with the following : NULL, 64bKCS, 64bKDS, 32bKCS, 32bUCS, 64bUDS, 64bUCS, TSS0, ...
  gdt1 = (codedata_descriptor*) (uint32_t) gdt_location;
  //Null segment
  gdt1[0] = 0;
  //64-bit kernel code segment
  gdt1[1] = SGT64_DBIT_CODEDATA +
            SGT64_DBIT_SBIT +
            SGT64_DBIT_PRESENT +
            SGT64_DBIT_LONG;
  //Kernel data segment : setting all base bits to 0 and all limit bits to 1
  gdt1[2] = 0x000f00000000ffff;
  gdt1[2]+= SGT32_DBIT_WRITABLE +
            SGT32_DBIT_SBIT +
            SGT32_DBIT_PRESENT +
            SGT32_DBIT_DEFAULT_32OPSZ +
            SGT32_DBIT_GRANULARITY;
  //32-bit kernel code segment
  gdt1[3] = 0x000f00000000ffff;
  gdt1[3]+= SGT32_DBIT_READABLE +
            SGT32_DBIT_CODEDATA +
            SGT32_DBIT_SBIT +
            SGT32_DBIT_PRESENT +
            SGT32_DBIT_DEFAULT_32OPSZ +
            SGT32_DBIT_GRANULARITY;
  //32-bit user code segment
  gdt1[4] = 0x000f00000000ffff;
  gdt1[4]+= SGT32_DBIT_READABLE +
            SGT32_DBIT_CODEDATA +
            SGT32_DBIT_SBIT +
            SGT32_DPL_USERMODE +
            SGT32_DBIT_PRESENT +
            SGT32_DBIT_DEFAULT_32OPSZ +
            SGT32_DBIT_GRANULARITY;
  //User data segment
  gdt1[5] = 0x000f00000000ffff;
  gdt1[5]+= SGT32_DBIT_WRITABLE +
            SGT32_DBIT_SBIT +
            SGT32_DPL_USERMODE +
            SGT32_DBIT_PRESENT +
            SGT32_DBIT_DEFAULT_32OPSZ +
            SGT32_DBIT_GRANULARITY;
  //64-bit user code segment
  gdt1[6] = SGT64_DBIT_CODEDATA +
            SGT64_DBIT_SBIT +
            SGT64_DPL_USERMODE +
            SGT64_DBIT_PRESENT +
            SGT64_DBIT_LONG;
  
  //TSS segments (1 per core)
  gdt2 = (system_descriptor*) (uint32_t) (gdt_location+7*sizeof(codedata_descriptor));
  tss_length = TSS64_SIZE+IOPB64_SIZE;
  for(current_core=0; current_core<kinfo->cpu_info.core_amount; ++current_core) {
    //Allocate and fill the interesting parts of the TSS
    tss_location = kmmap_alloc_pgalign(kinfo, tss_length, NATURE_KNL, TSS_NAME);
    iomap_base = (uint16_t*) (uint32_t) (tss_location+TSS64_SIZE-2);
    *iomap_base = TSS64_SIZE; //We put the IO permission bitmap right after the TSS
    iopb = (uint32_t*) (uint32_t) (tss_location+TSS64_SIZE);
    for(index=0; index<IOPB64_SIZE/4; ++index) {
      iopb[index]=0xffffffff; //Fill the IO permission bitmap with 1
    }
    
    //Generate a TSS descriptor
    tss_base = tss_location&0xffffffff;
    tss_limit = tss_length-1;
    gdt2[current_core][0] = tss_base&0xff000000;
    gdt2[current_core][0]<<= 16;
    gdt2[current_core][0]+= tss_base&0x00ffffff;
    gdt2[current_core][0]<<= 16;
    gdt2[current_core][0]+= tss_limit&0xffff;
    gdt2[current_core][0]+= SGT32_IS_A_TSS +
                     SGT32_DPL_USERMODE +
                     SGT32_DBIT_PRESENT;
    gdt2[current_core][1] = 0; //We won't need the additional address space of long-mode TSS descriptor
  }
  
  //Now that the GDT is complete, setup GDTR value and load it. While we're at it, load a null LDT
  //and a 32-bit Task Register
  gdtr = gdt_location;
  gdtr <<= 16;
  gdtr += gdt_length-1;
  ldtr = 0;
  //32-bit code segment is at offset 24, data segment is at offset 16
  __asm__ volatile ("lgdt %0;\
                     mov  $16, %%ax;\
                     mov  %%ax, %%ds;\
                     mov  %%ax, %%es;\
                     mov  %%ax, %%fs;\
                     mov  %%ax, %%gs;\
                     mov  %%ax, %%ss;\
                     ljmp $24, $cs_loaded;\
                   cs_loaded:\
                     lldt %1;"
             :
             : "m" (gdtr), "m" (ldtr)
             : "%ax"
             );

}