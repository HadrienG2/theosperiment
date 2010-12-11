 /* Physical memory management, ie managing of pages of RAM

    Copyright (C) 2010    Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#ifndef _PHYSMEM_H_
#define _PHYSMEM_H_

#include <address.h>
#include <kernel_information.h>
#include <memory_support.h>
#include <pid.h>
#include <synchronization.h>

#include <dbgstream.h>

//This class takes care of
// -Mapping which pages of physical memory are in use by processes, which are free
// -...which PID (Process IDentifier) owns each used page of memory
// -Allocation of pages of memory to a specific PID
// -Allocation of multiple non-contiguous pages, called a chunk of memory and managed as a whole
// -Sharing of physical memory pages/chunks (adding and removing owners).
//
//Later : -Allocating a specific page/chunk in memory
//        -Removing all traces of a process
class PhyMemManager {
    private:
        PhyMemMap* phy_mmap; //A map of the whole memory
        PhyMemMap* phy_highmmap; //A map of high memory (>0x100000)
        PhyMemMap* free_lowmem; //A contiguous chunk representing free low memory
        PhyMemMap* free_highmem; //A contiguous chunk representing free high memory
        PhyMemMap* free_mapitems; //A collection of spare PhyMemMap objects forming a dummy chunk,
                                  //ready for use in a memory map
        KernelMutex mmap_mutex;
        //Support methods used by public methods
        addr_t alloc_storage_space(); //Get some memory map storage space
        PhyMemMap* chunk_allocator(PhyMemMap* map_used,     //See alloc_chunk()
                                   const PID initial_owner,
                                   const addr_t size);
        PhyMemMap* contigchunk_allocator(PhyMemMap* map_used, //See alloc_contigchunk()
                                         const PID initial_owner,
                                         const addr_t requested_size);
        PhyMemMap* chunk_liberator(PhyMemMap* chunk); //See free()
        PhyMemMap* chunk_owneradd(PhyMemMap* chunk,const PID new_owner); //See owneradd()
        PhyMemMap* chunk_ownerdel(PhyMemMap* chunk, const PID former_owner); //See ownerdel()
        PhyMemMap* merge_with_next(PhyMemMap* first_item); //Merge two consecutive elements of
                                                           //the memory map (in order to save space)
        PhyMemMap* page_allocator(PhyMemMap* map_used, const PID initial_owner); //See alloc_page()
    public:
        PhyMemManager(const KernelInformation& kinfo);
        //Page/chunk allocation and freeing functions
        PhyMemMap* alloc_page(const PID initial_owner); //Allocates a page of memory.
        PhyMemMap* alloc_chunk(const PID initial_owner, //Allocates a non-contiguous chunk of memory
                               const addr_t size);      //at list "size" large.
        PhyMemMap* alloc_contigchunk(const PID initial_owner, //Same as above, but with a contiguous
                                     const addr_t size);      //chunk of memory
        PhyMemMap* free(PhyMemMap* chunk); //Free a chunk of memory (fast version)
        PhyMemMap* free(addr_t chunk_beginning); //Same as above, but slower and easier to use
        
        //Sharing functions
        PhyMemMap* owneradd(PhyMemMap* chunk, const PID new_owner); //Add owners to a chunk
        PhyMemMap* ownerdel(PhyMemMap* chunk, const PID former_owner); //Remove owners from a chunk
                                                                       //(delete it if he has no
                                                                       //longer any owner)
        
        //x86_64-specific methods
        PhyMemMap* alloc_lowpage(const PID initial_owner); //Allocate a page of low memory (<1MB)
        PhyMemMap* alloc_lowchunk(const PID initial_owner, //Allocate a chunk of low memory
                                  const addr_t size);
        PhyMemMap* alloc_lowcontigchunk(const PID initial_owner, const addr_t size);
        
        //Debug methods. Will go out in a final release.
        void print_highmmap(); //Print a map of high memory (>=1MB)
        void print_lowmmap(); //Print a map of low memory (<1MB)
        void print_mmap(); //Print the full memory map
};

#endif