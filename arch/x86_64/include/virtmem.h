 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

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

#ifndef _VIRTMEM_H_
#define _VIRTMEM_H_

#include <address.h>
#include <memory_support.h>
#include <physmem.h>
#include <pid.h>
#include <synchronization.h>

const int VIRMEMMANAGER_VERSION = 1; //Increase this when deep changes require a modification of
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
        PhyMemManager* phymem;
        VirMapList* map_list;
        VirMemMap* free_mapitems; //A collection of ready to use virtual memory map items
                                  //(chained using next_buddy)
        VirMapList* free_listitems; //A collection of ready to use map list items
        KernelMutex maplist_mutex; //Hold that mutex when parsing the map list
                                   //or adding/removing maps from it.
        //Support methods
        addr_t alloc_mapitems(); //Get some memory map storage space
        addr_t alloc_listitems(); //Get some map list storage space
        VirMemMap* chunk_liberator(VirMemMap* chunk);
        VirMemMap* chunk_mapper(const PhyMemMap* phys_chunk,
                                const VirMemFlags flags,
                                VirMapList* target);
        VirMapList* find_pid(PID target); //Find the map list entry associated to this PID,
                                          //return NULL if it does not exist.
        VirMapList* find_or_create_pid(PID target); //Same as above, but try to create the entry
                                                    //if it does not exist yet
        VirMapList* setup_pid(PID target); //Create management structures for a new PID
        VirMapList* remove_pid(PID target); //Discards management structures for this PID
        VirMemMap* flag_adjust(VirMemMap* chunk,        //Adjust the paging flags associated with a
                               const VirMemFlags flags, //chunk.
                               const VirMemFlags mask,
                               VirMapList* target);
        addr_t setup_4kpages(addr_t vir_addr,     //Setup paging structures for 4KB x86 paging in
                             const addr_t length, //a virtual address range.
                             addr_t pml4t_location); 
        addr_t remove_paging(addr_t vir_addr, //Remove paging structures in a virtual address range
                             const addr_t length,
                             addr_t pml4t_location);
        addr_t remove_all_paging(VirMapList* target);
        uint64_t x86flags(VirMemFlags flags); //Converts VirMemFlags to x86 paging flags
    public:
        //Constructor gets the current layout of paged memory, setup management structures
        VirMemManager(PhyMemManager& physmem);
        
        //Map a non-contiguous chunk of physical memory as a contiguous chunk of the target's virtual memory
        VirMemMap* map(const PhyMemMap* phys_chunk,
                       const PID target,
                       const VirMemFlags flags = VMEM_FLAG_R + VMEM_FLAG_W + VMEM_FLAG_P);
                       
        //Destroy a chunk of virtual memory
        VirMemMap* free(VirMemMap* chunk);
        
        //Change a chunk's flags (including in page tables, of course)
        VirMemMap* adjust_flags(VirMemMap* chunk, const VirMemFlags flags, const VirMemFlags mask);
        VirMemMap* set_flags(VirMemMap* chunk, const VirMemFlags flags) {
            return adjust_flags(chunk, flags, ~0);
        }
        
        //Remove all traces of a process in VirMemManager. Does not affect physical chunks, prefer
        //the MemAllocator variant in most cases.
        void kill(PID target);
        
        //Prepare for a context switch by giving the CR3 value to load before jumping
        void cr3_value(PID target);
        
        //Debug methods. Will go out in final release.
        void print_maplist();
        void print_mmap(PID owner);
        void print_pml4t(PID owner);
};

#endif