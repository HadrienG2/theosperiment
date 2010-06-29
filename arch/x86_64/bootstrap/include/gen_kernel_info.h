 /* Functions generating the kernel information structure

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

#ifndef _GEN_KINFO_H_
#define _GEN_KINFO_H_

#include <bs_kernel_information.h>
#include <multiboot.h>

extern const char* MMAP_TOO_SMALL;

//Add BIOS memory map information to the kernel memory map
//Returns pointer to memory map if successful, 0 otherwise
kernel_memory_map* add_bios_mmap(kernel_memory_map* kmmap_buffer, unsigned int *index_ptr, multiboot_info_t* mbd);
//Adds info about the bootstrap kernel to the kernel memory map
kernel_memory_map* add_bskernel(kernel_memory_map* kmmap_buffer, unsigned int *index_ptr, multiboot_info_t* mbd);
//Add multiboot information structures to the kernel memory map
kernel_memory_map* add_mbdata(kernel_memory_map* kmmap_buffer, unsigned int *index_ptr, multiboot_info_t* mbd);
//Add modules-related memory map information to the kernel memory map
kernel_memory_map* add_modules(kernel_memory_map* kmmap_buffer, unsigned int *index_ptr, multiboot_info_t* mbd);
//Duplicate part of a memory map structure in another.
kernel_memory_map* copy_memory_map_chunk(kernel_memory_map* source, kernel_memory_map* dest, unsigned int start, unsigned int length);
//Copy an element between two memory map structures.
kernel_memory_map* copy_memory_map_elt(kernel_memory_map* source, kernel_memory_map* dest, unsigned int source_index, unsigned int dest_index);
//Generate a kernel information structure (see kernel_information.h)
kernel_information* generate_kernel_info(multiboot_info_t* mbd);
//Generates a detailed map of the memory.
kernel_memory_map* generate_memory_map(multiboot_info_t* mbd, kernel_information* kinfo);
//Merges overlapping entries of a sorted memory map
kernel_memory_map* merge_memory_map(kernel_information* kinfo);
//Sorts out a map
kernel_memory_map* sort_memory_map(kernel_information* kinfo);

#endif
