 /* Memory allocation testing routines

      Copyright (C) 2011  Hadrien Grasland

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

#ifndef _MALLOC_TEST_H_
#define _MALLOC_TEST_H_

#include <kmem_allocator.h>
#include <physmem.h>
#include <virtmem.h>

namespace Tests {
    const int MALLOC_TEST_VERSION = 1;

    //The internal state of a MemAllocator, as described in kmem_allocator.h
    struct MemAllocatorState {
        PhyMemManager* phymem;
        VirMemManager* virmem;
        MallocPIDList* map_list;
        KnlMallocMap* knl_free_map;
        KnlMallocMap* knl_busy_map;
        MallocMap* free_mapitems; //As usual, this is a casted integer in a saved state, because
                                  //there's no point copying and storing a pack of blank data.
        MallocPIDList* free_listitems; //Same for this.
        KernelMutex maplist_mutex;
        KernelMutex knl_mutex;
    };

    //Main tests
    bool meta_mallocator(); //Check MemAllocator version
    MemAllocator* init_mallocator(PhyMemManager& phymem, VirMemManager& virmem);
    bool test_mallocator(MemAllocator& mallocator);

    //Auxiliary functions
    bool malinit_check_mapitems(MemAllocatorState* mallocator_state);
    bool malinit_check_listitems(MemAllocatorState* mallocator_state);
}

#endif