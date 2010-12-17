 /* A PhyMemManager- and VirMemManager-based memory allocator

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
 
#include <kmem_allocator.h>
#include <align.h>

#include <dbgstream.h>

addr_t MemAllocator::alloc_mapitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    MallocMap* current_item;
    
    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return NULL;
    
    //Fill it with initialized map items
    free_mapitems = (MallocMap*) (allocated_chunk->location);
    current_item = free_mapitems;
    for(used_space = sizeof(MallocMap); used_space < PG_SIZE; used_space+=sizeof(MallocMap)) {
        *current_item = MallocMap();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    *current_item = MallocMap();
    current_item->next_item = NULL;
    
    //All good !
    return allocated_chunk->location;
}

addr_t MemAllocator::alloc_listitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    MallocPIDList* current_item;
    
    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return NULL;
    
    //Fill it with initialized list items
    free_listitems = (MallocPIDList*) (allocated_chunk->location);
    current_item = free_listitems;
    for(used_space = sizeof(MallocPIDList); used_space < PG_SIZE; used_space+=sizeof(MallocPIDList)) {
        *current_item = MallocPIDList();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    *current_item = MallocPIDList();
    current_item->next_item = NULL;
    
    //All good !
    return allocated_chunk->location;
}

addr_t MemAllocator::allocator(addr_t size, MallocPIDList* target) {
    //How it works :
    //  1.Look for a suitable hole in the target's free_map linked list
    //  2.If there is none, allocate a chunk through phymem and map it through virmem, then put
    //    it in free_map. Return NULL if it fails
    //  3.Take the requested space from the chunk found/created in free_map, update free_map and
    //    busy_map
    
    PhyMemMap* phy_chunk = NULL;
    VirMemMap* vir_chunk = NULL;
    
    //Steps 1 : Look for a suitable hole in target->free_map
    MallocMap* hole = NULL;
    if(target->free_map) hole = target->free_map->find_contigchunk(size);
    
    //Step 2 : If there's none, create it
    if(!hole) {
        //Allocating enough memory
        phy_chunk = phymem->alloc_chunk(align_pgup(size), target->map_owner);
        if(!phy_chunk) return NULL;
        vir_chunk = virmem->map(phy_chunk, target->map_owner);
        if(!vir_chunk) {
            phymem->free(phy_chunk);
            return NULL;
        }
        
        //Putting that memory in a MallocMap block
        if(!free_mapitems) {
            alloc_mapitems();
            if(!free_mapitems) {
                virmem->free(vir_chunk);
                phymem->free(phy_chunk);
                return NULL;
            }
        }
        hole = free_mapitems;
        free_mapitems = free_mapitems->next_item;
        hole->next_item = NULL;
        hole->location = vir_chunk->location;
        hole->size = vir_chunk->size;
        hole->belongs_to = vir_chunk;
        
        //Put that block in target->free_map
        if(hole->location < target->free_map->location) {
            hole->next_item = target->free_map;
            target->free_map = hole;
        } else {
            MallocMap* map_parser = target->free_map;
            while(map_parser->next_item) {
                if(map_parser->next_item->location > hole->location) break;
                map_parser = map_parser->next_item;
            }
            hole->next_item = map_parser->next_item;
            map_parser->next_item = hole;
        }
    }
    
    //Step 3 : Taking the requested amount of memory out of our hole
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            if(phy_chunk) phymem->free(phy_chunk);
            if(vir_chunk) virmem->free(vir_chunk);
            return NULL;
        }
    }
    MallocMap* allocated = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = hole->location;
    allocated->size = size;
    allocated->belongs_to = hole->belongs_to;
    hole->size-= size;
    hole->location+= size;
    
    //Putting the newly allocated memory at its place in target->busy_map
    if(allocated->location < target->busy_map->location) {
        allocated->next_item = target->busy_map;
        target->busy_map = allocated;
    } else {
        MallocMap* map_parser = target->busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > allocated->location) break;
            map_parser = map_parser->next_item;
        }
        allocated->next_item = map_parser->next_item;
        map_parser->next_item = allocated;
    }
    
    //Freeing the hole if it's now empty
    if(hole->size == 0) {
        //Remove the hole from the map
        if(hole == target->free_map) {
            target->free_map = hole->next_item;
        } else {
            MallocMap* map_parser = target->free_map;
            while(map_parser->next_item) {
                if(map_parser->next_item == hole) break;
                map_parser = map_parser->next_item;
            }
            //At this stage, map_parser->next_item is hole
            map_parser->next_item = hole->next_item;
        }
        
        //Free it
        *hole = MallocMap();
        hole->next_item = free_mapitems;
        free_mapitems = hole;
    }

    return allocated->location;
}

addr_t MemAllocator::allocator_shareable(addr_t size, MallocPIDList* target) {
    //Same as above, but always allocates a new chunk and does not put extra memory in free_map
    
    //Allocating memory
    PhyMemMap* phy_chunk = phymem->alloc_chunk(align_pgup(size), target->map_owner);
    if(!phy_chunk) return NULL;
    VirMemMap* vir_chunk = virmem->map(phy_chunk, target->map_owner);
    if(!vir_chunk) {
        phymem->free(phy_chunk);
        return NULL;
    }
    
    //Putting that memory in a MallocMap block
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            virmem->free(vir_chunk);
            phymem->free(phy_chunk);
            return NULL;
        }
    }
    MallocMap* allocated = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = vir_chunk->location;
    allocated->size = vir_chunk->size;
    allocated->belongs_to = vir_chunk;
        
    //Putting that block in target->busy_map
    if(allocated->location < target->busy_map->location) {
        allocated->next_item = target->busy_map;
        target->busy_map = allocated;
    } else {
        MallocMap* map_parser = target->busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > allocated->location) break;
            map_parser = map_parser->next_item;
        }
        allocated->next_item = map_parser->next_item;
        map_parser->next_item = allocated;
    }

    return allocated->location;
}

addr_t MemAllocator::liberator(addr_t location, MallocPIDList* target) {
    //How it works :
    //  1.Find the relevant item in busy_map. If it fails, return NULL. Keep its predecessor around
    //    too : that will be useful in step 2.
    //  2.In the way, check if that item is the sole item belonging to its chunk of virtual memory
    //    in busy_map (examining the neighbours should be sufficient, since busy_map is sorted).
    // 3a.If so, liberate the chunk and remove any item of free_map belonging to it. If this results
    //    in busy_map being empty, free_map is necessarily empty too : remove that PID.
    // 3b.If not, move the busy_map item in free_map, merging it with neighbors if possible.
    
    MallocMap *freed_item, *previous_item;

    //Step 1 : Finding the item in busy_map.
    //We can't use MallocMap->find_thischunk() here, since we want the previous item too.
    if(target->busy_map->location == location) {
        freed_item = target->busy_map;
        previous_item = NULL;
    } else {
        previous_item = target->busy_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->location == location) {
                freed_item = previous_item->next_item;
                break;
            }
            previous_item = previous_item->next_item;
        }
        if(!freed_item) return NULL;
    }
    
    //Step 2 : Check if it was the sole busy item in this chunk by looking at its nearest neighbours
    bool item_is_alone = true;
    if(previous_item->belongs_to == freed_item->belongs_to) item_is_alone = false;
    if(freed_item->belongs_to == freed_item->next_item->belongs_to) item_is_alone = false;
    
    if(item_is_alone) {
        //Step 3a : Liberate the chunks and any item of free_map belonging to them.
        //To begin with, take our item out of busy_map.
        if(previous_item) {
            previous_item->next_item = freed_item->next_item;
        } else {
            target->busy_map = freed_item->next_item;
        }
        
        //Then free it and liberate the physical/virtual chunks associated with it if possible.
        //We use phymem->ownerdel instead of phymem->free here, since the process might have shared
        //this data with other processes, in which case freeing it would be a bit brutal for those.
        VirMemMap* belonged_to = freed_item->belongs_to;
        phymem->ownerdel(belonged_to->points_to, belonged_to->owner->map_owner);
        virmem->free(belonged_to);
        *freed_item = MallocMap();
        freed_item->next_item = free_mapitems;
        free_mapitems = freed_item;
        
        //Then parse free_map, looking for a free item which belongs to this chunk, if there's any.
        //Remove it from free_map and free it too.
        freed_item = NULL;
        if(target->free_map->belongs_to == belonged_to) {
            freed_item = target->free_map;
            target->free_map = freed_item->next_item;
            *freed_item = MallocMap();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
        } else {
            previous_item = target->free_map;
            while(previous_item->next_item) {
                if(previous_item->next_item->belongs_to == belonged_to) {
                    freed_item = previous_item->next_item;
                    break;
                }
                previous_item = previous_item->next_item;
            }
            if(freed_item) {
                previous_item->next_item = freed_item->next_item;
                *freed_item = MallocMap();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            }
        }
    } else {
        //Step 3b : Move the busy_map item in free_map, merging it with neighbors if possible
        if(freed_item->location < target->free_map->location) {
            //TODO : DO NOT FORGET TO MERGE !
            freed_item->next_item = target->free_map;
            target->free_map = freed_item;
        } else {
            //TODO : DO NOT FORGET TO MERGE !
            MallocMap* map_parser = target->free_map;
            while(map_parser->next_item) {
                if(map_parser->next_item->location > freed_item->location) break;
                map_parser = map_parser->next_item;
            }
            freed_item->next_item = map_parser->next_item;
            map_parser->next_item = freed_item;
        }
    }
    
    //Stub !
    return NULL;
}

addr_t MemAllocator::sharer(addr_t location, MallocPIDList* source, MallocPIDList* target) {
    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) return NULL;
    }

    //Setup chunk sharing
    MallocMap* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) return NULL;
    PhyMemMap* phy_item = shared_item->belongs_to->points_to;
    PID target_pid = target->map_owner;
    VirMemFlags flags = shared_item->belongs_to->flags;
    VirMemMap* shared_chunk = virmem->map(phy_item, target_pid, flags);
    if(!shared_chunk) return NULL;
    
    //Make the chunks to be inserted in target->busy_map and target->free_map
    MallocMap* busy_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_item->size;
    busy_item->belongs_to = shared_chunk;
    MallocMap* free_item = NULL;
    if(busy_item->size!=shared_chunk->size) {
        free_item = free_mapitems;
        free_mapitems = free_mapitems->next_item;
        free_item->location = busy_item->location+busy_item->size;
        free_item->size = shared_chunk->size-busy_item->size;
        free_item->belongs_to = shared_chunk;
    }
    
    //Insert the busy item in busy_map
    if(busy_item->location < target->busy_map->location) {
        busy_item->next_item = target->busy_map;
        target->busy_map = busy_item;
    } else {
        MallocMap* map_parser = target->busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > busy_item->location) break;
            map_parser = map_parser->next_item;
        }
        busy_item->next_item = map_parser->next_item;
        map_parser->next_item = busy_item;
    }
    if(!free_item) return busy_item->location; //If no free item has been created, it ends here

    //Else, insert the free item in free_map
    if(free_item->location < target->free_map->location) {
        free_item->next_item = target->free_map;
        target->free_map = free_item;
    } else {
        MallocMap* map_parser = target->free_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > free_item->location) break;
            map_parser = map_parser->next_item;
        }
        free_item->next_item = map_parser->next_item;
        map_parser->next_item = free_item;
    }
    return busy_item->location;
}

addr_t MemAllocator::knl_allocator(addr_t size) {
    //This works a lot like allocator(), except that it uses knl_free_map and knl_busy_map and that
    //since paging is disabled for the kernel, the chunk is allocated through
    //PhyMemManager::alloc_contigchunk, and not mapped through virmem.

    //Stub !
    return NULL;
}

addr_t MemAllocator::knl_allocator_shareable(addr_t size) {
    //As above, but always allocate a new chunk and does not mark remaining memory as free
    
    //Stub !
    return NULL;
}

addr_t MemAllocator::knl_liberator(addr_t location) {
    //This works a lot like liberator(), except that it uses knl_free_map and knl_busy_map and that
    //   -Since paging is disabled for the kernel, the chunk is only liberated through phymem
    //   -The kernel's structures are part of MemAllocator, they are never removed.

    //Stub !
    return NULL;
}

addr_t MemAllocator::share_from_knl(addr_t location, MallocPIDList* target) {
    //Like sharer(), but the original chunk belongs to the kernel

    //Stub !
    return NULL;
}

addr_t MemAllocator::share_to_knl(addr_t location, MallocPIDList* source) {
    //Like sharer(), but the kernel is the recipient
    
    //Stub !
    return NULL;
}

MallocPIDList* MemAllocator::find_pid(PID target) {
    MallocPIDList* list_item;
    
    list_item = map_list;
    while(list_item) {
        if(list_item->map_owner == target) break;
        list_item = list_item->next_item;
    }
    
    return list_item;
}

MallocPIDList* MemAllocator::find_or_create_pid(PID target) {
    MallocPIDList* list_item;
    
    if(!map_list) {
        map_list = setup_pid(target);
        return map_list;
    }
    list_item = map_list;
    while(list_item->next_item) {
        if(list_item->map_owner == target) break;
        list_item = list_item->next_item;
    }
    if(list_item->map_owner != target) {
        list_item->next_item = setup_pid(target);
        list_item = list_item->next_item;
    }
    
    return list_item;
}

MallocPIDList* MemAllocator::setup_pid(PID target) {
    MallocPIDList* result;
    
    //Allocate management structures
    if(!free_listitems) {
        alloc_listitems();
        if(!free_listitems) return NULL;
    }
    
    //Fill them
    result = free_listitems;
    free_listitems = free_listitems->next_item;
    result->map_owner = target;
    result->next_item = NULL;
    
    return result;
}

MallocPIDList* MemAllocator::remove_pid(PID target) {
    MallocPIDList *result, *previous_item;
    
    //Remove the PID from the map list
    if(map_list->map_owner == target) {
        result = map_list;
        map_list = map_list->next_item;
    } else {
        previous_item = map_list;
        while(previous_item->next_item) {
            if(previous_item->next_item->map_owner == target) break;
            previous_item = previous_item->next_item;
        }
        result = previous_item->next_item;
        previous_item->next_item = previous_item->next_item->next_item;
        if(!result) return NULL;
    }
    
    //Free its entry
    *result = MallocPIDList();
    result->next_item = free_listitems;
    free_listitems = result;
    
    return result;
}

MemAllocator::MemAllocator(PhyMemManager& physmem, VirMemManager& virtmem) : phymem(&physmem),
                                                                             virmem(&virtmem),
                                                                             map_list(NULL),
                                                                             free_mapitems(NULL),
                                                                             free_listitems(NULL),
                                                                             knl_free_map(NULL),
                                                                             knl_busy_map(NULL) {
    alloc_mapitems();
    alloc_listitems();
}

addr_t MemAllocator::malloc(addr_t size, PID target) {
    MallocPIDList* list_item;
    addr_t result;
    
    if(target == PID_KERNEL) {
        knl_mutex.grab_spin();
        
            //Allocate that chunk (special kernel case)
            result = knl_allocator(size);
        
        knl_mutex.release();
    } else {    
        maplist_mutex.grab_spin();
        
            list_item = find_or_create_pid(target);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            
        list_item->mutex.grab_spin();
        maplist_mutex.release();
            
            //Allocate that chunk
            result = allocator(size, list_item);
        
        list_item->mutex.release();
    }
    
    return result;
}

addr_t MemAllocator::malloc_shareable(addr_t size, PID target) {
    MallocPIDList* list_item;
    addr_t result;
    
    if(target == PID_KERNEL) {
        knl_mutex.grab_spin();
        
            //Allocate that chunk (special kernel case)
            result = knl_allocator_shareable(size);
        
        knl_mutex.release();
    } else {    
        maplist_mutex.grab_spin();
        
            list_item = find_or_create_pid(target);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            
        list_item->mutex.grab_spin();
        maplist_mutex.release();
            
            //Allocate that chunk
            result = allocator_shareable(size, list_item);
        
        list_item->mutex.release();
        
        if(!result) {
            maplist_mutex->grab_spin();
            
                //If mapping has failed, we might have created a PID without an address space, which is
                //a waste of precious memory space. Liberate it.
                if(list_item->map_pointer == NULL) remove_pid(target);
                
            maplist_mutex->release();
        }
    }
    
    return result;
}

addr_t MemAllocator::free(addr_t location, PID target) {
    MallocPIDList* list_item;
    addr_t result;
    
    if(target == PID_KERNEL) {
        knl_mutex.grab_spin();
        
            //Liberate that chunk (special kernel case)
            result = knl_liberator(location);
        
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
        
            list_item = find_pid(target);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            list_item->mutex.grab_spin();
                
                //Liberate that chunk
                result = liberator(location, list_item);
            
            list_item->mutex.release();

        maplist_mutex.release();
    }
    
    return result;
}

addr_t owneradd(addr_t location, PID current_owner, PID new_owner) {
    if(current_owner == new_owner) return NULL; //No point in doing that
    //TODO : Safely calls the appropriate share() function

    //Stub !
    return NULL;
}

void MemAllocator::print_maplist() {
    maplist_mutex.grab_spin();
    
        if(!map_list) {
            dbgout << txtcolor(TXT_RED) << "Error : Map list does not exist";
            dbgout << txtcolor(TXT_LIGHTGRAY);
        } else {
            dbgout << *map_list;
        }
    
    maplist_mutex.release();
}

void MemAllocator::print_busymap(PID owner) {
    if(owner == PID_KERNEL) {
        knl_mutex.grab_spin();
        
            if(knl_busy_map) {
                dbgout << *knl_busy_map;
            } else {
                dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
                dbgout << txtcolor(TXT_LIGHTGRAY);
            }
        
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
        
            MallocPIDList* list_item = find_pid(owner);
            if(!list_item) {
                dbgout << txtcolor(TXT_RED) << "Error : PID not registered";
                dbgout << txtcolor(TXT_LIGHTGRAY);
            }
        
        list_item->mutex.grab_spin();
        maplist_mutex.release();
        
            if(list_item) {
                if(list_item->busy_map) {
                    dbgout << *(list_item->busy_map);
                } else {
                    dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
                    dbgout << txtcolor(TXT_LIGHTGRAY);
                }
            }
        
        list_item->mutex.release();
    }
}

void MemAllocator::print_freemap(PID owner) {
    if(owner == PID_KERNEL) {
        knl_mutex.grab_spin();
        
            if(knl_free_map) {
                dbgout << *knl_free_map;
            } else {
                dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
                dbgout << txtcolor(TXT_LIGHTGRAY);
            }
        
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
        
            MallocPIDList* list_item = find_pid(owner);
            if(!list_item) {
                dbgout << txtcolor(TXT_RED) << "Error : PID not registered";
                dbgout << txtcolor(TXT_LIGHTGRAY);
            }
        
        list_item->mutex.grab_spin();
        maplist_mutex.release();
        
            if(list_item) {
                if(list_item->free_map) {
                    dbgout << *(list_item->free_map);
                } else {
                    dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
                    dbgout << txtcolor(TXT_LIGHTGRAY);
                }
            }
        
        list_item->mutex.release();
    }
}