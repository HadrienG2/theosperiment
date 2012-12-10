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

#ifndef _SGT_GENERATION_H_
#define _SGT_GENERATION_H_

#include <bs_kernel_information.h>
#include <stdint.h>

extern const int SGT32_LIMIT_CHUNK1_SIZE;
extern const int SGT32_BASE_CHUNK1_SIZE;
extern const uint64_t SGT32_IS_A_TSS; //Defines the segment as an unused 32-bit TSS
extern const uint64_t SGT32_DBIT_ACCESSED; //Bit 40. For code and data segments
extern const uint64_t SGT32_DBIT_READABLE; //Bit 41. For code segments
extern const uint64_t SGT32_DBIT_CONFORMING; //Bit 42. For code segments. A lower-privilege task
                                             //accessing this segment remains at its PL
extern const uint64_t SGT32_DBIT_WRITABLE; //Bit 41. For data segments
extern const uint64_t SGT32_DBIT_EXPANDDOWN; //Bit 42. For data segments
extern const uint64_t SGT32_DBIT_CODEDATA; //Bit 43. 1 for code, 0 for data
extern const uint64_t SGT32_DBIT_SBIT; //Bit 44. 1 for code/data, 0 for TSS
extern const unsigned int SGT32_DPL_BITSHIFT; //How many bits to the left the DPL must be shifted
extern const uint64_t SGT32_DPL_USERMODE; //Segment is accessible from user mode
extern const uint64_t SGT32_DBIT_PRESENT; //Bit 47. The segment is present in memory
extern const unsigned int SGT32_LIMIT_CHUNK2_SIZE;
extern const uint64_t SGT32_DBIT_DEFAULT_32OPSZ; //Bit 54. Default operand size is 16 or 32 bit.
extern const uint64_t SGT32_DBIT_GRANULARITY; //Bit 55. Specifies if limit is in bytes or pages.
                                              //Set it for identity mapping
extern const unsigned int SGT32_BASE_CHUNK2_SIZE;

extern const uint64_t SGT64_IS_A_TSS; //Defines the segment as an unused 64-bit TSS
extern const uint64_t SGT64_DBIT_CONFORMING; //Bit 42. For code segments. A lower-privilege task
                                             //accessing this segment remains at its PL
extern const uint64_t SGT64_DBIT_CODEDATA; //Bit 43. 1 for code, 0 for data
extern const uint64_t SGT64_DBIT_SBIT; //Bit 44. 1 for code/data, 0 for TSS
extern const uint64_t SGT64_DPL_USERMODE; //Segment is accessible from user mode
extern const uint64_t SGT64_DBIT_PRESENT; //Bit 47. The segment is present in memory
extern const uint64_t SGT64_DBIT_LONG; //Bit 53. Defines this as a 64-bit segment

extern const unsigned int TSS64_SIZE; //Size of a 64-bit Task State Segment in bytes
extern const unsigned int IOMAP_BASE; //Position of the IOPB relative to the TSS (=TSS64_SIZE)
extern const unsigned int IOPB64_SIZE; //Maximum size of an IO Permission Bitmap in bytes

typedef uint64_t codedata_descriptor;
typedef uint64_t system_descriptor[2];

extern const char* GDT_NAME;

//Replace GRUB's GDT with a nice one. Load a null LDT.
//Content of final GDT :
//  * Null descriptor
//  * 64-bit kernel code segment
//  * Kernel data segment
//  * 32-bit kernel code segment (used by the present code)
//  * 32-bit user code segment (not used, present for SYSCALL compatibility reasons)
//  * User data segment
//  * 64-bit user code segment
//  * One TSS per CPU core
void replace_sgt(KernelInformation* kinfo);

#endif