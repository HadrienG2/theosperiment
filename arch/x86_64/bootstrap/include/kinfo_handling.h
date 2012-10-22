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
#include <stdint.h>

//Add BIOS memory map information to the kernel memory map
//Returns pointer to memory map if successful, 0 otherwise
KernelMemoryMap* add_bios_mmap(KernelMemoryMap* kmmap_buffer, unsigned int *index_ptr, const multiboot_info_t* mbd);
//Adds info about the bootstrap kernel to the kernel memory map
KernelMemoryMap* add_bskernel(KernelMemoryMap* kmmap_buffer, unsigned int *index_ptr, const multiboot_info_t* mbd);
//Add interesting parts of multiboot information structures to the kernel memory map
KernelMemoryMap* add_mbdata(KernelMemoryMap* kmmap_buffer, unsigned int *index_ptr, const multiboot_info_t* mbd);
//Add modules-related information to the kernel information structure
void add_modules(KernelInformation* kinfo, const multiboot_info_t* mbd);
//Duplicate part of a memory map structure in another.
KernelMemoryMap* copy_memory_map_chunk(const KernelMemoryMap* source,
                                       KernelMemoryMap* dest,
                                       const unsigned int start,
                                       const unsigned int length);
//Copy an element between two memory map structures.
KernelMemoryMap* copy_memory_map_elt(const KernelMemoryMap* source,
                                     KernelMemoryMap* dest,
                                     const unsigned int source_index,
                                     const unsigned int dest_index);
//Find a chunk of free high mem space of at least the specified size.
KernelMemoryMap* find_freemem(const KernelInformation* kinfo, const uint64_t minimal_size);
//Same as above, but with page alignment taken into account
KernelMemoryMap* find_freemem_pgalign(const KernelInformation* kinfo, const uint64_t minimal_size);
//Gathers information about the CPU and checks CPU capabilities
KernelCPUInfo* generate_cpu_info(KernelInformation* kinfo);
//Generates a detailed map of the memory.
KernelMemoryMap* generate_memory_map(const multiboot_info_t* mbd, KernelInformation* kinfo);
//Generate info related to multiprocessing.
KernelCPUInfo* generate_multiprocessing_info(KernelInformation* kinfo);


//Generate a kernel information structure (see KernelInformation.h)
KernelInformation* kinfo_gen(const multiboot_info_t* mbd);
//Add an element of specified location, size, nature, and name to the memory map.
void kmmap_add(KernelInformation* kinfo,
               const uint64_t location,
               const uint64_t size,
               const uint8_t nature,
               const char* name);
//Allocate a chunk of free high mem space of the specified size, nature, and name
uint64_t kmmap_alloc(KernelInformation* kinfo, const uint64_t size, const uint8_t nature, const char* name);
//Same as above, but with page aligment taken into account
uint64_t kmmap_alloc_pgalign(KernelInformation* kinfo, const uint64_t size, const uint8_t nature, const char* name);
//Return the amount of physical memory
uint64_t kmmap_mem_amount(const KernelInformation* kinfo);
//Update the memory map so that it follows latest changes
void kmmap_update(KernelInformation* kinfo);


//Merges overlapping entries of a sorted memory map
KernelMemoryMap* merge_memory_map(KernelInformation* kinfo);
//Sorts out a map
KernelMemoryMap* sort_memory_map(KernelInformation* kinfo);

#endif
