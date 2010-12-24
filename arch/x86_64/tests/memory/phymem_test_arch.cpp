 /* Physical memory management testing routines (arch-specific part)

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

#include <memory_test.h>
#include <phymem_test_arch.h>

namespace MemTest {
    PhyMemManager* phy_test2_init_arch(PhyMemManager& phymem) {
        item_title("x86_64 : Checking free_highmmap, free_lowmem and free_highmem");
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        //Find the first free and allocatable item in phy_mmap. Check it's set up properly
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        while(map_parser) {
            if((map_parser->allocatable) && (map_parser->owners[0] == PID_NOBODY)) break;
            map_parser = map_parser->next_mapitem;
        }
        if(phymem_state->free_lowmem != map_parser) {
            test_failure("free_lowmem is not set up properly");
            return NULL;
        }
        //Continue to parse phy_mmap until the frontier of low mem, check free_lowmem chaining
        PhyMemMap* last_free = map_parser;
        map_parser = map_parser->next_mapitem;
        while(map_parser) {
            if(map_parser->location >= 0x100000) break;
            if((map_parser->allocatable) && (map_parser->owners[0] == PID_NOBODY)) {
                if(map_parser != last_free->next_buddy) {
                    test_failure("free_lowmem is not set up properly");
                    return NULL;
                }
                last_free = map_parser;
            }
            map_parser = map_parser->next_mapitem;
        }
        //Check that free_lowmem terminates here and that phy_highmmap is there
        if(last_free->next_buddy != NULL) {
            test_failure("free_lowmem is not set up properly");
            return NULL;
        }
        if(phymem_state->phy_highmmap != map_parser) {
            test_failure("phy_highmmap is not set up properly");
            return NULL;
        }
        //Do the same with high memory
        while(map_parser) {
            if((map_parser->allocatable) && (map_parser->owners[0] == PID_NOBODY)) break;
            map_parser = map_parser->next_mapitem;
        }
        if(phymem_state->free_highmem != map_parser) {
            test_failure("free_highmem is not set up properly");
            return NULL;
        }
        //Continue to parse phy_mmap until the end, check free_highmem chaining
        last_free = map_parser;
        map_parser = map_parser->next_mapitem;
        while(map_parser) {
            if((map_parser->allocatable) && (map_parser->owners[0] == PID_NOBODY)) {
                if(map_parser != last_free->next_buddy) {
                    test_failure("free_highmem is not set up properly");
                    return NULL;
                }
                last_free = map_parser;
            }
            map_parser = map_parser->next_mapitem;
        }
        //Check that free_highmem terminates properly
        if(last_free->next_buddy != NULL) {
            test_failure("free_highmem is not set up properly");
            return NULL;
        }
        
        return &phymem;
    }
    
    PhyMemState* save_phymem_state(PhyMemManager& phymem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        
        //Allocate storage space
        PhyMemState* result = (PhyMemState*) kalloc(sizeof(PhyMemState));
        if(!result) {
            test_failure("Could not allocate storage space");
            return NULL;
        }        
        result->phy_mmap = (PhyMemMap*) kalloc(sizeof(PhyMemMap)*phymem_state->phy_mmap->length());
        if(result->phy_mmap == NULL) {
            kfree(result);
            test_failure("Could not allocate storage space");
            return NULL;
        }
        result->free_mapitems = (PhyMemMap*) kalloc(sizeof(PhyMemMap)*
                                                    phymem_state->free_mapitems->length());
        if(result->free_mapitems == NULL) {
            kfree(result);
            kfree(result->phy_mmap);
            test_failure("Could not allocate storage space");
            return NULL;
        }
        
        //TODO Parse phymem's memory map and make a copy of it. Set up phy_highmmap, free_lowmem,
        //and free_highmem in the way.
        
        //TODO Parse phymem's memory map a second time, checking that there are no forgotten buddies
        //TODO Make a copy of free_mapitems
        
        return result;
    }
    
    void discard_phymem_state(PhyMemState* saved_state) {
        //TODO : Free the saved state
    }
}