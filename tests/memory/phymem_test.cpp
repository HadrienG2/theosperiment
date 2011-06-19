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
        bool free_item_overlap = false;
        KernelMemoryMap* kmmap = kinfo.kmmap;
        PhyMemMap should_be;
        while(map_parser) {
            //We just overflowed kmmap : something's wrong
            if(index >= kinfo.kmmap_size) {
                test_failure("Mapping has failed");
                return NULL;
            }

            //Define what each map item should be according to kmmap
            uint64_t mapitem_start = 0, mapitem_end = 0;
            if(kmmap[index].nature == NATURE_FRE) {
                mapitem_start = align_pgup(kmmap[index].location);
                mapitem_end = align_pgdown(kmmap[index].location+kmmap[index].size);
            } else {
                mapitem_start = align_pgdown(kmmap[index].location);
                mapitem_end = align_pgup(kmmap[index].location+kmmap[index].size);
            }
            while(mapitem_start >= mapitem_end) {
                ++index;
                if(kmmap[index].nature == NATURE_FRE) {
                    mapitem_start = align_pgup(kmmap[index].location);
                    mapitem_end = align_pgdown(kmmap[index].location+kmmap[index].size);
                } else {
                    mapitem_start = align_pgdown(kmmap[index].location);
                    mapitem_end = align_pgup(kmmap[index].location+kmmap[index].size);
                }
                mapitem_start = align_pgup(kmmap[index].location);
                mapitem_end = align_pgup(kmmap[index].location+kmmap[index].size);
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
                if(kmmap[index].location + kmmap[index].size > mapitem_end) {
                    if(!free_item_overlap) {
                        free_item_overlap = true;
                        break;
                    } else {
                        free_item_overlap = false;
                    }
                }
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
        subtest_title("Contiguous memory allocation and liberation");
        if(!phy_test_contigalloc(phymem)) return false;
        
        subtest_title("Contiguous memory allocation with free item splitting");
        if(!phy_test_splitalloc(phymem)) return false;
        
        subtest_title("Contiguous memory allocation in non-contiguous free memory");
        if(!phy_test_rockyalloc_contig(phymem)) return false;
        
        subtest_title("Non-contiguous memory allocation");
        if(!phy_test_noncontigalloc(phymem)) return false;
        
        subtest_title("Sharing functions");
        if(!phy_test_sharing(phymem)) return false;
        
        subtest_title("Allocation of a reserved chunk");
        if(!phy_test_resvalloc(phymem)) return false;
        
        subtest_title("Finding a chunk");
        if(!phy_test_find(phymem)) return false;
        
        subtest_title("kill() test");
        if(!phy_test_kill(phymem)) return false;
        
        subtest_title("Internal allocator");
        if(!phy_test_internal_alloc(phymem)) return false;
        
        subtest_title("Arch-specific tests");
        if(!phy_test_arch(phymem)) return false;

        return true;
    }
    
    bool phy_test_contigalloc(PhyMemManager& phymem) {
        item_title("Put PhyMemManager in a state with two consecutive free pages, save it");
        PhyMemState* saved_state = phy_setstate_2_free_pgs(phymem);
        if(!saved_state) return false;
        
        item_title("Allocate a contiguous chunk of two pages, check the returned result");
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        
        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_merge_alloc(phymem, saved_state)) return false;
        
        item_title("Use pointer-based free() to free the chunk, check state");
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        
        item_title("Allocate a contiguous chunk of two pages again, check the result");
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        
        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_perfectfit_alloc(phymem, saved_state)) return false;
        
        item_title("Use location-based free() to free the chunk, check state");
        if(!phymem.free(returned_chunk->location)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        
        return true;
    }
    
    bool phy_test_find(PhyMemManager& phymem) {
        item_title("Try to find the first chunk of phy_mmap with find_thischunk()");
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        if(phymem.find_thischunk(map_parser->location) != map_parser) {
            test_failure("find_thischunk() didn't find the first chunk");
            return false;
        }
        
        item_title("Try to find the second chunk of phy_mmap with find_thischunk()");
        map_parser = map_parser->next_mapitem;
        if(phymem.find_thischunk(map_parser->location) != map_parser) {
            test_failure("find_thischunk() didn't find the second chunk");
            return false;
        }
        
        item_title("Try to find the last chunk of phy_mmap with find_thischunk()");
        while(map_parser->next_mapitem) map_parser = map_parser->next_mapitem;
        if(phymem.find_thischunk(map_parser->location) != map_parser) {
            test_failure("find_thischunk() didn't find the last chunk");
            return false;
        }
        
        return true;
    }
    
    bool phy_test_internal_alloc(PhyMemManager& phymem) {
        item_title("Check that sizeof(PhyMemMap) is a power of 2");
        int tmp = sizeof(PhyMemMap);
        while(tmp%2 == 0) tmp = tmp/2;
        if(tmp!=1) {
            test_failure("sizeof(PhyMemMap) is not a power of 2");
            return false;
        }
        
        item_title("Check that all free map items are used before new ones are allocated");
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        int old_free_length = phymem_state->free_mapitems->buddy_length(), new_free_length;
        while(phymem_state->free_mapitems) {
            phymem.alloc_page(PID_KERNEL);
            new_free_length = phymem_state->free_mapitems->buddy_length();
            if(new_free_length > old_free_length) break;
            old_free_length = new_free_length;
        }
        if(phymem_state->free_mapitems) {
            test_failure("New map items were allocated before old ones were all used");
            return false;
        }
        
        item_title("Check the allocation process (number of items, number of pages used)");
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return false;
        }
        PhyMemMap* page_1 = phymem.alloc_page(PID_KERNEL);
        if(!check_phystate_internal_alloc(phymem, saved_state)) return false;
        
        item_title("Check that memory map items are cleaned up when discarded");
        PhyMemMap* page_2 = phymem.alloc_page(PID_KERNEL);
        phymem.free(page_1);
        phymem.free(page_2);
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE);
        PhyMemMap should_be;
        should_be.next_buddy = phymem_state->free_mapitems->next_buddy;
        if(*(phymem_state->free_mapitems) != should_be) {
            test_failure("Map items are not cleaned up when discarded");
            return false;
        }
        phymem.free(returned_chunk);

        return true;
    }
    
    bool phy_test_kill(PhyMemManager& phymem) {
        item_title("Put PhyMemManager in a state with two consecutive free pages, save it");
        PhyMemState* saved_state = phy_setstate_2_free_pgs(phymem);
        if(!saved_state) return false;
        
        item_title("Allocate two pages to a process, then kill the process");
        phymem.alloc_page(42);
        phymem.alloc_page(42);
        phymem.kill(42);
        
        item_title("Check that we went back to the initially saved state");
        if(!cmp_phymem_state(phymem, saved_state)) return false;
        
        item_title("Try to kill the kernel. Check that it fails.");
        phymem.kill(PID_KERNEL);
        if(!cmp_phymem_state(phymem, saved_state)) return false;
        
        return true;
    }
    
    bool phy_test_noncontigalloc(PhyMemManager& phymem) {
        item_title("First three tests should give same results with non-contiguous allocation");
        //Fragmented contiguous free space
        PhyMemState* saved_state = phy_setstate_2_free_pgs(phymem);
        if(!saved_state) return false;
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_merge_alloc(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        //"Perfect fit" situation
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_perfectfit_alloc(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk->location)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        //Free space is too large
        saved_state = phy_setstate_3pg_free_chunk(phymem);
        if(!saved_state) return false;
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        if(!check_phystate_split_alloc(phymem, saved_state)) return false;
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        
        item_title("Last test should return a non-contiguous chunk instead");
        saved_state = phy_setstate_rocky_freemem(phymem);
        if(!saved_state) return false;
        returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, false);
        if(!phy_check_returned_2pgchunk_noncontig(returned_chunk)) return false;
        
        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_rockyalloc_noncontig(phymem, saved_state)) return false;
        
        item_title("Check that the non-contiguous chunk is freed properly");
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_rockyfree_noncontig(phymem, saved_state, returned_chunk)) return false;
        
        return true;;
    }
    
    bool phy_test_resvalloc(PhyMemManager& phymem) {
        item_title("Allocate a page, mark it reserved, free it, save PhyMemManager state");
        PhyMemMap* resv_page = phymem.alloc_page(PID_KERNEL);
        resv_page->allocatable = false;
        phymem.free(resv_page);
        if((resv_page->allocatable) || (resv_page->next_buddy)) {
            test_failure("Pages don't stay reserved when freed");
            return false;
        }
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return false;
        }
        
        item_title("Try to allocate the page with alloc_resvchunk(), check that it works");
        PhyMemMap* returned_chunk = phymem.alloc_resvchunk(resv_page->location, PID_KERNEL);
        if(!phy_check_returned_resvchunk(returned_chunk, resv_page)) return false;
        if(!check_phystate_resvalloc(phymem, saved_state, returned_chunk)) return false;
        
        item_title("Try to allocate it to another PID, check that it fails");
        returned_chunk = phymem.alloc_resvchunk(resv_page->location, 42);
        if(returned_chunk) {
            test_failure("Successive allocations are supposed to fail");
            return false;
        }
        if(!cmp_phymem_state(phymem, saved_state)) return false;
        
        item_title("Check that owneradd() and ownerdel() do work");
        if(!phymem.owneradd(resv_page, 42)) {
            test_failure("owneradd() returned false");
            return false;
        }
        if(resv_page->owners[1] != 42) {
            test_failure("owneradd() claimed to work, but didn't");
            return false;
        }
        if(!phymem.ownerdel(resv_page, 42)) {
            test_failure("ownerdel() returned false");
            return false;
        }
        if(resv_page->owners[1] != PID_NOBODY) {
            test_failure("ownerdel() claimed to work, but didn't");
            return false;
        }
        
        item_title("Free the page, check that it went back to its initial state");
        if(!phymem.free(resv_page)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_resvfree(phymem, saved_state, resv_page)) return false;

        return true;
    }
    
    bool phy_test_rockyalloc_contig(PhyMemManager& phymem) {
        item_title("Put PhyMemManager in a F-FF state (F=free page, -=busy page) and save");
        PhyMemState* saved_state = phy_setstate_rocky_freemem(phymem);
        if(!saved_state) return false;
        
        item_title("Allocate a contiguous chunk of two pages, check the result");
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        
        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_rockyalloc_contig(phymem, saved_state)) return false;
        
        item_title("Use pointer-based free() to free the chunk, check state");
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_rockyfree_contig(phymem, saved_state)) return false;
        
        return true;
    }
    
    bool phy_test_sharing(PhyMemManager& phymem) {
        item_title("Allocate a page, save PhyMemManager's state");
        PhyMemMap* shared_page = phymem.alloc_page(PID_KERNEL);
        if(!phy_check_returned_page(shared_page)) return false;
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return false;
        }
        
        item_title("Check that adding an owner works properly");
        if(!phymem.owneradd(shared_page, 42)) {
            test_failure("owneradd() returned false");
            return false;
        }
        if(!check_phystate_owneradd(phymem, saved_state, shared_page)) return false;
        
        item_title("Free the page, check that all owners are now gone");
        if(!phymem.free(shared_page)) {
            test_failure("Page was not freed properly");
            return false;
        }
        if((shared_page->owners[0] != PID_NOBODY) || (shared_page->owners[1] != PID_NOBODY)) {
            test_failure("Page was not freed properly");
            return false;
        }
        
        item_title("Allocate the page again, save PhyMemManager's state");
        shared_page = phymem.alloc_page(PID_KERNEL);
        if(!phy_check_returned_page(shared_page)) return false;
        saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return false;
        }
        
        item_title("Remove the page's sole owner, check that it is freed");
        if(!phymem.ownerdel(shared_page, PID_KERNEL)) {
            test_failure("ownerdel() returned false");
            return false;
        }
        if(!check_phystate_ownerdel(phymem, saved_state, shared_page)) return false;
        
        return true;
    }
    
    bool phy_test_splitalloc(PhyMemManager& phymem) {
        item_title("Put PhyMemManager in a state with a 3 pages long free chunk, save it");
        PhyMemState* saved_state = phy_setstate_3pg_free_chunk(phymem);
        if(!saved_state) return false;
        
        item_title("Allocate a contiguous chunk of two pages, check the returned result");
        PhyMemMap* returned_chunk = phymem.alloc_chunk(PID_KERNEL, 2*PG_SIZE, true);
        if(!phy_check_returned_2pgchunk_contig(returned_chunk)) return false;
        
        item_title("Check modifications to PhyMemManager's state");
        if(!check_phystate_split_alloc(phymem, saved_state)) return false;
        
        item_title("Use pointer-based free() to free the chunk, check state");
        if(!phymem.free(returned_chunk)) {
            test_failure("The free operation has returned false");
            return false;
        }
        if(!check_phystate_chunk_free(phymem, saved_state, returned_chunk)) return false;
        
        return true;
    }
    
    bool phy_check_returned_2pgchunk_contig(PhyMemMap* returned_chunk) {
        if(!returned_chunk) {
            test_failure("The pointer must be nonzero");
            return false;
        }
        if(align_pgdown(returned_chunk->location) != returned_chunk->location) {
            test_failure("location must be page-aligned");
            return false;
        }
        if(returned_chunk->size != 2*PG_SIZE) {
            test_failure("size must be 2*PG_SIZE");
            return false;
        }
        if((returned_chunk->owners[0] != PID_KERNEL) || (returned_chunk->owners[1] != PID_NOBODY)) {
            test_failure("owners must only include PID_KERNEL");
            return false;
        }
        if(returned_chunk->allocatable == false) {
            test_failure("allocatable must be true");
            return false;
        }
        if(returned_chunk->next_buddy != NULL) {
            test_failure("next_buddy must be NULL");
            return false;
        }
        if(returned_chunk->next_mapitem == NULL) {
            test_failure("next_mapitem must be nonzero");
            return false;
        }
        
        return true;
    }
    
    bool phy_check_returned_2pgchunk_noncontig(PhyMemMap* returned_chunk) {
        //Checking the chunk
        if(!returned_chunk) {
            test_failure("The pointer must be nonzero");
            return false;
        }
        if(align_pgdown(returned_chunk->location) != returned_chunk->location) {
            test_failure("location must be page-aligned");
            return false;
        }
        if(returned_chunk->size != PG_SIZE) {
            test_failure("size must be PG_SIZE");
            return false;
        }
        if((returned_chunk->owners[0] != PID_KERNEL) || (returned_chunk->owners[1] != PID_NOBODY)) {
            test_failure("owners must only include PID_KERNEL");
            return false;
        }
        if(returned_chunk->allocatable == false) {
            test_failure("allocatable must be true");
            return false;
        }
        if(returned_chunk->next_buddy == NULL) {
            test_failure("next_buddy must be nonzero");
            return false;
        }        
        if(returned_chunk->next_mapitem == NULL) {
            test_failure("next_mapitem must be nonzero");
            return false;
        }
        
        //Checking its buddy
        PhyMemMap* buddy = returned_chunk->next_buddy;
        if(align_pgdown(buddy->location) != buddy->location) {
            test_failure("Buddy's location must be page-aligned");
            return false;
        }
        if(buddy->size != PG_SIZE) {
            test_failure("Buddy's size must be PG_SIZE");
            return false;
        }
        if((buddy->owners[0] != PID_KERNEL) || (buddy->owners[1] != PID_NOBODY)) {
            test_failure("Buddy's owners must only include PID_KERNEL");
            return false;
        }
        if(buddy->allocatable == false) {
            test_failure("Buddy's allocatable must be true");
            return false;
        }
        if(buddy->next_buddy != NULL) {
            test_failure("Buddy's next_buddy must be NULL");
            return false;
        }
        if(buddy->next_mapitem == NULL) {
            test_failure("Buddy's next_mapitem must be nonzero");
            return false;
        }
        
        return true;
    }
    
    bool phy_check_returned_page(PhyMemMap* returned_chunk) {
        if(!returned_chunk) {
            test_failure("The pointer must be nonzero");
            return false;
        }
        if(align_pgdown(returned_chunk->location) != returned_chunk->location) {
            test_failure("location must be page-aligned");
            return false;
        }
        if(returned_chunk->size != PG_SIZE) {
            test_failure("size must be PG_SIZE");
            return false;
        }
        if((returned_chunk->owners[0] != PID_KERNEL) || (returned_chunk->owners[1] != PID_NOBODY)) {
            test_failure("owners must only include PID_KERNEL");
            return false;
        }
        if(returned_chunk->allocatable == false) {
            test_failure("allocatable must be true");
            return false;
        }
        if(returned_chunk->next_buddy != NULL) {
            test_failure("next_buddy must be NULL");
            return false;
        }
        if(returned_chunk->next_mapitem == NULL) {
            test_failure("next_mapitem must be nonzero");
            return false;
        }
        
        return true;
    }
    
    bool phy_check_returned_resvchunk(PhyMemMap* returned_chunk,
                                      PhyMemMap* resv_page) {
        if(returned_chunk != resv_page) {
            test_failure("Returned pointer must target the reserved page");
            return false;
        }
        if(returned_chunk->location != resv_page->location) {
            test_failure("location mismatches");
            return false;
        }
        if(returned_chunk->size != PG_SIZE) {
            test_failure("size must be PG_SIZE");
            return false;
        }
        if((returned_chunk->owners[0] != PID_KERNEL) || (returned_chunk->owners[1] != PID_NOBODY)) {
            test_failure("owners must only include PID_KERNEL");
            return false;
        }
        if(returned_chunk->allocatable) {
            test_failure("allocatable must be false");
            return false;
        }
        if(returned_chunk->next_buddy != NULL) {
            test_failure("next_buddy must be NULL");
            return false;
        }
        if(returned_chunk->next_mapitem == NULL) {
            test_failure("next_mapitem must be nonzero");
            return false;
        }
        
        return true;
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
    
    PhyMemState* phy_setstate_2_free_pgs(PhyMemManager& phymem) {
        PhyMemMap* last_pages[2];
        
        //Allocate pages until we have two consecutive ones, if possible.
        last_pages[0] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't two free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't two free pages in there");
            return NULL;
        }
        while(last_pages[0]->location + last_pages[0]->size != last_pages[1]->location) {
            last_pages[0] = last_pages[1];
            last_pages[1] = phymem.alloc_page(PID_KERNEL);
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
    
    PhyMemState* phy_setstate_3pg_free_chunk(PhyMemManager& phymem) {
        PhyMemMap* last_pages[3];
        
        //Allocate pages until we have three consecutive ones, if possible.
        last_pages[0] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        last_pages[2] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[2] == NULL) {
            test_failure("There aren't three free pages in there");
            return NULL;
        }
        while((last_pages[0]->location + last_pages[0]->size != last_pages[1]->location) || 
          (last_pages[1]->location + last_pages[1]->size != last_pages[2]->location)) {
            last_pages[0] = last_pages[1];
            last_pages[1] = last_pages[2];
            last_pages[2] = phymem.alloc_page(PID_KERNEL);
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
        PhyMemMap* chunk = phymem.alloc_chunk(PID_KERNEL, 3*PG_SIZE, true);
        phymem.free(chunk);
        
        //Save phymem's state, return the result
        PhyMemState* saved_state = save_phymem_state(phymem);
        if(!saved_state) {
            test_failure("Could not save PhyMemManager's state");
            return NULL;
        }
        return saved_state;
    }
    
    PhyMemState* phy_setstate_rocky_freemem(PhyMemManager& phymem) {
        PhyMemMap* last_pages[4];
        
        //Allocate pages until we have three consecutive ones, if possible.
        last_pages[0] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[0] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[1] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[1] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[2] = phymem.alloc_page(PID_KERNEL);
        if(last_pages[2] == NULL) {
            test_failure("There aren't four free pages in there");
            return NULL;
        }
        last_pages[3] = phymem.alloc_page(PID_KERNEL);
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
            last_pages[3] = phymem.alloc_page(PID_KERNEL);
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
}