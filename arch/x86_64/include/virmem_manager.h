 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

      Copyright (C) 2010-2012  Hadrien Grasland

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

#ifndef _VIRTMEM_H_
#define _VIRTMEM_H_

#include <address.h>
#include <memory_support.h>
#include <phymem_manager.h>
#include <pid.h>
#include <synchronization.h>

const int VIRMEMMANAGER_VERSION = 2; //Increase this when deep changes require a modification of
                                     //the testing protocol

//This class is an abstraction of the paging mechanism. It allows...
//-Page grouping in chunks and management of chunks as a whole
//-Allocation/miberation of a contiguous chunk of linear addresses pointing to a
// non-contiguous chunk of physical addresses.
//-Interoperability with the physical memory manager
//-Transparent management of per-process page tables (and hence multiple process management)
//
//Later : -Switching processes
class VirMemManager {
    private:
        PhyMemManager* phymem_manager;
        OwnerlessMutex maplist_mutex; //Hold that mutex when parsing the map list
                                   //or adding/removing maps from it.
        VirMemProcess* process_list;
        VirMemChunk* free_mapitems; //A collection of ready to use virtual memory map items
                                  //(chained using next_buddy)
        VirMemProcess* free_process_descs; //A collection of ready to use process descriptors

        //Support methods
        bool alloc_mapitems(); //Get some memory map storage space
        bool alloc_process_descs(); //Get some map list storage space
        VirMemChunk* alloc_virtual_address_space(VirMemProcess* target,
                                                 size_t location, //NULL if no specific location is requested
                                                 size_t size);
        VirMemChunk* chunk_mapper(VirMemProcess* target,
                                  const PhyMemChunk* phys_chunk,
                                  const VirMemFlags flags,
                                  size_t location = NULL);
        bool chunk_liberator(VirMemProcess* target,
                             VirMemChunk* chunk);
        VirMemProcess* find_pid(const PID target); //Find the map list entry associated to this PID,
                                                //return NULL if it does not exist.
        VirMemProcess* find_or_create_pid(PID target); //Same as above, but try to create the entry
                                                       //if it does not exist yet
        VirMemChunk* flag_adjust(VirMemProcess* target, //Adjust the paging flags associated with a chunk
                                 VirMemChunk* chunk,
                                 const VirMemFlags flags,
                                 const VirMemFlags mask);
        bool map_k_chunks(VirMemProcess* target); //Maps K chunks in a newly created address space
        bool map_kernel(); //Maps the kernel's initial address space during initialization
        bool remove_all_paging(VirMemProcess* target);
        bool remove_pid(PID target); //Discards management structures for this PID
        VirMemProcess* setup_pid(PID target); //Create management structures for a new PID
        void unmap_k_chunk(VirMemChunk* chunk); //Removes K pages from the address space of non-kernel processes.
        uint64_t x86flags(VirMemFlags flags); //Converts VirMemFlags to x86 paging flags
    public:
        //Constructor gets the current layout of paged memory, setup management structures
        VirMemManager(PhyMemManager& physmem);

        //Process management functions
        void remove_process(PID target); //Removes all traces of a PID in VirMemManager

        //Chunk manipulation functions
        VirMemChunk* map_chunk(const PID target,               //Map a non-contiguous physical memory chunk as a
                               const PhyMemChunk* phys_chunk,  //contiguous virtual chunk in the target address space
                               const VirMemFlags flags = VIRMEM_FLAGS_RW);
        bool free_chunk(const PID target, //Unmaps a chunk of virtual memory
                        size_t chunk_beginning);
        VirMemChunk* adjust_chunk_flags(const PID target, //Change a virtual chunk's properties
                                        size_t chunk_beginning,
                                        const VirMemFlags flags,
                                        const VirMemFlags mask);

        //x86_64 specific.
        //Prepare for a context switch by giving the CR3 value to load before jumping
        uint64_t cr3_value(const PID target);

        //Debug methods. Will go out in final release.
        void print_maplist();
        void print_mmap(PID owner);
        void print_pml4t(PID owner);
};

#endif
