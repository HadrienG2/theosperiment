 /* Physical memory management testing routines (arch-specific part)

      Copyright (C) 2010-2011  Hadrien Grasland

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

#ifndef _PHYMEM_TEST_ARCH_H_
#define _PHYMEM_TEST_ARCH_H_

#include <kernel_information.h>
#include <memory_support.h>
#include <physmem.h>
#include <synchronization.h>

namespace Tests {
    //The internal state of a PhyMemManager, as described in physmem.h
    struct PhyMemState {
        PhyMemMap* phy_mmap;
        PhyMemMap* phy_highmmap;
        PhyMemMap* free_lowmem;
        PhyMemMap* free_highmem;
        PhyMemMap* free_mapitems; //In a saved state, this is in fact a casted integer : the number
                                  //of free mapitems in the stored state. There's no other useful
                                  //information in free_mapitems, so no need to keep lots of map
                                  //items around each time the state of PhyMemManager is saved
        OwnerlessMutex mmap_mutex;
    };

    //State management functions
    PhyMemState* save_phymem_state(PhyMemManager& phymem); //Save phymem's internal state in
                                                           //an internal buffer, returns pointer to
                                                           //the buffer.
    bool cmp_phymem_state(PhyMemManager& current_phymem, //Compare phymem's state to a saved state
                          PhyMemState* state);           //returns true if it's the same, displays
                                                         //why and returns false otherwise

    //Arch-specific auxiliary functions
    PhyMemManager* phy_init_arch(PhyMemManager& phymem);
    PhyMemState* phy_setstate_2_free_pgs_low(PhyMemManager& phymem);
    PhyMemState* phy_setstate_3pg_free_chunk_low(PhyMemManager& phymem);
    PhyMemState* phy_setstate_rocky_freemem_low(PhyMemManager& phymem);
    bool phy_test_arch(PhyMemManager& phymem);
    bool phy_test_contigalloc_low(PhyMemManager& phymem);
    bool phy_test_noncontigalloc_low(PhyMemManager& phymem);
    bool phy_test_rockyalloc_contig_low(PhyMemManager& phymem);
    bool phy_test_splitalloc_low(PhyMemManager& phymem);
    
    //State checkers : these functions are used to check that the when going from a known and saved
    //initial state of PhyMemManager and doing a well-defined set of operations on it, the state
    //of PhyMemManager is modified in the expected way. They are used to check that nothing has
    //changed, except the expected values, and in the expected way. They work by applying the
    //expected changes on the saved state and checking if the new state matches the changes.
    bool check_phystate_chunk_free(PhyMemManager& phymem,
                                   PhyMemState* saved_state,
                                   PhyMemMap* returned_chunk);
    bool check_phystate_chunk_free_low(PhyMemManager& phymem,
                                       PhyMemState* saved_state,
                                       PhyMemMap* returned_chunk);
    bool check_phystate_internal_alloc(PhyMemManager& phymem,
                                       PhyMemState* saved_state);
    bool check_phystate_merge_alloc(PhyMemManager& phymem,
                                    PhyMemState* saved_state);
    bool check_phystate_merge_alloc_low(PhyMemManager& phymem,
                                        PhyMemState* saved_state);
    bool check_phystate_owneradd(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* shared_page);
    bool check_phystate_ownerdel(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* shared_page);
    bool check_phystate_perfectfit_alloc(PhyMemManager& phymem,
                                         PhyMemState* saved_state);
    bool check_phystate_perfectfit_alloc_low(PhyMemManager& phymem,
                                             PhyMemState* saved_state);
    bool check_phystate_resvalloc(PhyMemManager& phymem,
                                  PhyMemState* saved_state,
                                  PhyMemMap* returned_chunk);
    bool check_phystate_resvfree(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* returned_chunk);
    bool check_phystate_rockyalloc_contig(PhyMemManager& phymem,
                                          PhyMemState* saved_state);
    bool check_phystate_rockyalloc_contig_low(PhyMemManager& phymem,
                                              PhyMemState* saved_state);
    bool check_phystate_rockyalloc_noncontig(PhyMemManager& phymem,
                                             PhyMemState* saved_state);
    bool check_phystate_rockyalloc_noncontig_low(PhyMemManager& phymem,
                                                 PhyMemState* saved_state);
    bool check_phystate_rockyfree_contig(PhyMemManager& phymem,
                                         PhyMemState* saved_state);
    bool check_phystate_rockyfree_contig_low(PhyMemManager& phymem,
                                             PhyMemState* saved_state);
    bool check_phystate_rockyfree_noncontig(PhyMemManager& phymem,
                                            PhyMemState* saved_state,
                                            PhyMemMap* returned_chunk);
    bool check_phystate_rockyfree_noncontig_low(PhyMemManager& phymem,
                                                PhyMemState* saved_state,
                                                PhyMemMap* returned_chunk);
    bool check_phystate_split_alloc(PhyMemManager& phymem,
                                    PhyMemState* saved_state);
    bool check_phystate_split_alloc_low(PhyMemManager& phymem,
                                        PhyMemState* saved_state);

    //This one is used in virmem/mallocator tests, but it's so phymem-specific that it fits here
    //better. Gathers data about the free highmem area(s) where the next two pages would be
    //allocated...
    PhyMemMap* find_twopg_freemem(PhyMemManager& phymem);
    //...and then check that those pages have been allocated properly during virmem initialization
    bool check_twopg_freemem(PhyMemManager& phymem, PhyMemMap* free_phy_mem); 
}

#endif