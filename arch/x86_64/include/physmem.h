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
//  -Sharing of physical memory pages/chunks (adding and removing owners).
//
//Later : -Allocating a specific page/chunk in memory
//        -Removing all traces of a process.
//        -Ordering a full cleanup of the management structures (as an example when the computer is on but not being used)
class PhyMemManager {
  private:
    PhyMemMap* phy_mmap; //A map of the whole memory
    PhyMemMap* phy_highmmap; //A map of high memory (>0x100000)
    PhyMemMap* free_lowmem; //A contiguous chunk representing free low memory
    PhyMemMap* free_highmem; //A contiguous chunk representing free high memory
    PhyMemMap* free_mapitems; //A collection of spare PhyMemMap objects forming a dummy chunk, that can be put in a memory map
    KernelMutex mmap_mutex;
    //Support methods used by public methods
    addr_t alloc_storage_space(); //Get some memory map storage space
    PhyMemMap* chunk_allocator(PhyMemMap* map_used, const PID initial_owner, const addr_t size); //Allocate a chunk of memory of pre-defined size
    PhyMemMap* chunk_liberator(PhyMemMap* chunk); 
    PhyMemMap* chunk_owneradd(PhyMemMap* chunk, const PID new_owner); //Add an owner to a chunk (for sharing purposes)
    PhyMemMap* chunk_ownerdel(PhyMemMap* chunk, const PID former_owner); //Remove a chunk owner
    PhyMemMap* merge_with_next(PhyMemMap* first_item);
    PhyMemMap* page_allocator(PhyMemMap* map_used, const PID initial_owner);
  public:
    PhyMemManager(const KernelInformation& kinfo);
    //Page/chunk allocation and freeing functions
    PhyMemMap* alloc_chunk(const PID initial_owner, const addr_t size);
    PhyMemMap* alloc_page(const PID initial_owner);
    PhyMemMap* free(PhyMemMap* chunk);
    
    //Sharing functions
    PhyMemMap* owneradd(PhyMemMap* chunk, const PID new_owner);
    PhyMemMap* ownerdel(PhyMemMap* chunk, const PID former_owner);
    
    //x86_64-specific methods
    PhyMemMap* alloc_lowchunk(const PID initial_owner, const addr_t size); //Allocate a chunk of low memory
    PhyMemMap* alloc_lowpage(const PID initial_owner); //Allocate a page of low memory
    
    //Internal methods. Should be protected, but C++'s brain-deadness when it comes to circular
    //header inclusion doesn't allow this.
    PhyMemMap* dump_mmap() const {return phy_mmap;}
    
    //Debug methods. Those will go out someday, so don't use them either.
    void print_highmmap();
    void print_lowmmap();
    void print_mmap();
};

#endif