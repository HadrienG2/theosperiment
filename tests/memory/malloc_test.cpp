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

#include <align.h>
#include <malloc_test.h>
#include <memory_support.h>
#include <test_display.h>
#include <physmem.h>
#include <phymem_test_arch.h>

#include <dbgstream.h>

namespace Tests {
    bool meta_mallocator() {
        item_title("Check MemAllocator version");
        if(MALLOC_TEST_VERSION != MEMALLOCATOR_VERSION) {
            test_failure("Test and MemAllocator versions mismatch");
            return false;
        }
        return true;
    }

    MemAllocator* init_mallocator(PhyMemManager& phymem, VirMemManager& virmem) {
        item_title("Initialize a MemAllocator object");
        //We must first take note of where free space would be allocated in physical memory during
        //mallocator's initialization, in order to check that it has been done properly later.
        PhyMemMap* free_phy_mem = find_twopg_freemem(phymem);
        if(!free_phy_mem) {
            test_failure("Not enough physical memory left");
            return NULL;
        }
        static MemAllocator mallocator(phymem, virmem);
        MemAllocatorState* mallocator_state = (MemAllocatorState*) &mallocator;

        item_title("Check phymem-virmem-mallocator integration");
        if(mallocator_state->phymem != &phymem) {
            test_failure("MemAllocator->phymem points to an incorrect location");
            return NULL;
        }
        if(mallocator_state->virmem != &virmem) {
            test_failure("MemAllocator->virmem points to an incorrect location");
            return NULL;
        }
        if(!check_twopg_freemem(phymem, free_phy_mem)) return NULL;

        item_title("Check that map_list, knl_free_map and knl_busy_map are all NULL");
        if(mallocator_state->map_list) {
            test_failure("There's a nonzero pointer in map_list");
            return NULL;
        }
        if(mallocator_state->knl_free_map) {
            test_failure("There's a nonzero pointer in knl_free_map");
            return NULL;
        }
        if(mallocator_state->knl_busy_map) {
            test_failure("There's a nonzero pointer in knl_busy_map");
            return NULL;
        }

        item_title("Check that map and list items are properly allocated");
        if(!malinit_check_mapitems(mallocator_state)) return NULL;
        if(!malinit_check_listitems(mallocator_state)) return NULL;

        item_title("Check that knl_mutex and maplist_mutex are available");
        if(mallocator_state->knl_mutex.state() == 0) {
            test_failure("knl_mutex is not available");
            return NULL;
        }
        if(mallocator_state->maplist_mutex.state() == 0) {
            test_failure("maplist_mutex is not available");
            return NULL;
        }

        item_title("Setup kalloc with our MemAllocator, so that we may use it later");
        setup_kalloc(mallocator);
        
        item_title("Allocate 1 byte of data with kalloc");
        kalloc(1);

        return &mallocator;
    }

    bool test_mallocator(MemAllocator& mallocator) {
        fail_notimpl();
        return false;
    }

    bool malinit_check_mapitems(MemAllocatorState* mallocator_state) {
        //A well-aligned page of empty map items should be put in free_mapitems.
        //Check alignment, bad alignment means that map items have leaked.
        addr_t mapitems_location = (addr_t) mallocator_state->free_mapitems;
        if(mapitems_location != align_pgdown(mapitems_location)) {
            test_failure("Map items have leaked");
            return false;
        }

        //Check that the map items are initialized properly and that the full page is occupied
        unsigned int used_space = 0;
        MallocMap* current_item = mallocator_state->free_mapitems;
        MallocMap should_be;
        should_be.next_item = current_item+1;
        while(current_item) {
            used_space+= sizeof(MallocMap);
            if(*current_item != should_be) {
                test_failure("Map items are not initialized properly");
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
            test_failure("Map items have leaked");
            return false;
        }
        if(used_space > PG_SIZE) {
            test_failure("Allocated space overflow");
            return false;
        }

        //That page should have been allocated first.
        PhyMemMap* allocd_page = mallocator_state->phymem->find_this(mapitems_location);
        if((!allocd_page) ||
           (allocd_page->size != PG_SIZE) ||
           (allocd_page->owners[0]!=PID_KERNEL) ||
           (allocd_page->owners[1]!=PID_NOBODY)) {
            test_failure("The page used by map items has not been allocated properly");
            return false;
        }

        return true;
    }

    bool malinit_check_listitems(MemAllocatorState* mallocator_state) {
        //A well-aligned page of empty list items should be put in free_listitems.
        //Check alignment, bad alignment means that map items have leaked.
        addr_t listitems_location = (addr_t) mallocator_state->free_listitems;
        if(listitems_location != align_pgdown(listitems_location)) {
            test_failure("List items have leaked");
            return false;
        }

        //Check that the map items are initialized properly and that the full page is occupied
        unsigned int used_space = 0;
        MallocPIDList* current_item = mallocator_state->free_listitems;
        MallocPIDList should_be;
        should_be.next_item = current_item+1;
        while(current_item) {
            used_space+= sizeof(MallocPIDList);
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
        PhyMemMap* allocd_page = mallocator_state->phymem->find_this(listitems_location);
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