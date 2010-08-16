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

#ifndef _KERNEL_INFO_H_
#define _KERNEL_INFO_H_

#include <address.h>
#include <hack_stdint.h>
#include <arch_specific_kinfo.h>

//WARNING : ANY CHANGE MADE TO THIS FILE SHOULD BE MIRRORED TO THE ARCH-SPECIFIC
//BS_KERNEL_INFORMATION.H FILES WHEN THEY EXIST. OTHERWISE, INCONSISTENT BEHAVIOR WILL OCCUR.

//Possible mmap elements nature
#define NATURE_FRE  0   //0 : Free memory
#define NATURE_RES  1   //1 : Reserved address range
#define NATURE_BSK  2   //2 : Bootstrap kernel component
#define NATURE_KNL  3   //3 : Kernel and modules

struct KernelCPUInfo {
  uint8_t phys_addr_size; //Size of the biggest physical address the CPU can handle in bits
  uint8_t virt_addr_size; //Size of the biggest virtual address in bits.
  uint32_t core_amount; //Indicates how many CPU cores there are on this system
  uint32_t cache_line_size; //Size of a cache line in bytes. 0 means that caching is not supported
  ArchSpecificCPUInfo arch_info; //Other arch-specific info
} __attribute__ ((packed));

struct KernelMemoryMap {
  addr_t location;
  addr_t size;
  uint8_t nature;
  char* name;   //String naming the area. For free and reserved memory it's either "Low Mem" or "High Mem".
                //Bootstrap kernel is called "Bootstrap", its separate parts have a precise naming
                //Kernel and modules are called by their GRUB modules names
} __attribute__ ((packed));

struct KernelInformation {
  char* command_line; //char* to the kernel command line
  uint32_t kmmap_size; //Number of entries in kernel memory map
  KernelMemoryMap* kmmap; //Pointer to the kernel memory map
  KernelCPUInfo cpu_info; //Information about the processor we run on
  ArchSpecificKInfo arch_info; //Other arch-specific information
} __attribute__ ((packed));

#endif