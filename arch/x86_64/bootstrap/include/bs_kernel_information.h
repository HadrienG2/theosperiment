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

#include <hack_stdint.h>
#include <bs_arch_specific_kinfo.h>

//Reserved entries of kernel memory map
#define MAX_KMMAP_SIZE 1000

typedef struct kernel_memory_map kernel_memory_map;
struct kernel_memory_map {
  uint64_t location;
  uint64_t size;
  unsigned char nature; //0 : Free memory
                        //1 : Reserved address range
                        //2 : Bootstrap kernel component
                        //3 : Kernel and modules
  uint64_t name; //char* to a string naming the area. For free and reserved memory it's either "Low Mem" or "High Mem".
                //Bootstrap kernel is called "Bootstrap", its separate parts have a precise naming
                //Kernel and modules are called by their GRUB modules names
};
 
typedef struct kernel_information {
  uint64_t command_line; //char* to the kernel command line
  //Memory map
  uint32_t kmmap_size; //Number of entries in kernel memory map
  uint64_t kmmap; //Pointer to the kernel memory map
  arch_specific_info arch_info; //Some arch-specific info
} kernel_information;

#endif