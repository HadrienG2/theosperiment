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
        OwnerlessMutex mmap_mutex;
        //Support methods used by public methods
        bool alloc_mapitems(); //Get some memory map storage space
        PhyMemMap* chunk_allocator(const PID initial_owner,
                                   const addr_t size,
                                   PhyMemMap* map_used,
                                   bool contiguous = false);
        PhyMemMap* resvchunk_allocator(const PID initial_owner, const addr_t location);
        bool chunk_liberator(PhyMemMap* chunk);
        bool chunk_owneradd(PhyMemMap* chunk, const PID new_owner);
        bool chunk_ownerdel(PhyMemMap* chunk, const PID former_owner);
        void merge_with_next(PhyMemMap* first_item); //Merge two consecutive elements of
                                                     //the memory map (in order to save space)
        void killer(PID target);
    public:
        PhyMemManager(const KernelInformation& kinfo);
        //Page/chunk allocation and freeing functions
        PhyMemMap* alloc_page(const PID initial_owner); //Allocates a page of memory.
        PhyMemMap* alloc_chunk(const PID initial_owner,  //Allocates a chunk of memory which is at
                               const addr_t size,        //least "size" large. The "contiguous" flag
                               bool contiguous = false); //forces it to be physically contiguous
        PhyMemMap* alloc_resvchunk(const PID initial_owner,  //Get access to a reserved chunk of
                                   const addr_t location);   //memory, if it is not allocated yet
        bool free(PhyMemMap* chunk); //Free a chunk of memory (fast version)
        bool free(addr_t chunk_beginning); //Same as above, but slower and easier to use

        //Sharing functions
        bool owneradd(PhyMemMap* chunk, const PID new_owner); //Add owners to a chunk
        bool ownerdel(PhyMemMap* chunk, const PID former_owner); //Remove owners from a chunk
                                                                 //(delete it if he has no
                                                                 //longer any owner)

        //Cleanup functions
        void kill(PID target); //Removes all traces of a PID in PhyMemManager (slow method, prefer
                               //the higher-level version from MemAllocator when possible)

        //Finding a chunk in the map
        PhyMemMap* find_this(addr_t location);

        //x86_64-specific methods
        PhyMemMap* alloc_lowpage(const PID initial_owner); //Allocate a page of low memory (<1MB)
        PhyMemMap* alloc_lowchunk(const PID initial_owner, //Allocate a chunk of low memory
                                  const addr_t size,
                                  bool contiguous = false);

        //Debug methods. Will go out in a final release.
        void print_highmmap(); //Print a map of high memory (>=1MB)
        void print_lowmmap(); //Print a map of low memory (<1MB)
        void print_mmap(); //Print the full memory map
};

#endif