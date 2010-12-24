 /* Physical memory management testing routines

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
#include <memory_test.h>
#include <phymem_test.h>
#include <phymem_test_arch.h>

namespace MemTest {
    PhyMemManager* test_phymem(const KernelInformation& kinfo) {
        reset_sub_title();
        subtest_title("Meta (testing the test itself)");
        if(!phy_test1_meta()) return NULL;
        
        subtest_title("Initial state");
        PhyMemManager* phymem_ptr = phy_test2_init(kinfo);
        if(!phymem_ptr) return NULL;
        PhyMemManager& phymem = *phymem_ptr;
        
        subtest_title("Page allocation");
        if(!phy_test3_pagealloc(phymem)) return NULL;
        
        return &phymem;
    }
    
    bool phy_test1_meta() {
        item_title("Check PhyMemManager version");
        if(PHYMEM_TEST_VERSION != PHYMEMMANAGER_VERSION) {
            test_failure("Test and PhyMemManager versions mismatch");
            return false;
        }
        return true;
    }
    
    PhyMemManager* phy_test2_init(const KernelInformation& kinfo) {
        item_title("Initialize PhyMemManager");
        static PhyMemManager phymem(kinfo);
        
        item_title("Check availability of mmap_mutex");
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        if(!(phymem_state->mmap_mutex.state())) {
            test_failure("mmap_mutex not available in a freshly initialized PhyMemManager");
            return NULL;
        }
        
        item_title("Check that the assumptions about phy_mmap are respected");
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        while(map_parser) {
            if(map_parser->location%PG_SIZE) {
                test_failure("Map items aren't always on a page-aligned location");
                return NULL;
            }
            if(map_parser->size%PG_SIZE) {
                test_failure("Map items don't always have a page-aligned size");
                return NULL;
            }
            if(map_parser->next_mapitem) {
                if(map_parser->location > map_parser->next_mapitem->location) {
                    test_failure("Map items are not sorted");
                    return NULL;
                }
                if(map_parser->location+map_parser->size > map_parser->next_mapitem->location) {
                    test_failure("Map items overlap");
                    return NULL;
                }
            }
            map_parser = map_parser->next_mapitem;
        }
        
        item_title("Check that kmmap is properly mapped in phy_mmap");
        PhyMemMap storage_space;
        PhyMemMap* stored_data = NULL;
        PhyMemMap* after_storage = NULL;
        map_parser = phymem_state->phy_mmap;
        unsigned int index = 0;
        KernelMemoryMap* kmmap = kinfo.kmmap;
        PhyMemMap should_be;
        while(map_parser) {
            if(index >= kinfo.kmmap_size) {
                test_failure("Mapping has failed");
                return NULL;
            }
            //Define what each map item should be
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
            should_be.next_mapitem = map_parser->next_mapitem;
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
            //Check that it is indeed that
            if(*map_parser != should_be) {
                //If not, one possibility is that it is the place where PhyMemManager stores its
                //data. Check that it was a suitable place for allocating it.
                if(stored_data) {
                    //It's not that, we've already found a possible storage location
                    test_failure("Mapping has failed");
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
                storage_space = should_be;
                stored_data = map_parser;
                if(should_be.size > map_parser->size) {
                    //Manage remaining space, too.
                    map_parser = map_parser->next_mapitem;
                    after_storage = map_parser;
                }
            }
            //If it is, everything is all right : move to the next item
            map_parser = map_parser->next_mapitem;
        }
        if(index != kinfo.kmmap_size) {
            //If at this point there are still items of kmmap remaining, not everything has
            //been properly mapped
            test_failure("Mapping has failed");
            return NULL;
        }
        
        item_title("Check that the storage space of PhyMemManager is properly allocated");
        if(storage_space.location != stored_data->location) {
            test_failure("Mapping or storage space allocation has failed");
            return NULL;
        }
        if(after_storage) {
            if(after_storage->location != stored_data->location + stored_data->size) {
                test_failure("Mapping or storage space allocation has failed");
                return NULL;
            }
            if(storage_space.size != stored_data->size + after_storage->size) {
                test_failure("Mapping or storage space allocation has failed");
                return NULL;
            }
        }
        
        item_title("Check that all map items are allocated before use, without leaks");
        map_parser = phymem_state->phy_mmap;
        PhyMemMap* should_be_ptr = (PhyMemMap*) (stored_data->location);
        while(map_parser) {
            if(map_parser != should_be_ptr) {
                test_failure("Map item allocation failed");
                return NULL;
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
                test_failure("Allocated map items leaked");
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
                dbgout << should_be;
                dbgout << *map_parser;
                test_failure("free_mapitems is not initialized properly");
                return NULL;
            }
            map_parser = map_parser->next_buddy;
        }
        
        if(!phy_test2_init_arch(phymem)) return NULL;
        
        return &phymem;
    }
    
    PhyMemManager* phy_test3_pagealloc(PhyMemManager& phymem) {
        test_failure("Not implemented yet");
        return NULL;
    }
}