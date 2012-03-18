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
#include <align.h>
#include <kernel_information.h>
#include <memory_support.h>
#include <pid.h>
#include <synchronization.h>

const int PHYMEMMANAGER_VERSION = 2; //Increase this when changes require a modification of
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
        PhyMemChunk* phy_mmap; //A map of the whole memory
        PhyMemChunk* phy_highmmap; //A map of high memory (addresses >0x100000)
        PhyMemChunk* free_lowmem; //A noncontiguous chunk representing free low memory
        PhyMemChunk* free_mem; //A noncontiguous chunk representing free high memory
        PhyMemChunk* free_mapitems; //A collection of spare PhyMemChunk objects forming a dummy chunk,
                                  //ready for use in a memory map
        PIDs* free_pids; //A collection of spare PIDs objects forming a dummy PIDs, ready for use
                         //in a memory map
        OwnerlessMutex mmap_mutex; //Hold when concurrent actions must be avoided

        //Support methods used by public methods
        bool alloc_mapitems(PhyMemChunk* free_mem_override = NULL); //Get some memory map storage space
        bool alloc_pids(PhyMemChunk* free_mem_override = NULL); //Get some PIDs storage space
        PhyMemChunk* chunk_allocator(const PID initial_owner,
                                     const size_t size,
                                     PhyMemChunk*& free_mem_used,
                                     bool contiguous);
        bool chunk_liberator(PhyMemChunk* chunk);
        bool chunk_owneradd(PhyMemChunk* chunk, const PID new_owner);
        bool chunk_ownerdel(PhyMemChunk* chunk, const PID former_owner);
        void discard_empty_chunks();
        void fill_mmap(const KernelInformation& kinfo);
        void fix_overlap(PhyMemChunk* first_chunk, PhyMemChunk* second_chunk);
        PhyMemChunk* generate_chunk(const KernelInformation& kinfo, size_t& index);
        void killer(PID target);
        void merge_with_next(PhyMemChunk* first_item); //Merge two consecutive elements of
                                                       //the memory map (in order to save space)
        void pids_liberator(PIDs& target);
        bool split_chunk(PhyMemChunk* chunk, const size_t position);
    public:
        PhyMemManager(const KernelInformation& kinfo);

        //Process management functions
        void remove_process(PID target); //Removes all traces of a PID in PhyMemManager

        //Page/chunk allocation, sharing and freeing functions
        PhyMemChunk* alloc_chunk(const PID initial_owner,     //Allocates a chunk of memory which is
                                 const size_t size = PG_SIZE, //at least "size" large. The
                                 bool contiguous = false);    //"contiguous" flag forces it to be
                                                              //physically contiguous
        bool share_chunk(const PID new_owner,  //Add owners to a chunk
                         size_t chunk_beginning);
        bool free_chunk(const PID former_owner,  //Free a chunk from a PID's grasp
                        size_t chunk_beginning); //(liberate it if it no longer has any owner)

        //x86_64-specific methods
        PhyMemChunk* alloc_lowchunk(const PID initial_owner, //Allocate a chunk of low memory
                                    const size_t size = PG_SIZE,
                                    bool contiguous = false);

        //WARNING : Functions after this point are nonstandard, subject to change without
        //warnings, and should not be read or manipulated by external software.

        //Dumps the memory map, used by VirMemManager() to derive the kernel's virtual address space.
        PhyMemChunk* dump_mmap() {return phy_mmap;}

        //Debug methods
        void print_highmmap(); //Print a map of high memory (>=1MB)
        void print_lowmmap(); //Print a map of low memory (<1MB)
        void print_mmap(); //Print the full memory map
};

#endif