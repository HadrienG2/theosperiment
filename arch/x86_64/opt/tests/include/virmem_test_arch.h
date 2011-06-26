 /* Virtual memory management testing routines (arch-specific part)

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

#ifndef _VIRMEM_TEST_ARCH_H_
#define _VIRMEM_TEST_ARCH_H_

#include <address.h>
#include <memory_support.h>
#include <physmem.h>
#include <synchronization.h>

namespace Tests {
    //The internal state of a VirMemManager, as described in virtmem.h
    struct VirMemState {
        PhyMemManager* phymem;
        OwnerlessMutex maplist_mutex;
        VirMapList* map_list;
        VirMemMap* free_mapitems; //In a saved state, this is just a casted integer, counting the
                                  //number of items stored in there. As said about PhyMemState,
                                  //there's no useful information to keep in there, only a bunch
                                  //of blank VirMemMaps
        VirMapList* free_listitems; //Same as above, but with VirMapLists
        PhyMemMap* phy_knl_rx;
        PhyMemMap* phy_knl_r;
        PhyMemMap* phy_knl_rw;
        size_t knl_rx_loc;
        size_t knl_r_loc;
        size_t knl_rw_loc;
    };

    //Arch-specific tests of the initial content of map_list
    VirMapList* virinit_maplist_tests(VirMapList* init_maplist);

    //Check that the kernel's initial location is correctly mapped in virmem's state
    bool virinit_check_knl_location(VirMemState* virmem_state);
}

#endif