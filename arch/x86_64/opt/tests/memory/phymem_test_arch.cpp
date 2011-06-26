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
#include <phymem_test.h>
#include <phymem_test_arch.h>
#include <test_display.h>

namespace Tests {
    PhyMemState* save_phymem_state(PhyMemManager& phymem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;

        //Allocate storage space.
        static PhyMemState state;
        static PhyMemMap buffer[512];
        PhyMemState* result = &state;
        result->phy_mmap = buffer;

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
        size_t free_count = 0;
        while(source_parser) {
            //Move to the next item
            ++free_count;
            source_parser = source_parser->next_buddy;
        }
        result->free_mapitems = (PhyMemMap*) free_count;

        //Initialize mutex
        result->mmap_mutex = OwnerlessMutex();

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
        if(free_length != (size_t) (state->free_mapitems)) {
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
    
    PhyMemState* phy_setstate_2_free_pgs_low(PhyMemManager& phymem) {
        PhyMemMap* last_pages[2];
        
        //Allocate pages until we have two consecutive ones, if possible.
        last_pages[0] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't two free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't two free pages in there");
            return NULL;
        }
        while(last_pages[0]->location + last_pages[0]->size != last_pages[1]->location) {
            last_pages[0] = last_pages[1];
            last_pages[1] = phymem.alloc_lowpage(PID_KERNEL);
            if(last_pages[1] == NULL) {
                test_failure("There aren't two consecutive free pages in there");
                return NULL;
            }
        }
        
        //Free these pages
        phymem.free(last_pages[0]);
        phymem.free(last_pages[1]);
        
        //Save phymem's state, return the result
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return NULL;
        }
        return saved_state;
    }
    
