 /* Physical memory management, ie managing of pages of RAM

    Copyright (C) 2010-2011   Hadrien Grasland

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

const int PHYMEMMANAGER_VERSION = 1; //Increase this when changes require a modification of
                                     //the testing protocol

//This class takes care of
// -Mapping which pages of physical memory are in use by processes, which are free
// -...which PID (Process IDentifier) owns each used page of memory
// -Allocation of pages of memory to a specific PID
// -Allocation of multiple non-contiguous pages, called a chunk of memory and managed as a whole
// -Sharing of physical memory pages/chunks (adding and removing owners).
// -Getting access to a reserved part of memory under explicit request
// -Killing a process
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
        addr_t alloc_mapitems(); //Get some memory map storage space
        PhyMemMap* page_allocator(const PID initial_owner, PhyMemMap* map_used);
        PhyMemMap* chunk_allocator(const addr_t size,
                                   const PID initial_owner,
                                   PhyMemMap* map_used);
        PhyMemMap* contigchunk_allocator(const addr_t requested_size,
                                         const PID initial_owner,
                                         PhyMemMap* map_used);
        PhyMemMap* resvchunk_allocator(const addr_t location, const PID initial_owner);
        PhyMemMap* chunk_liberator(PhyMemMap* chunk);
        PhyMemMap* chunk_owneradd(PhyMemMap* chunk, const PID new_owner);
        PhyMemMap* chunk_ownerdel(PhyMemMap* chunk, const PID former_owner);
        PhyMemMap* merge_with_next(PhyMemMap* first_item); //Merge two consecutive elements of
                                                           //the memory map (in order to save space)
        void killer(PID target);
    public:
        PhyMemManager(const KernelInformation& kinfo);
        //Page/chunk allocation and freeing functions
        PhyMemMap* alloc_page(const PID initial_owner); //Allocates a page of memory.
        PhyMemMap* alloc_chunk(const addr_t size,        //Allocates a non-contiguous chunk of memory
                               const PID initial_owner); //at least "size" large.
        PhyMemMap* alloc_contigchunk(const addr_t size,        //Same as above, but with a contiguous
                                     const PID initial_owner); //chunk of memory
        PhyMemMap* alloc_resvchunk(const addr_t location,    //Get access to a reserved chunk of
                                   const PID initial_owner); //memory, if it is not allocated yet
        PhyMemMap* free(PhyMemMap* chunk); //Free a chunk of memory (fast version)
        PhyMemMap* free(addr_t chunk_beginning); //Same as above, but slower and easier to use

        //Sharing functions
        PhyMemMap* owneradd(PhyMemMap* chunk, const PID new_owner); //Add owners to a chunk
        PhyMemMap* ownerdel(PhyMemMap* chunk, const PID former_owner); //Remove owners from a chunk
                                                                       //(delete it if he has no
                                                                       //longer any owner)

        //Cleanup functions
        void kill(PID target); //Removes all traces of a PID in PhyMemManager (slow method, prefer
                               //the higher-level version from MemAllocator when possible)

        //Finding a chunk in the map
        PhyMemMap* find_this(addr_t location);

        //x86_64-specific methods
        PhyMemMap* alloc_lowpage(const PID initial_owner); //Allocate a page of low memory (<1MB)
        PhyMemMap* alloc_lowchunk(const addr_t size, //Allocate a chunk of low memory
                                  const PID initial_owner);
        PhyMemMap* alloc_lowcontigchunk(const addr_t size, const PID initial_owner);

        //Debug methods. Will go out in a final release.
        void print_highmmap(); //Print a map of high memory (>=1MB)
        void print_lowmmap(); //Print a map of low memory (<1MB)
        void print_mmap(); //Print the full memory map
};

#endif