 /* Memory management testing routines

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

#ifndef _MEMORY_TEST_H_
#define _MEMORY_TEST_H_

#include <kmem_allocator.h>
#include <physmem.h>
#include <virtmem.h>

namespace MemTest {
    const int PHYMEM_TEST_VERSION = 1;
    const int VIRMEM_TEST_VERSION = 1;
    const int MALLOC_TEST_VERSION = 1;

    //Global memory management regression test
    void test_memory(const KernelInformation& kinfo);
    
    //Regression tests of specific parts of memory management
    PhyMemManager* test_phymem(const KernelInformation& kinfo);
    VirMemManager* test_virmem(PhyMemManager& phymem);
    MemAllocator* test_mallocator(PhyMemManager& phymem, VirMemManager& virmem);
    
    //PhyMemManager tests
    struct PhyMemState { //The internal state of a PhyMemManager, as described in physmem.h
        PhyMemMap* phy_mmap;
        PhyMemMap* phy_highmmap;
        PhyMemMap* free_lowmem;
        PhyMemMap* free_highmem;
        PhyMemMap* free_mapitems;
        KernelMutex mmap_mutex;
    };
    bool phy_test1_meta();
    PhyMemManager* phy_test2_init(const KernelInformation& kinfo);
    
    //Helper functions
    
}

#endif