 /* Physical memory management, ie managing of pages of RAM

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

#ifndef _PHYSMEM_H_
#define _PHYSMEM_H_

#include <address.h>
#include <kernel_information.h>
#include <memory_support.h>
#include <pid.h>
#include <synchronization.h>

#include <dbgstream.h>

//This class takes care of
//  -Mapping which pages of physical memory are in use by processes, which are free
//  -...which PID (Process IDentifier) owns each used page of memory
//  -Allocation of pages of memory to a specific PID
//  -Allocation of multiple non-contiguous pages, called a chunk of memory and managed as a whole
//
//Later: -Sharing of physical memory pages/chunks.
class PhyMemManager {
  private:
    MemMap* phy_mmap; //A map of the whole memory
    MemMap* first_hole; //First free region in said map
    MemMap* phy_highmmap; //A map of high memory (>0x100000)
    MemMap* first_highhole; //First free region in said map
    MemMap* free_mapitems; //A list of unused memory map items that can be put in a memory map
    KernelMutex mmap_mutex;
    addr_t alloc_storage_space();
    MemMap* chunk_allocator(MemMap* map_used, PID initial_owner, addr_t size);
    MemMap* merge_with_next(MemMap* first_item);
    addr_t page_allocator(MemMap* map_used, PID initial_owner);
  public:
    PhyMemManager(KernelInformation& kinfo);
    //Routines for exclusive page/chunk allocation and suppression functions
    MemMap* alloc_chunk(PID initial_owner, addr_t size);
    addr_t alloc_page(PID initial_owner);
    addr_t free(addr_t location);
    
    //x86_64-specific methods
    MemMap* alloc_lowchunk(PID initial_owner, addr_t size); //Allocate a chunk of low memory
    addr_t alloc_lowpage(PID initial_owner); //Allocate a page of low memory
    
    //Debug methods (to be deleted)
    void print_lowmmap();
    void print_highmmap();
};

#endif