    PhyMemState* phy_setstate_3pg_free_chunk_low(PhyMemManager& phymem) {
        PhyMemMap* last_pages[3];
        
        //Allocate pages until we have three consecutive ones, if possible.
        last_pages[0] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        last_pages[2] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[2] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        while((last_pages[0]->location + last_pages[0]->size != last_pages[1]->location) || 
          (last_pages[1]->location + last_pages[1]->size != last_pages[2]->location)) {
            last_pages[0] = last_pages[1];
            last_pages[1] = last_pages[2];
            last_pages[2] = phymem.alloc_lowpage(PID_KERNEL);
            if(last_pages[2] == NULL) {
                test_failure("There aren't three consecutive free pages in there");
                return NULL;
            }
        }
        
        //Free the three contiguous pages
        phymem.free(last_pages[0]);
        phymem.free(last_pages[1]);
        phymem.free(last_pages[2]);
        
        //Allocate a three pages contiguous chunk on top of them, free it
        PhyMemMap* chunk = phymem.alloc_lowchunk(PID_KERNEL, 3*PG_SIZE, true);
        phymem.free(chunk);
        
        //Save phymem's state, return the result
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return NULL;
        }
        return saved_state;
    }
    
    PhyMemState* phy_setstate_rocky_freemem_low(PhyMemManager& phymem) {
        PhyMemMap* last_pages[4];
        
        //Allocate pages until we have three consecutive ones, if possible.
        last_pages[0] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[2] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[2] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[3] = phymem.alloc_lowpage(PID_KERNEL);
        if(last_pages[3] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        while((last_pages[0]->location + last_pages[0]->size != last_pages[1]->location) || 
          (last_pages[1]->location + last_pages[1]->size != last_pages[2]->location) ||
          (last_pages[2]->location + last_pages[2]->size != last_pages[3]->location)) {
            last_pages[0] = last_pages[1];
            last_pages[1] = last_pages[2];
            last_pages[2] = last_pages[3];
            last_pages[3] = phymem.alloc_lowpage(PID_KERNEL);
            if(last_pages[3] == NULL) {
                test_failure("There aren't four consecutive free pages in there");
                return NULL;
            }
        }
        
        //Free the first, third and fourth page
        phymem.free(last_pages[0]);
        phymem.free(last_pages[2]);
        phymem.free(last_pages[3]);
        
        //Save phymem's state, return the result
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return NULL;
        }
        return saved_state;
    }
    
    bool phy_test_arch(PhyMemManager& phymem) {
        item_title("Repeat all allocation/liberation tests with the low memory allocators");
        //Contiguous memory allocation and liberation
        if(!phy_test_contigalloc_low(phymem)) return false;
        //Contiguous memory allocation with free item splitting
        if(!phy_test_splitalloc_low(phymem)) return false;
        //Contiguous memory allocation in non-contiguous free memory
        if(!phy_test_rockyalloc_contig_low(phymem)) return false;
        //Non-contiguous memory allocation
        if(!phy_test_noncontigalloc_low(phymem)) return false;
        
        return true;
    }
    
    bool phy_test_contigalloc_low(PhyMemManager& phymem) {
        //Refer to phy_test_contigalloc() for explanations
        PhyMemState* saved_state = phy_setstate_2_free_pgs_low(phymem);
        if(!saved_state) return false;
        PhyMemMap* returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_merge_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_perfectfit_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk->location)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        
        return true;
    }
    
    bool phy_test_noncontigalloc_low(PhyMemManager& phymem) {
        //Refer to phy_test_noncontigalloc() for explanations
        //Fragmented contiguous free space
        PhyMemState* saved_state = phy_setstate_2_free_pgs_low(phymem);
        if(!saved_state) return false;
        PhyMemMap* returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_merge_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        //"Perfect fit" situation
        returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_perfectfit_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk->location)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        //Free space is too large
        saved_state = phy_setstate_3pg_free_chunk_low(phymem);
        if(!saved_state) return false;
        returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_split_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        
        //Last test should return different results
        saved_state = phy_setstate_rocky_freemem_low(phymem);
        if(!saved_state) return false;
        returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_noncontig(returned_chunk)) return false;
        if(!check_phystate_rockyalloc_noncontig_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_rockyfree_noncontig_low(phymem, saved_state, returned_chunk)) return false;
        
        return true;;
    }
    
    bool phy_test_rockyalloc_contig_low(PhyMemManager& phymem) {
        //Refer to phy_test_rockyalloc() for explanations
        PhyMemState* saved_state = phy_setstate_rocky_freemem_low(phymem);
        if(!saved_state) return false;
        PhyMemMap* returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_rockyalloc_contig_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_rockyfree_contig_low(phymem, saved_state)) return false;
        
        return true;
    }
    
    bool phy_test_splitalloc_low(PhyMemManager& phymem) {
        //Refer to phy_test_splitalloc() for explanations
        PhyMemState* saved_state = phy_setstate_3pg_free_chunk_low(phymem);
        if(!saved_state) return false;
        PhyMemMap* returned_chunk = phymem.alloc_lowchunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_split_alloc_low(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free_low(phymem, saved_state, returned_chunk)) return false;
        
        return true;
    }
    
    bool check_phystate_chunk_free(PhyMemManager& phymem,
                                   PhyMemState* saved_state,
                                   PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->clear_owners();
        map_parser->next_buddy = saved_state->free_highmem;
        saved_state->free_highmem = map_parser;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_chunk_free_low(PhyMemManager& phymem,
                                       PhyMemState* saved_state,
                                       PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_mmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->clear_owners();
        map_parser->next_buddy = saved_state->free_lowmem;
        saved_state->free_lowmem = map_parser;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_internal_alloc(PhyMemManager& phymem,
                                       PhyMemState* saved_state) {
        static PhyMemMap buffer[2];
        //Allocate a page of map items
        buffer[0] = *(saved_state->free_highmem);
        buffer[0].location+= PG_SIZE;
        buffer[0].size-= PG_SIZE;
        saved_state->free_highmem->size = PG_SIZE;
        saved_state->free_highmem->add_owner(PID_KERNEL);
        saved_state->free_highmem->next_buddy = false;
        saved_state->free_highmem->next_mapitem = &(buffer[0]);
        saved_state->free_highmem = &(buffer[0]);
        saved_state->free_mapitems = (PhyMemMap*) (PG_SIZE/sizeof(PhyMemMap) - 1);
        //Allocate the requested page itself
        buffer[1] = *(saved_state->free_highmem);
        buffer[1].location+= PG_SIZE;
        buffer[1].size-= PG_SIZE;
        saved_state->free_highmem->size = PG_SIZE;
        saved_state->free_highmem->add_owner(PID_KERNEL);
        saved_state->free_highmem->next_buddy = false;
        saved_state->free_highmem->next_mapitem = &(buffer[1]);
        saved_state->free_highmem = &(buffer[1]);
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) - 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }

    bool check_phystate_merge_alloc(PhyMemManager& phymem,
                                    PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_highmem;
        saved_state->free_highmem = saved_state->free_highmem->next_buddy->next_buddy;
        allocation_region->size = 2*PG_SIZE;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        allocation_region->next_mapitem = allocation_region->next_mapitem->next_mapitem;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) + 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_merge_alloc_low(PhyMemManager& phymem,
                                        PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_lowmem;
        saved_state->free_lowmem = saved_state->free_lowmem->next_buddy->next_buddy;
        allocation_region->size = 2*PG_SIZE;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        allocation_region->next_mapitem = allocation_region->next_mapitem->next_mapitem;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) + 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_owneradd(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* shared_page) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != shared_page->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->owners.add_pid(42);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_ownerdel(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* shared_page) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != shared_page->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->owners.clear_pids();
        map_parser->next_buddy = saved_state->free_highmem;
        saved_state->free_highmem = map_parser;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_perfectfit_alloc(PhyMemManager& phymem,
                                         PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_highmem;
        saved_state->free_highmem = saved_state->free_highmem->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_perfectfit_alloc_low(PhyMemManager& phymem,
                                             PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_lowmem;
        saved_state->free_lowmem = saved_state->free_lowmem->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_resvalloc(PhyMemManager& phymem,
                                  PhyMemState* saved_state,
                                  PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->add_owner(PID_KERNEL);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_resvfree(PhyMemManager& phymem,
                                 PhyMemState* saved_state,
                                 PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        map_parser->clear_owners();
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyalloc_contig(PhyMemManager& phymem,
                                          PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_highmem->next_buddy;
        allocation_region->size = 2*PG_SIZE;
        allocation_region->add_owner(PID_KERNEL);
        saved_state->free_highmem->next_buddy = allocation_region->next_buddy->next_buddy;
        allocation_region->next_buddy = NULL;
        allocation_region->next_mapitem = allocation_region->next_mapitem->next_mapitem;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) + 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyalloc_contig_low(PhyMemManager& phymem,
                                              PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_lowmem->next_buddy;
        allocation_region->size = 2*PG_SIZE;
        allocation_region->add_owner(PID_KERNEL);
        saved_state->free_lowmem->next_buddy = allocation_region->next_buddy->next_buddy;
        allocation_region->next_buddy = NULL;
        allocation_region->next_mapitem = allocation_region->next_mapitem->next_mapitem;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) + 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyalloc_noncontig(PhyMemManager& phymem,
                                             PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_highmem;
        saved_state->free_highmem = saved_state->free_highmem->next_buddy->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region = allocation_region->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyalloc_noncontig_low(PhyMemManager& phymem,
                                             PhyMemState* saved_state) {
        PhyMemMap* allocation_region = saved_state->free_lowmem;
        saved_state->free_lowmem = saved_state->free_lowmem->next_buddy->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region = allocation_region->next_buddy;
        allocation_region->add_owner(PID_KERNEL);
        allocation_region->next_buddy = NULL;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyfree_contig(PhyMemManager& phymem,
                                         PhyMemState* saved_state) {
        PhyMemMap* allocated_block = saved_state->free_highmem->next_mapitem->next_mapitem;
        allocated_block->clear_owners();
        allocated_block->next_buddy = saved_state->free_highmem->next_buddy;
        saved_state->free_highmem->next_buddy = allocated_block;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyfree_contig_low(PhyMemManager& phymem,
                                             PhyMemState* saved_state) {
        PhyMemMap* allocated_block = saved_state->free_lowmem->next_mapitem->next_mapitem;
        allocated_block->clear_owners();
        allocated_block->next_buddy = saved_state->free_lowmem->next_buddy;
        saved_state->free_lowmem->next_buddy = allocated_block;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyfree_noncontig(PhyMemManager& phymem,
                                            PhyMemState* saved_state,
                                            PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_highmmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        PhyMemMap* chunk_beginning = map_parser;
        map_parser->clear_owners();
        map_parser = map_parser->next_buddy;
        map_parser->clear_owners();
        map_parser->next_buddy = saved_state->free_highmem;
        saved_state->free_highmem = chunk_beginning;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_rockyfree_noncontig_low(PhyMemManager& phymem,
                                                PhyMemState* saved_state,
                                                PhyMemMap* returned_chunk) {
        PhyMemMap* map_parser = saved_state->phy_mmap;
        while(map_parser->location != returned_chunk->location) {
            map_parser = map_parser->next_mapitem;
        }
        PhyMemMap* chunk_beginning = map_parser;
        map_parser->clear_owners();
        map_parser = map_parser->next_buddy;
        map_parser->clear_owners();
        map_parser->next_buddy = saved_state->free_lowmem;
        saved_state->free_lowmem = chunk_beginning;
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_split_alloc(PhyMemManager& phymem,
                                    PhyMemState* saved_state) {        
        static PhyMemMap buffer;
        buffer = *(saved_state->free_highmem);
        buffer.location+= 2*PG_SIZE;
        buffer.size-= 2*PG_SIZE;
        saved_state->free_highmem->size = 2*PG_SIZE;
        saved_state->free_highmem->add_owner(PID_KERNEL);
        saved_state->free_highmem->next_buddy = NULL;
        saved_state->free_highmem->next_mapitem = &buffer;
        saved_state->free_highmem = &buffer;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) - 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }
    
    bool check_phystate_split_alloc_low(PhyMemManager& phymem,
                                        PhyMemState* saved_state) {        
        static PhyMemMap buffer;
        buffer = *(saved_state->free_lowmem);
        buffer.location+= 2*PG_SIZE;
        buffer.size-= 2*PG_SIZE;
        saved_state->free_lowmem->size = 2*PG_SIZE;
        saved_state->free_lowmem->add_owner(PID_KERNEL);
        saved_state->free_lowmem->next_buddy = NULL;
        saved_state->free_lowmem->next_mapitem = &buffer;
        saved_state->free_lowmem = &buffer;
        saved_state->free_mapitems = (PhyMemMap*) (((uint64_t) saved_state->free_mapitems) - 1);
        if(!cmp_phymem_state(phymem, saved_state)) return false;

        return true;
    }

    PhyMemMap* find_twopg_freemem(PhyMemManager& phymem) {
        PhyMemState* phymem_state = (PhyMemState*) &phymem;

        //We have to make room for a third allocated page, in case PhyMemManager has to
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
        size_t final_location;
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
        size_t free_mapitems_location = (size_t) (phymem_state->free_mapitems);
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
}