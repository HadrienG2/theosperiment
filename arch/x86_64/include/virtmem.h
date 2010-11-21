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

//This class is an abstraction of the paging mechanism. It allows...
//-Page grouping in chunks and management of chunks as a whole (i.e. setting permission RW on address range 0x1000->0x7000)
//-Allocation/Liberation of a contiguous chunk of linear addresses pointing to a non-contiguous chunk of physical addresses.
//-Interoperability with the physical memory manager
//-Transparent management of per-process page tables (and hence multiple process management)
//
//Later : -Memory sharing functions
//        -Use something else than 2MB pages sometimes ?
//        -Removing all traces of a process from memory (when it's killed or closed), switching processes
class VirMemManager {
  private:
    PhyMemManager* phymem;
    VirMapList* map_list;
    VirMemMap* free_mapitems; //A collection of ready to use virtual memory map items forming a dummy chunk.
    VirMapList* free_listitems; //A collection of ready to use map list items forming a dummy list.
    KernelMutex maplist_mutex; //Hold that mutex when parsing the map list or adding/removing maps from it.
    //Support methods
    addr_t alloc_mapitems(); //Get some memory map storage space
    addr_t alloc_listitems(); //Get some map list storage space
    VirMemMap* chunk_liberator(VirMemMap* chunk, VirMapList* target);
    VirMemMap* chunk_mapper(const PhyMemMap* phys_chunk, const VirMemFlags flags, VirMapList* target);
    VirMapList* find_or_create_pid(PID target); //Same as below, but create the entry if it does not exist yet
    VirMapList* find_pid(PID target);  //Find the map list entry associated to this PID
    addr_t setup_4kpages(addr_t vir_addr, const addr_t length, addr_t pml4t_location); //Setup paging structures for 4KB x86 paging in
                                                                                       //a virtual address range.
    VirMapList* setup_pid(PID target); //Create management structures for a new PID
    addr_t remove_paging(addr_t vir_addr, const addr_t length, addr_t pml4t_location); //Remove paging structures in a virtual address range
    VirMapList* remove_pid(PID target); //Remove management structures for this PID
    uint64_t x86flags(VirMemFlags flags); //Converts VirMemFlags to x86 paging flags
  public:
    //Constructor gets the current layout of paged memory, setup management structures
    VirMemManager(PhyMemManager& physmem);
    
    //Destroy a chunk of virtual memory
    VirMemMap* free(VirMemMap* chunk, const PID target);
    //Map a non-contiguous chunk of physical memory as a contiguous chunk of the target's virtual memory
    VirMemMap* map(const PhyMemMap* phys_chunk, const VirMemFlags flags, const PID target);
    //Change a chunk's flags (including in page tables, of course)
    VirMemMap* set_flags(VirMemMap* chunk, const VirMemFlags flags);
    
    //Debug methods. Will go out in final release.
    void print_maplist();
    void print_mmap(PID owner);
};

#endif