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

#include <memory_support.h>
#include <physmem.h>
#include <test_display.h>
#include <virmem_test_arch.h>
#include <x86paging.h>

namespace Tests {
    VirMapList* virinit_maplist_tests(VirMapList* init_maplist) {
        if(init_maplist->pml4t_location != x86paging::get_pml4t()) {
            test_failure("Kernel's paging structures are incorrectly referenced");
            return NULL;
        }

        return init_maplist;
    }

    bool virinit_check_knl_location(VirMemState* virmem_state) {
        addr_t kernel_pml4t = virmem_state->map_list->pml4t_location;

        addr_t knl_phy_rx_loc = x86paging::get_target(virmem_state->knl_rx_loc, kernel_pml4t);
        PhyMemMap* phy_knl_rx = virmem_state->phymem->find_this(knl_phy_rx_loc);
        if(phy_knl_rx != virmem_state->phy_knl_rx) {
            test_failure("Knl RX segment location is wrong");
            return false;
        }
        addr_t knl_phy_r_loc = x86paging::get_target(virmem_state->knl_r_loc, kernel_pml4t);
        PhyMemMap* phy_knl_r = virmem_state->phymem->find_this(knl_phy_r_loc);
        if(phy_knl_r != virmem_state->phy_knl_r) {
            test_failure("Knl R segment location is wrong");
            return false;
        }
        addr_t knl_phy_rw_loc = x86paging::get_target(virmem_state->knl_rw_loc, kernel_pml4t);
        PhyMemMap* phy_knl_rw = virmem_state->phymem->find_this(knl_phy_rw_loc);
        if(phy_knl_rw != virmem_state->phy_knl_rw) {
            test_failure("Knl RW segment location is wrong");
            return false;
        }

        return true;
    }
}