 /* Virtual memory management testing routines

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

#include <align.h>
#include <memory_support.h>
#include <phymem_test_arch.h>
#include <test_display.h>
#include <virmem_test.h>
#include <virmem_test_arch.h>

extern char knl_rx_start, knl_r_start, knl_rw_start;

namespace Tests {
    bool meta_virmem() {
        item_title("Check VirMemManager version");
        if(VIRMEM_TEST_VERSION != VIRMEMMANAGER_VERSION) {
            test_failure("Test and VirMemManager versions mismatch");
            return false;
        }
        return true;
    }

    VirMemManager* init_virmem(PhyMemManager& phymem) {
        item_title("Initialize a VirMemManager object");
        //We must first take note of where free space would be allocated in physical memory
        //during virmem initialization, in order to check that it has been done properly later
        PhyMemMap* free_phy_mem = find_twopg_freemem(phymem);
        if(!free_phy_mem) {
            test_failure("Not enough physical memory left");
            return NULL;
        }
        static VirMemManager virmem(phymem);
        VirMemState* virmem_state = (VirMemState*) &virmem;

        item_title("Check phymem-virmem integration");
        if(virmem_state->phymem != &phymem) {
            test_failure("VirMemManager->phymem points to an incorrect location");
            return NULL;
        }
        if(!check_twopg_freemem(phymem, free_phy_mem)) return NULL;

        item_title("Check availability of maplist_mutex");
        if(virmem_state->maplist_mutex.state() == 0) {
            test_failure("maplist_mutex not available in a freshly initialized VirMemManager");
            return NULL;
        }

        item_title("Check the state of map_list");
        VirMapList* maplist_entry = virmem_state->map_list;
        if((!maplist_entry) || (maplist_entry->next_item)) {
            test_failure("There should be one single item in map_list after initialization");
            return NULL;
        }
        if(maplist_entry->map_owner != PID_KERNEL) {
            test_failure("Map owner must be PID_KERNEL");
            return NULL;
        }
        if(maplist_entry->map_pointer) {
            test_failure("Paging is disabled for the kernel, so map_pointer should be NULL");
            return NULL;
        }
        if(maplist_entry->mutex.state() == 0) {
            test_failure("All mutexes should be available at this stage");
            return NULL;
        }
        if(!virinit_maplist_tests(maplist_entry)) return NULL;

        item_title("Check that map and list items are properly allocated");
        if(!virinit_check_mapitems(virmem_state)) return NULL;
        if(!virinit_check_listitems(virmem_state)) return NULL;

        item_title("Check the information about virtual kernel location");
        if(virmem_state->knl_rx_loc != (addr_t) &knl_rx_start) {
            test_failure("Knl RX segment location is wrong");
            return NULL;
        }
        if(virmem_state->knl_r_loc != (addr_t) &knl_r_start) {
            test_failure("Knl R segment location is wrong");
            return NULL;
        }
        if(virmem_state->knl_rw_loc != (addr_t) &knl_rw_start) {
            test_failure("Knl RW segment location is wrong");
            return NULL;
        }

        item_title("Check the information about physical kernel location");
        if(!virinit_check_knl_location(virmem_state)) return NULL;

        return &virmem;
    }

    bool test_virmem(VirMemManager& virmem) {
        fail_notimpl();
        return false;
    }

    bool virinit_check_mapitems(VirMemState* virmem_state) {
        //A well-aligned page of empty map items should be put in free_mapitems.
        //Check alignment, bad alignment means that map items have leaked.
        addr_t mapitems_location = (addr_t) virmem_state->free_mapitems;
        if(mapitems_location != align_pgdown(mapitems_location)) {
            test_failure("Map items have leaked");
            return false;
        }

        //Check that the map items are initialized properly and that the full page is occupied
        unsigned int used_space = 0;
        VirMemMap* current_item = virmem_state->free_mapitems;
        VirMemMap should_be;
        should_be.next_buddy = current_item+1;
        while(current_item) {
            used_space+= sizeof(VirMemMap);
            if(*current_item != should_be) {
                test_failure("Map items are not initialized properly");
                return false;
            }
            current_item = current_item->next_buddy;
            if(current_item->next_buddy) {
                ++(should_be.next_buddy);
            } else {
                should_be.next_buddy = NULL;
            }
        }
        if(used_space < PG_SIZE) {
            test_failure("Map items have leaked");
            return false;
        }
        if(used_space > PG_SIZE) {
            test_failure("Allocated space overflow");
            return false;
        }

        //That page should have been allocated first.
        PhyMemMap* allocd_page = virmem_state->phymem->find_thischunk(mapitems_location);
        if((!allocd_page) ||
           (allocd_page->size != PG_SIZE) ||
           (allocd_page->owners[0]!=PID_KERNEL) ||
           (allocd_page->owners[1]!=PID_NOBODY)) {
            test_failure("The page used by map items has not been allocated properly");
            return false;
        }

        return true;
    }

    bool virinit_check_listitems(VirMemState* virmem_state) {
        //A well-aligned page of empty list items should be put in free_listitems, except for the
        //first item which should have went in map_list.
        //Check alignment, bad alignment means that list items have leaked.
        addr_t listitems_location = (addr_t) virmem_state->map_list;
        if(listitems_location != align_pgdown(listitems_location)) {
            test_failure("List items have leaked");
            return false;
        }

        //Check that the map items are initialized properly and that the full page is occupied
        unsigned int used_space = sizeof(VirMapList);
        VirMapList* current_item = virmem_state->free_listitems;
        if(current_item != (virmem_state->map_list)+1) {
            test_failure("List items are not initialized properly");
            return false;
        }
        VirMapList should_be;
        should_be.next_item = (virmem_state->map_list)+2;
        while(current_item) {
            used_space+= sizeof(VirMapList);
            if(*current_item != should_be) {
                test_failure("List items are not initialized properly");
                return false;
            }
            current_item = current_item->next_item;
            if(current_item->next_item) {
                ++(should_be.next_item);
            } else {
                should_be.next_item = NULL;
            }
        }
        if(used_space < PG_SIZE) {
            test_failure("List items have leaked");
            return false;
        }
        if(used_space > PG_SIZE) {
            test_failure("Allocated space overflow");
            return false;
        }

        //That page should have been allocated first.
        PhyMemMap* allocd_page = virmem_state->phymem->find_thischunk(listitems_location);
        if((!allocd_page) ||
           (allocd_page->size != PG_SIZE) ||
           (allocd_page->owners[0]!=PID_KERNEL) ||
           (allocd_page->owners[1]!=PID_NOBODY)) {
            test_failure("The page used by list items has not been allocated properly");
            return false;
        }

        return true;
    }
}