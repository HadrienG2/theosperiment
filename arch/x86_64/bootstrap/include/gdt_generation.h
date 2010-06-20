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
#include <hack_stdint.h>

#define LIMIT_CHUNK1_SIZE   16
#define BASE_CHUNK1_SIZE    24
#define IS_A_TSS            0x0000090000000000 //Defines the segment as an unused 32-bit TSS
#define DBIT_ACCESSED       0x0000010000000000 //Bit 40. For code and data segments
#define DBIT_READABLE       0x0000020000000000 //Bit 41. For code segments
#define DBIT_CONFORMING     0x0000040000000000 //Bit 42. For code segments. A lower-privilege task accessing this segment remains at its PL
#define DBIT_WRITABLE       0x0000020000000000 //Bit 41. For data segments
#define DBIT_EXPANDDOWN     0x0000040000000000 //Bit 42. For data segments
#define DBIT_CODEDATA       0x0000080000000000 //Bit 43. 1 for code, 0 for data
#define DBIT_SBIT           0x0000100000000000 //Bit 44. 1 for code/data, 0 for TSS
#define DPL_BITSHIFT        45                 //How many bits to the left the DPL (Descriptor Privilege Level) must be shifted
#define DPL_USERMODE        0x0000600000000000 //Segment is accessible from user mode
#define DBIT_PRESENT        0x0000800000000000 //Bit 47. The segment is present in memory
#define LIMIT_CHUNK2_SIZE   4
#define DBIT_DEFAULT_32OPSZ 0x0040000000000000 //Bit 54. Specifies whether the default operand size is 16 or 32. 1 = 32.
#define DBIT_GRANULARITY    0x0080000000000000 //Bit 55. Specifies if limit is in bytes or in pages. Must be 1 for identity mapping
#define BASE_CHUNK2_SIZE    8

typedef uint64_t segment_descriptor;

//Replace GRUB's GDT with a new one
void replace_32b_gdt();

#endif