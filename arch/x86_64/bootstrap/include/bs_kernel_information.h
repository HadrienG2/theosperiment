 /* Structures that are passed to the main kernel by the bootstrap kernel

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

#ifndef _BS_KERNEL_INFO_H_
#define _BS_KERNEL_INFO_H_

#include <bs_arch_specific_kinfo.h>
#include <stdint.h>

//WARNING : ANY CHANGE MADE TO THIS FILE SHOULD BE MIRRORED TO KERNEL_INFORMATION.H.
//OTHERWISE, INCONSISTENT BEHAVIOR WILL OCCUR.

//Possible mmap elements nature
#define NATURE_FRE  0   //0 : Free memory
#define NATURE_RES  1   //1 : Reserved address range
#define NATURE_BSK  2   //2 : Bootstrap kernel component
#define NATURE_KNL  3   //3 : Kernel and modules

//Reserved entries of kernel memory map.
#define MAX_KMMAP_SIZE 512

typedef struct KernelCPUInfo KernelCPUInfo;
typedef struct KernelMemoryMap KernelMemoryMap;
typedef struct KernelInformation KernelInformation;

struct KernelCPUInfo {
  uint32_t core_amount; //Indicates how many CPU cores there are on this system
  uint32_t cache_line_size; //Size of a cache line in bytes. 0 means that caching is not supported
  ArchSpecificCPUInfo arch_info; //Other arch-specific info
} __attribute__ ((packed));

struct KernelMemoryMap {
  addr_t location;
  addr_t size;
  uint8_t nature;
  addr_t name; //char* to a string naming the area. For free and reserved memory it's either "Low Mem" or "High Mem".
               //Bootstrap kernel is called "Bootstrap", its separate parts have a precise naming
               //Kernel and modules are called by their GRUB modules names
} __attribute__ ((packed));

struct KernelInformation {
  addr_t command_line; //char* to the kernel command line
  uint32_t kmmap_size; //Number of entries in kernel memory map
  addr_t kmmap; //Pointer to the kernel memory map
  KernelCPUInfo cpu_info; //Information about the processor we run on
  ArchSpecificKInfo arch_info; //Other arch-specific information
} __attribute__ ((packed));

#endif