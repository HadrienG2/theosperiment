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

#ifndef _GDT_GENERATION_H_
#define _GDT_GENERATION_H_

#include <bs_kernel_information.h>
#include <stdint.h>

#define SGT32_LIMIT_CHUNK1_SIZE   16
#define SGT32_BASE_CHUNK1_SIZE    24
#define SGT32_IS_A_TSS            0x0000090000000000 //Defines the segment as an unused 32-bit TSS
#define SGT32_DBIT_ACCESSED       0x0000010000000000 //Bit 40. For code and data segments
#define SGT32_DBIT_READABLE       0x0000020000000000 //Bit 41. For code segments
#define SGT32_DBIT_CONFORMING     0x0000040000000000 //Bit 42. For code segments. A lower-privilege task accessing this segment remains at its PL
#define SGT32_DBIT_WRITABLE       0x0000020000000000 //Bit 41. For data segments
#define SGT32_DBIT_EXPANDDOWN     0x0000040000000000 //Bit 42. For data segments
#define SGT32_DBIT_CODEDATA       0x0000080000000000 //Bit 43. 1 for code, 0 for data
#define SGT32_DBIT_SBIT           0x0000100000000000 //Bit 44. 1 for code/data, 0 for TSS
#define SGT32_DPL_BITSHIFT        45                 //How many bits to the left the DPL (Descriptor Privilege Level) must be shifted
#define SGT32_DPL_USERMODE        0x0000600000000000 //Segment is accessible from user mode
#define SGT32_DBIT_PRESENT        0x0000800000000000 //Bit 47. The segment is present in memory
#define SGT32_LIMIT_CHUNK2_SIZE   4
#define SGT32_DBIT_DEFAULT_32OPSZ 0x0040000000000000 //Bit 54. Specifies whether the default operand size is 16 or 32. 1 = 32.
#define SGT32_DBIT_GRANULARITY    0x0080000000000000 //Bit 55. Specifies if limit is in bytes or in pages. Must be 1 for identity mapping
#define SGT32_BASE_CHUNK2_SIZE    8

#define SGT64_DBIT_CONFORMING     0x0000040000000000 //Bit 42. For code segments. A lower-privilege task accessing this segment remains at its PL
#define SGT64_DBIT_CODEDATA       0x0000080000000000 //Bit 43. 1 for code, 0 for data
#define SGT64_DBIT_SBIT           0x0000100000000000 //Bit 44. 1 for code/data, 0 for TSS
#define SGT64_DPL_USERMODE        0x0000600000000000 //Segment is accessible from user mode
#define SGT64_DBIT_PRESENT        0x0000800000000000 //Bit 47. The segment is present in memory
#define SGT64_DBIT_LONG		        0x0020000000000000 //Bit 53. Defines the segment as a 64-bit segment
#define SGT64_DBIT_GRANULARITY    0x0080000000000000 //Bit 55. Specifies if limit is in bytes or in pages. Must be 1 for identity mapping

typedef uint64_t segment32_descriptor;
typedef uint64_t segment64_descriptor;

//Replace GRUB's GDT with a nice one with identity mapping (no TSS at the moment, this GDT is to
//be quickly replaced)
void replace_gdt();

#endif