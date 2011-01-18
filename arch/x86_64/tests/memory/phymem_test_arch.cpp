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

#include <align.h>
#include <kmem_allocator.h>
#include <phymem_test_arch.h>
#include <test_display.h>

namespace Tests {
    PhyMemManager* phy_init_arch(PhyMemManager& phymem) {
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

    PhyMemMap* find_twopg_freemem(PhyMemManager& phymem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;

        //In fact, we have to make room for a third allocated page, in case PhyMemManager has to
        //allocate new map items
        static PhyMemMap storage_space[3];
        unsigned int to_be_allocd = 2*PG_SIZE;
        if(phymem_state->free_mapitems->buddy_length() < 2) to_be_allocd+=PG_SIZE;

        //Then make a copy of the relevant elements of phy_highmem and return it
        if(phymem_state->free_highmem == NULL) return NULL;
        storage_space[0] = *(phymem_state->free_highmem);
        if(storage_space[0].size >= to_be_allocd) {
            storage_space[0].next_buddy = NULL;
            return &(storage_space[0]);
        }
        to_be_allocd-=storage_space[0].size;
        if(storage_space[0].next_buddy == NULL) return NULL;
        storage_space[1] = *(storage_space[0].next_buddy);
        storage_space[0].next_buddy = &(storage_space[1]);
        if(storage_space[1].size >= to_be_allocd) {
            storage_space[1].next_buddy = NULL;
            return &(storage_space[0]);
        }
        to_be_allocd-=storage_space[1].size;
        if(storage_space[1].next_buddy == NULL) return NULL;
        storage_space[2] = *(storage_space[1].next_buddy);
        storage_space[1].next_buddy = &(storage_space[2]);
        return &(storage_space[0]);
    }

    bool check_twopg_freemem(PhyMemManager& phymem, PhyMemMap* free_phy_mem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;

        //First check if only the two requested pages have been allocated in the provided space,
        //which is the most common case
        unsigned int remaining_space = 2*PG_SIZE;
        addr_t final_location;
        if(free_phy_mem->size <= remaining_space) {
            remaining_space-= free_phy_mem->size;
            final_location = free_phy_mem->next_buddy->location+remaining_space;
        } else {
            final_location = free_phy_mem->location+remaining_space;
        }
        if(phymem_state->free_highmem->location == final_location) return true;

        //If it's not the case, it might be possible that phymem has allocated extra map items,
        //which would mean that one more page has been allocated. In this case, three pages have
        //been allocated, and stuff from free_mapitems is in one of them.
        remaining_space = 3*PG_SIZE;
        bool free_mapitems_found = false;
        addr_t free_mapitems_location = (addr_t) (phymem_state->free_mapitems);
        PhyMemMap* studied_item = free_phy_mem;
        while(remaining_space) {
            if(studied_item->size <= remaining_space) {
                if((free_mapitems_location >= studied_item->location) &&
                   (free_mapitems_location < studied_item->location + studied_item->size)) {
                    free_mapitems_found = true;
                }
                remaining_space-= studied_item->size;
                studied_item = studied_item->next_buddy;
            } else {
                if((free_mapitems_location >= studied_item->location) &&
                   (free_mapitems_location < studied_item->location + remaining_space)) {
                    free_mapitems_found = true;
                }
                final_location = studied_item->location + remaining_space;
                remaining_space = 0;
            }
        }

        if(free_mapitems_found && (phymem_state->free_highmem->location == final_location)) {
            return true;
        }

        test_failure("The required memory has not been allocated properly");
        return false;
    }

    PhyMemState* save_phymem_state(PhyMemManager& phymem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;

        //Allocate storage space.
        PhyMemState* result = (PhyMemState*) kalloc(sizeof(PhyMemState));
        if(!result) {
            test_failure("Could not allocate storage space");
            return NULL;
        }
        result->phy_mmap = (PhyMemMap*) kalloc(sizeof(PhyMemMap)*(phymem_state->phy_mmap->length()));
        if(result->phy_mmap == NULL) {
            kfree(result);
            test_failure("Could not allocate storage space");
            return NULL;
        }

        //Parse phymem's memory map and make a copy of it. Set up phy_highmmap, free_lowmem,
        //and free_highmem in the way. Manage buddies in the simplest case where only free map items
        //use buddies, sorted in the original map's order
        PhyMemMap* last_buddy_source = NULL;
        PhyMemMap* last_buddy_dest = NULL;
        bool more_buddies = false; //Whether more complex buddy management is needed.
        PhyMemMap* source_parser = phymem_state->phy_mmap;
        PhyMemMap* dest_parser = result->phy_mmap;
        while(source_parser) {
            //Manage phy_highmmap, free_lowmem, and free_highmem
            if(phymem_state->phy_highmmap == source_parser) {
                result->phy_highmmap = dest_parser;
            }
            if(phymem_state->free_lowmem == source_parser) {
                result->free_lowmem = dest_parser;
            }
            if(phymem_state->free_highmem == source_parser) {
                result->free_highmem = dest_parser;
            }
            //Manage buddies in the simplest case (only free_*mem uses buddies)
            if((last_buddy_source) && (last_buddy_source->next_buddy == source_parser)) {
                last_buddy_dest->next_buddy = dest_parser;
                last_buddy_source = NULL;
            }
            if(source_parser->next_buddy) {
                if(last_buddy_source) {
                    more_buddies = true;
                } else {
                    last_buddy_source = source_parser;
                    last_buddy_dest = dest_parser;
                }
            }
            //Fill the chunk's properties
            dest_parser->location = source_parser->location;
            dest_parser->size = source_parser->size;
            dest_parser->owners = source_parser->owners;
            dest_parser->allocatable = source_parser->allocatable;
            dest_parser->next_buddy = NULL;
            dest_parser->next_mapitem = dest_parser+1;
            //Move to the next item
            source_parser = source_parser->next_mapitem;
            if(source_parser) dest_parser = dest_parser->next_mapitem;
        }
        dest_parser->next_mapitem = NULL;
        if(last_buddy_source) more_buddies = true;

        //Introduce more fine-grained buddy management, if needed
        if(more_buddies) {
            //Parse the source and dest map, looking for unmanaged buddies.
            source_parser = phymem_state->phy_mmap;
            dest_parser = result->phy_mmap;
            while(source_parser) {
                if((source_parser->next_buddy) && !(dest_parser->next_buddy)) {
                    //A missing buddy was found !
                    //Parse the source and dest map a second time, looking for the missing buddy
                    PhyMemMap* buddy_parser_source = phymem_state->phy_mmap;
                    PhyMemMap* buddy_parser_dest = result->phy_mmap;
                    while(buddy_parser_source) {
                        if(source_parser->next_buddy == buddy_parser_source) break;
                        buddy_parser_source = buddy_parser_source->next_mapitem;
                        buddy_parser_dest = buddy_parser_dest->next_mapitem;
                    }
                    dest_parser->next_buddy = buddy_parser_dest;
                }
                source_parser = source_parser->next_mapitem;
                dest_parser = dest_parser->next_mapitem;
            }
        }

        //Count the amount of items in free_mapitems
        source_parser = phymem_state->free_mapitems;
        addr_t free_count = 0;
        while(source_parser) {
            //Move to the next item
            ++free_count;
            source_parser = source_parser->next_buddy;
        }
        result->free_mapitems = (PhyMemMap*) free_count;

        //Initialize mutex
        result->mmap_mutex = KernelMutex();

        return result;
    }

    bool cmp_phymem_state(PhyMemManager& current_phymem, PhyMemState* state) {
        PhyMemState* current_state = (PhyMemState*) &current_phymem;

        //Compare phy_mmap, phy_highmmap, free_lowmem, and free_highmem
        PhyMemMap* parser1 = current_state->phy_mmap;
        PhyMemMap* parser2 = state->phy_mmap;
        PhyMemMap* last_buddy1 = NULL;
        PhyMemMap* last_buddy2 = NULL;
        bool more_buddies = false;
        while(parser1 && parser2) {
            //Manage phy_highmmap, free_lowmem, and free_highmem
            if((current_state->phy_highmmap == parser1) || (state->phy_highmmap == parser2)) {
                if(!((current_state->phy_highmmap == parser1) &&
                     (state->phy_highmmap == parser2))) {
                    test_failure("phy_highmmap mismatch");
                    return false;
                }
            }
            if((current_state->free_lowmem == parser1) || (state->free_lowmem == parser2)) {
                if(!((current_state->free_lowmem == parser1) &&
                     (state->free_lowmem == parser2))) {
                    test_failure("free_lowmem mismatch");
                    return false;
                }
            }
            if((current_state->free_highmem == parser1) || (state->free_highmem == parser2)) {
                if(!((current_state->free_highmem == parser1) &&
                     (state->free_highmem == parser2))) {
                    test_failure("free_highmem mismatch");
                    return false;
                }
            }
            //Manage buddies in the simplest case (only free_*mem uses them)
            if((last_buddy1) && (last_buddy1->next_buddy == parser1)) {
                if(last_buddy2->next_buddy != parser2) {
                    test_failure("phy_mmap mismatch");
                    return false;
                }
            }
            if(parser1->next_buddy) {
                if(parser2->next_buddy == NULL) {
                    test_failure("phy_mmap mismatch");
                    return false;
                }
                if(last_buddy1) {
                    more_buddies = true;
                } else {
                    last_buddy1 = parser1;
                    last_buddy2 = parser2;
                }
            }
            //Check the chunk's other properties
            if(parser1->location != parser2->location) {
                test_failure("phy_mmap mismatch");
                return false;
            }
            if(parser1->size != parser2->size) {
                test_failure("phy_mmap mismatch");
                return false;
            }
            if(parser1->owners != parser2->owners) {
                test_failure("phy_mmap mismatch");
                return false;
            }
            if(parser1->allocatable != parser2->allocatable) {
                test_failure("phy_mmap mismatch");
                return false;
            }
            //Go to the next items
            parser1 = parser1->next_mapitem;
            parser2 = parser2->next_mapitem;
        }
        if(parser1 || parser2) {
            test_failure("phy_mmap mismatch");
            return false;
        }
        if(last_buddy1) more_buddies = true;

        //More sophisticated buddy management, when needed
        if(more_buddies) {
            //Parse the source and dest map, looking for unmanaged buddies.
            parser1 = current_state->phy_mmap;
            parser2 = state->phy_mmap;
            while(parser1) {
                if(parser1->next_buddy) {
                    //Parse the source and dest map a second time, looking for the missing buddy
                    PhyMemMap* buddy_parser1 = current_state->phy_mmap;
                    PhyMemMap* buddy_parser2 = state->phy_mmap;
                    while(buddy_parser1) {
                        if(parser1->next_buddy == buddy_parser1) break;
                        buddy_parser1 = buddy_parser1->next_mapitem;
                        buddy_parser2 = buddy_parser2->next_mapitem;
                    }
                    if(parser2->next_buddy != buddy_parser2) {
                        test_failure("phy_mmap mismatch");
                        return false;
                    }
                }
                parser1 = parser1->next_mapitem;
                parser2 = parser2->next_mapitem;
            }
        }

        //Check free_mapitems
        PhyMemMap* free_parser = current_state->free_mapitems;
        PhyMemMap should_be; //Freshly initialized map item, for reference purposes
        unsigned int free_length = 0;
        while(free_parser) {
            should_be.next_buddy = free_parser->next_buddy;
            if(*free_parser != should_be) {
                test_failure("free_mapitems mismatch");
                return false;
            }
            ++free_length;
            free_parser = free_parser->next_buddy;
        }
        if(free_length != (addr_t) (state->free_mapitems)) {
            test_failure("free_mapitems mismatch");
            return false;
        }

        //Check mutex
        if(current_state->mmap_mutex != state->mmap_mutex) {
            test_failure("mmap_mutex mismatch");
            return false;
        }

        return true;
    }

    void discard_phymem_state(PhyMemState* saved_state) {
        //Free the saved state. This is easier than you may think, thank to phy_mmap being
        //allocated as a single big block of memory. We must be careful with the map owners, though.
        PhyMemMap* map_parser = saved_state->phy_mmap;
        while(map_parser) {
            map_parser->clear_owners();
            map_parser = map_parser->next_mapitem;
        }
        kfree(saved_state->phy_mmap);
        kfree(saved_state);
    }
}