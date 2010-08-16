 /* Structures that are passed to the main kernel by the bootstrap kernel -- arch-specific data

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

#ifndef _BS_ARCH_INFO_H_
#define _BS_ARCH_INFO_H_

#include <address.h>
#include <stdint.h>

//WARNING : ANY CHANGE MADE TO THIS FILE SHOULD BE MIRRORED TO ARCH_SPECIFIC_KINFO.H.
//OTHERWISE, INCONSISTENT BEHAVIOR WILL OCCUR.

#define ARCH_INVALID 0
#define ARCH_X86_64 1

typedef struct ArchSpecificCPUInfo ArchSpecificCPUInfo;
typedef struct ArchSpecificKInfo ArchSpecificKInfo;
typedef struct StartupDriveInfo StartupDriveInfo;

struct ArchSpecificCPUInfo { //On x86_64, this is basically the information returned by CPUID
  uint8_t arch;
  addr_t vendor_string;
  uint8_t family;
  uint8_t model;
  addr_t processor_name;
  uint8_t stepping;
  uint16_t instruction_exts[2]; //The contents follow
    //word 1 : SSE
    //  bit 0 : SSE
    //  bit 1 : SSE2
    //  bit 2 : SSE3
    //  bit 3 : SSSE3
    //  bit 4 : SSE4.1
    //  bit 5 : SSE4A
    //  bit 6 : SSE5
    //word 2 : Misc.
    //  bit 0 : x87
    //  bit 1 : 128-bit media
    //  bit 2 : 3DNow
    //  bit 3 : 3DNowExt
    //  bit 4 : 3DNowPrefetch
    //  bit 5 : MMX
    //  bit 6 : MMXExt
  uint8_t global_page_support;
  uint8_t hyperthreading_support;
  uint8_t fast_syscall_support;
} __attribute__ ((packed));

struct StartupDriveInfo {
  uint8_t drive_number;
  uint8_t partition_number;
  uint8_t sub_partition_number;
  uint8_t subsub_partition_number;
} __attribute__ ((packed));

struct ArchSpecificKInfo {
  addr_t startup_drive; /* 64-bit pointer to a StartupDriveInfo structure */
} __attribute__ ((packed));

#endif 
