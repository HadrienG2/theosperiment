 /* Physical memory management testing routines

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
#include <memory_test.h>
#include <phymem_test.h>
#include <phymem_test_arch.h>
#include <test_display.h>

namespace Tests {
    bool meta_phymem() {
        item_title("Check PhyMemManager version");
        if(PHYMEM_TEST_VERSION != PHYMEMMANAGER_VERSION) {
            test_failure("Test and PhyMemManager versions mismatch");
            return false;
        }
        return true;
    }

    PhyMemManager* init_phymem(const KernelInformation& kinfo) {
        item_title("Initialize a PhyMemManager object");
        static PhyMemManager phymem(kinfo);

        item_title("Check availability of mmap_mutex");
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        if(phymem_state->mmap_mutex.state() == 0) {
            test_failure("mmap_mutex not available in a freshly initialized PhyMemManager");
            return NULL;
        }

        item_title("Check that the assumptions about phy_mmap are respected");
        if(!phy_init_check_mmap_assump(phymem_state)) return NULL;

        item_title("Check that kmmap is properly mapped in phy_mmap");
        PhyMemMap storage_space;
        PhyMemMap mapitems_space;
        PhyMemMap* stored_data = NULL;
        PhyMemMap* stored_mapitems = NULL;
        PhyMemMap* after_storage = NULL;
        PhyMemMap* after_mapitems = NULL;
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        unsigned int index = 0;
        KernelMemoryMap* kmmap = kinfo.kmmap;
        PhyMemMap should_be;
        while(map_parser) {
            //We just overflowed kmmap : something's wrong
            if(index >= kinfo.kmmap_size) {
                test_failure("Mapping has failed");
                return NULL;
            }

            //Define what each map item should be according to kmmap
            uint64_t mapitem_start = align_pgup(kmmap[index].location);
            uint64_t mapitem_end = align_pgup(kmmap[index].location + kmmap[index].size);
            while(mapitem_start == mapitem_end) {
                ++index;
                mapitem_start = align_pgup(kmmap[index].location);
                mapitem_end = align_pgup(kmmap[index].location + kmmap[index].size);
            }
            should_be = PhyMemMap();
            should_be.location = mapitem_start;
            should_be.size = mapitem_end-mapitem_start;
            should_be.allocatable = true;
            for(; index < kinfo.kmmap_size; ++index) {
                if(kmmap[index].location >= mapitem_end) break;
                switch(kmmap[index].nature) {
                    case NATURE_RES:
                        should_be.allocatable = false;
                        break;
                    case NATURE_BSK:
                    case NATURE_KNL:
                        should_be.add_owner(PID_KERNEL);
                }
                if(kmmap[index].location + kmmap[index].size > mapitem_end) break;
            }
            if(should_be.allocatable) {
                should_be.next_buddy = map_parser->next_buddy;
            }
            should_be.next_mapitem = map_parser->next_mapitem;

            //Manage two map items which we know will differ from kmmap : memory map storage space..
            if(map_parser->location == (addr_t) phymem_state->phy_mmap) {
                if(stored_data) {
                    //Failure ahead, we've already found where phy_mmap is stored
                    test_failure("A location appears twice in phy_mmap");
                    return NULL;
                }
                if(should_be.allocatable == false) {
                    //PhyMemManager must store its data in allocatable space
                    test_failure("Mapping or storage space allocation has failed");
                    return NULL;
                }
                if(should_be.owners[0] != PID_NOBODY) {
                    //PhyMemManager must store its data in available space
                    test_failure("Mapping or storage space allocation has failed");
                    return NULL;
                }
                storage_space = should_be;
                stored_data = map_parser;
                //Manage remaining space, too, if there's some, otherwise just move to the next item
                if(should_be.size > map_parser->size) {
                    should_be.size-= map_parser->size;
                    should_be.location+= map_parser->size;
                    should_be.next_mapitem = map_parser->next_mapitem->next_mapitem;
                    after_storage = map_parser->next_mapitem;
                    map_parser = map_parser->next_mapitem;
                } else {
                    map_parser = map_parser->next_mapitem;
                    continue;
                }
            }

            //...and extra free map items storage space. In other cases, mapping has failed
            if(*map_parser != should_be) {
                if(stored_mapitems) {
                    test_failure("Only one pack of map items should have been allocated");
                    return NULL;
                }
                if(should_be.allocatable == false) {
                    //It's not that, PhyMemManager must store its data in allocatable space
                    test_failure("Mapping or storage space allocation has failed");
                    return NULL;
                }
                if(should_be.owners[0] != PID_NOBODY) {
                    //It's not that, PhyMemManager must store its data in available space
                    test_failure("Mapping or storage space allocation has failed");
                    return NULL;
                }
                PhyMemMap* free_mapitems_parser = phymem_state->free_mapitems;
                while(free_mapitems_parser) {
                    if(align_pgdown((addr_t) free_mapitems_parser) == should_be.location) break;
                    free_mapitems_parser = free_mapitems_parser->next_buddy;
                }
                if(!free_mapitems_parser) {
                    test_failure("Mapping has failed");
                    return NULL;
                }
                //We've found free map items storage_space
                mapitems_space = should_be;
                stored_mapitems = map_parser;
                if(should_be.size > PG_SIZE) {
                    after_mapitems = after_storage->next_mapitem;
                    map_parser = map_parser->next_mapitem;
                }
            }
            //Move to the next item
            map_parser = map_parser->next_mapitem;
        }
        if(index != kinfo.kmmap_size) {
            //If at this point there are still items of kmmap remaining, not everything has
            //been properly mapped
            test_failure("Mapping has failed");
            return NULL;
        }

        item_title("Check that the storage space of PhyMemManager is properly allocated");
        if(!stored_data) {
            test_failure("Storage space allocation has failed");
            return NULL;
        }
        if(!stored_mapitems) {
            test_failure("Map item allocation has failed");
            return NULL;
        }
        if(storage_space.location != stored_data->location) {
            test_failure("Storage space allocation has failed");
            return NULL;
        }
        if(after_storage && (after_storage != stored_mapitems)) {
            if(after_storage->location != stored_data->location + stored_data->size) {
                test_failure("Storage space allocation has failed");
                return NULL;
            }
            if((after_storage != stored_mapitems) &&
              (storage_space.size != stored_data->size + after_storage->size)) {
                test_failure("Storage space allocation has failed");
                return NULL;
            }
        }
        if(after_mapitems) {
            if(after_mapitems->location != stored_mapitems->location + stored_mapitems->size) {
                test_failure("Map item allocation has failed");
                return NULL;
            }
            if(mapitems_space.size != stored_mapitems->size + after_mapitems->size) {
                test_failure("Map item allocation has failed");
                return NULL;
            }
        }

        item_title("Check that all map items are allocated before use, without leaks");
        //Manage map items allocated in storage space
        map_parser = phymem_state->phy_mmap;
        PhyMemMap* should_be_ptr = (PhyMemMap*) (stored_data->location);
        PhyMemMap* new_mapitem = NULL;
        while(map_parser) {
            if(map_parser != should_be_ptr) {
                if(!new_mapitem) {
                    new_mapitem = map_parser;
                    map_parser = map_parser->next_mapitem;
                    continue;
                } else {
                    test_failure("Map item allocation failed");
                    return NULL;
                }
            }
            map_parser = map_parser->next_mapitem;
            ++should_be_ptr;
        }
        if((addr_t) should_be_ptr >= stored_data->location + stored_data->size) {
            test_failure("Storage space overflow");
            return NULL;
        }
        map_parser = phymem_state->free_mapitems;
        while(map_parser) {
            if(map_parser != should_be_ptr) {
                if(map_parser == new_mapitem+1) break;
                test_failure("Map item allocation failed");
                return NULL;
            }
            map_parser = map_parser->next_buddy;
            ++should_be_ptr;
        }
        if((addr_t) should_be_ptr < stored_data->location + stored_data->size) {
            test_failure("Allocated map items leaked");
            return NULL;
        }
        if((addr_t) should_be_ptr > stored_data->location + stored_data->size) {
            test_failure("Storage space overflow");
            return NULL;
        }
        //Manage free mapitems allocated at the end of PhyMemManager()
        if((addr_t) new_mapitem != stored_mapitems->location) {
            test_failure("Map item allocation failed");
            return NULL;
        }
        should_be_ptr = new_mapitem+1;
        while(map_parser) {
            if(map_parser != should_be_ptr) {
                test_failure("Map item allocation failed");
                return NULL;
            }
            map_parser = map_parser->next_buddy;
            ++should_be_ptr;
        }
        if((addr_t) should_be_ptr < stored_mapitems->location + stored_mapitems->size) {
            test_failure("Allocated map items leaked");
            return NULL;
        }
        if((addr_t) should_be_ptr > stored_mapitems->location + stored_mapitems->size) {
            test_failure("Storage space overflow");
            return NULL;
        }

        item_title("Check that free_mapitems is initialized properly");
        should_be = PhyMemMap();
        map_parser = phymem_state->free_mapitems;
        while(map_parser) {
            if(map_parser->next_buddy) {
                should_be.next_buddy = map_parser->next_buddy;
            } else {
                should_be.next_buddy = NULL;
            }
            if(should_be != *map_parser) {
                test_failure("free_mapitems is not initialized properly");
                return NULL;
            }
            map_parser = map_parser->next_buddy;
        }

        if(!phy_init_arch(phymem)) return NULL;

        return &phymem;
    }

    bool test_phymem(PhyMemManager& phymem) {
        reset_sub_title();
        subtest_title("Page allocation");
        PhyMemMap* allocd_page = phy_test_pagealloc(phymem);
        if(!allocd_page) return false;

        subtest_title("Page freeing");
        if(phy_test_pagefree(phymem, allocd_page) == false) return false;

        subtest_title("Contiguous chunk allocation");
        if(!phy_test_contigchunkalloc(phymem)) return false;

        subtest_title("Allocation in non-contiguous memory");
        fail_notimpl();
        return false;
    }

    PhyMemMap* phy_test_pagealloc(PhyMemManager& phymem) {
        item_title("Save PhyMemManager state");
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) return NULL;

        item_title("Allocate a page to some PID using alloc_page()");
        PhyMemMap* allocd_page = phymem.alloc_page(PID_KERNEL);

        item_title("Check that the returned result is correct");
        if(!phy_check_returned_page(allocd_page)) return NULL;

        item_title("Check modifications to the PhyMemManager's state");
        if(!check_phystate_pagealloc(phymem, saved_state, allocd_page)) return NULL;
        discard_phymem_state(saved_state);

        return allocd_page;
    }

    bool phy_test_pagefree(PhyMemManager& phymem, PhyMemMap* allocd_page) {
        item_title("Save PhyMemManager state");
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) return false;

        item_title("Free the previously allocated page");
        if(phymem.free(allocd_page) == false) {
            test_failure("The page was not freed properly");
            return false;
        }

        item_title("Check modifications to the PhyMemManager's state");
        if(!check_phystate_chunkfree(phymem, saved_state, allocd_page)) return false;
        discard_phymem_state(saved_state);

        item_title("Start over from 1. with the location-based variant of free()");
        //Allocation...
        saved_state = save_phymem_state(phymem);
        if(!saved_state) return false;
        PhyMemMap* new_allocd_page = phymem.alloc_page(PID_KERNEL);
        if(!phy_check_returned_page(new_allocd_page)) return false;
        if(!check_phystate_pagealloc(phymem, saved_state, new_allocd_page)) return false;
        //Liberation... (saved_state is now a copy of phymem's state)
        if(phymem.free(new_allocd_page->location) == false) {
            test_failure("The page was not freed properly");
            return false;
        }
        if(!check_phystate_chunkfree(phymem, saved_state, new_allocd_page)) return false;
        discard_phymem_state(saved_state);

        item_title("Start over with alloc_chunk and PG_SIZE size");
        //Allocation...
        saved_state = save_phymem_state(phymem);
        if(!saved_state) return false;
        new_allocd_page = phymem.alloc_chunk(PID_KERNEL, PG_SIZE);
        if(!phy_check_returned_page(new_allocd_page)) return false;
        if(!check_phystate_pagealloc(phymem, saved_state, new_allocd_page)) return false;
        discard_phymem_state(saved_state);
        //Liberation is guaranteed to work if allocation has had the same result as before, don't
        //bother testing.
        phymem.free(new_allocd_page);

        item_title("Same as above, with contig flag on");
        saved_state = save_phymem_state(phymem);
        if(!saved_state) return false;
        new_allocd_page = phymem.alloc_chunk(PID_KERNEL, PG_SIZE, true);
        if(!phy_check_returned_page(new_allocd_page)) return false;
        if(!check_phystate_pagealloc(phymem, saved_state, new_allocd_page)) return false;
        discard_phymem_state(saved_state);
        phymem.free(new_allocd_page);

        return true;
    }

    bool phy_test_contigchunkalloc(PhyMemManager& phymem) {
        item_title("Allocate pages until we have two contiguous pages");
        PhyMemMap* contig_pages[3] = {NULL, NULL, NULL};
        while((!(contig_pages[0])) || (!(contig_pages[1])) ||
          (contig_pages[0]->location+PG_SIZE != contig_pages[1]->location)) {
            contig_pages[0] = contig_pages[1]; //This may leak a few pages of memory.
                                               //However, we don't care in this testing code.
            contig_pages[1] = phymem.alloc_page(PID_KERNEL);
            if(contig_pages[1] == NULL) {
                test_failure("Page allocation failed");
                return false;
            }
        }

        item_title("Save PhyMemManager's state");
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) return false;

        item_title("Free these two pages, the first one first then the second one");
        if(phymem.free(contig_pages[0]) == false) {
            test_failure("The first page was not freed properly");
            return false;
        }
        if(phymem.free(contig_pages[1]) == false) {
            test_failure("The second page was not freed properly");
            return false;
        }

        item_title("Check modifications to the PhyMemManager's state");
        if(!check_phystate_2contigpagesfree(phymem, saved_state, contig_pages[0])) return false;

        item_title("Allocate a chunk of two pages using alloc_chunk()");
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE);

        item_title("Check that the returned result is correct");
        if(!phy_check_returned_2pgcontigchunk(returned_chunk)) return false;

        item_title("Check modifications to the PhyMemManager's state");
        if(!check_phystate_2pgcontigchunk_alloc(phymem, saved_state, returned_chunk)) return false;

        item_title("Free the chunk");
        if(phymem.free(returned_chunk) == false) {
            test_failure("The chunk was not freed properly");
            return false;
        }

        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_chunkfree(phymem, saved_state, returned_chunk)) return false;

        item_title("Allocate the chunk of two pages again");
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE);

        item_title("Check that the returned result is the same as before");
        if(!phy_check_returned_2pgcontigchunk(returned_chunk)) return false;

        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_2pgcontigchunk_realloc(phymem, saved_state, returned_chunk)) return false;
        discard_phymem_state(saved_state);

        item_title("Free the chunk");
        phymem.free(returned_chunk);

        item_title("Allocate pages until we have three contiguous pages, free them");
        while((!(contig_pages[0])) || (!(contig_pages[1])) || (!(contig_pages[2])) ||
          (contig_pages[0]->location+PG_SIZE != contig_pages[1]->location) ||
          (contig_pages[1]->location+PG_SIZE != contig_pages[2]->location)) {
            contig_pages[0] = contig_pages[1]; //This may leak a few pages of memory.
            contig_pages[1] = contig_pages[2]; //However, we don't care in this testing code.
            contig_pages[2] = phymem.alloc_page(PID_KERNEL);
            if(contig_pages[2] == NULL) {
                test_failure("Page allocation failed");
                return false;
            }
        }
        phymem.free(contig_pages[0]);
        phymem.free(contig_pages[1]);
        phymem.free(contig_pages[2]);

        item_title("Allocate a 3-page chunk and free it to make this storage space contiguous");
        phymem.free(phymem.alloc_chunk(PID_KERNEL, 3*PG_SIZE));

        item_title("Save PhyMemManager state");
        saved_state = save_phymem_state(phymem);
            phymem.print_highmmap();
        if(!saved_state) return false;

        item_title("Allocate a 2-page chunk in this contiguous space");
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE);

        item_title("Check that the returned result is the same as before");
        if(!phy_check_returned_2pgcontigchunk(returned_chunk)) return false;

        item_title("Check modifications to the PhyMemManager's state");
        if(!check_phystate_2pg_in_3pg(phymem, saved_state, returned_chunk)) return false;
        discard_phymem_state(saved_state);

        item_title("Repeat these tests with alloc_contigchunk()");
        fail_notimpl();
        return false;
    }

    bool phy_init_check_mmap_assump(PhyMemState* phymem_state) {
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        while(map_parser) {
            if(map_parser->location%PG_SIZE) {
                test_failure("Map items aren't always on a page-aligned location");
                return false;
            }
            if(map_parser->size%PG_SIZE) {
                test_failure("Map items don't always have a page-aligned size");
                return false;
            }
            if(map_parser->next_mapitem) {
                if(map_parser->location > map_parser->next_mapitem->location) {
                    test_failure("Map items are not sorted");
                    return false;
                }
                if(map_parser->location+map_parser->size > map_parser->next_mapitem->location) {
                    test_failure("Map items overlap");
                    return false;
                }
            }
            map_parser = map_parser->next_mapitem;
        }

        return true;
    }

    bool phy_check_returned_page(PhyMemMap* returned_page) {
        if(!returned_page) {
            test_failure("The returned pointer is NULL");
            return false;
        }
        if((returned_page->location) % PG_SIZE) {
            test_failure("The returned chunk's location is not page-aligned");
            return false;
        }
        if(returned_page->size != PG_SIZE) {
            test_failure("The returned chunk is not a page");
            return false;
        }
        if(returned_page->has_owner(PID_KERNEL) == false) {
            test_failure("The returned chunk's owner does not include the specified PID");
            return false;
        }
        if(returned_page->owners[1] != PID_NOBODY) {
            test_failure("The returned chunk has more than one owner");
            return false;
        }
        if(returned_page->allocatable == false) {
            test_failure("The returned chunk is not allocatable");
            return false;
        }
        if(returned_page->next_buddy) {
            test_failure("The returned chunk has buddies");
            return false;
        }
        if(returned_page->next_mapitem == NULL) {
            test_failure("The returned chunk has not been inserted properly in the memory map");
            return false;
        }

        return true;
    }

    bool phy_check_returned_2pgcontigchunk(PhyMemMap* returned_chunk) {
        if(!returned_chunk) {
            test_failure("The returned pointer is NULL");
            return false;
        }
        if((returned_chunk->location) % PG_SIZE) {
            test_failure("The returned chunk's location is not page-aligned");
            return false;
        }
        if(returned_chunk->size != 2*PG_SIZE) {
            test_failure("The returned chunk isn't two contiguous pages long");
            return false;
        }
        if(returned_chunk->has_owner(PID_KERNEL) == false) {
            test_failure("The returned chunk's owner does not include the specified PID");
            return false;
        }
        if(returned_chunk->owners[1] != PID_NOBODY) {
            test_failure("The returned chunk has more than one owner");
            return false;
        }
        if(returned_chunk->allocatable == false) {
            test_failure("The returned chunk is not allocatable");
            return false;
        }
        if(returned_chunk->next_buddy) {
            test_failure("The returned chunk has buddies");
            return false;
        }
        if(returned_chunk->next_mapitem == NULL) {
            test_failure("The returned chunk has not been inserted properly in the memory map");
            return false;
        }

        return true;
    }
}