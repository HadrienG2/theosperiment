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
//
//Later : -Transparent management of per-process page tables (and hence multiple process management)
//        -Removing all traces of a process from memory (when it's killed or closed)
class VirMemManager {
  private:
    PhyMemManager* phymem;
    VirMapList* map_list;
    VirMemMap* free_mapitems; //A collection of ready to use virtual memory map items forming a dummy chunk.
    VirMapList* free_listitems; //A collection of ready to use map list items forming a dummy list.
    KernelMutex maplist_mutex;
    //Support methods used by public methods
    addr_t alloc_mapitems(); //Get some memory map storage space
    addr_t alloc_listitems(); //Get some map list storage space
  public:
    //Constructor gets the current layout of paged memory, setup management structures
    VirMemManager(PhyMemManager& physmem);
    
    //Map a non-contiguous chunk of physical memory as a contiguous chunk of the owner's virtual memory
    VirMemMap* map_chunk(PhyMemMap* phys_chunk, VirMemFlags flags);
    //Destroy a chunk of virtual memory and merge it with its neighbours
    VirMemMap* free(VirMemMap* chunk);
    //Change a chunk's flags
    VirMemMap* set_flags(VirMemMap* chunk, VirMemFlags flags);
    //Commit changes to the page tables of each process
    void commit();
};

#endif