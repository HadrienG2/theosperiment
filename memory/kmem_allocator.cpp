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

addr_t MemAllocator::allocator(const addr_t size, MallocPIDList* target, const VirMemFlags flags) {
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
    if(target->free_map) hole = target->free_map->find_contigchunk(size, flags);
    
    //Step 2 : If there's none, create it
    if(!hole) {
        //Allocating enough memory
        phy_chunk = phymem->alloc_chunk(align_pgup(size), target->map_owner);
        if(!phy_chunk) return NULL;
        vir_chunk = virmem->map(phy_chunk, target->map_owner, flags);
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
        if(!(target->free_map) || (hole->location < target->free_map->location)) {
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
            if(vir_chunk) virmem->free(vir_chunk);
            if(phy_chunk) phymem->free(phy_chunk);
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
    if(!(target->busy_map) || (allocated->location < target->busy_map->location)) {
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

addr_t MemAllocator::allocator_shareable(addr_t size,
                                         MallocPIDList* target,
                                         const VirMemFlags flags) {
    //Same as above, but always allocates a new chunk and does not put extra memory in free_map
    
    //Allocating memory
    PhyMemMap* phy_chunk = phymem->alloc_chunk(align_pgup(size), target->map_owner);
    if(!phy_chunk) return NULL;
    VirMemMap* vir_chunk = virmem->map(phy_chunk, target->map_owner, flags);
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
    if(!(target->busy_map) || (allocated->location < target->busy_map->location)) {
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

addr_t MemAllocator::liberator(const addr_t location, MallocPIDList* target) {
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
    if(target->busy_map == NULL) return NULL;
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
    if(previous_item && (previous_item->belongs_to == freed_item->belongs_to)) item_is_alone = false;
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
        phymem->ownerdel(belonged_to->points_to, target->map_owner);
        virmem->free(belonged_to);
        *freed_item = MallocMap();
        freed_item->next_item = free_mapitems;
        free_mapitems = freed_item;
        
        //Then parse free_map, looking for free items which belong to this chunk, if there are any.
        //Remove them from free_map and free them too.
        if(target->free_map) {
            freed_item = NULL;
            while(target->free_map->belongs_to == belonged_to) {
                freed_item = target->free_map;
                target->free_map = freed_item->next_item;
                *freed_item = MallocMap();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            }
            previous_item = target->free_map;
            while(previous_item->next_item) {
                if(previous_item->next_item->belongs_to == belonged_to) {
                    freed_item = previous_item->next_item;
                    previous_item->next_item = freed_item->next_item;
                    *freed_item = MallocMap();
                    freed_item->next_item = free_mapitems;
                    free_mapitems = freed_item;
                }
                previous_item = previous_item->next_item;
            }
        }
        
        //If this PID's busy_map is now empty, its free_map is empty too : remove it
        if(target->busy_map == NULL) remove_pid(target->map_owner);
        
        return location;
    }
    
    //Step 3b : Move freed_item in free_map, merging it with neighbours if possible
    //A merge is possible if the following conditions are met :
     //  -There's a contiguous item in free_map
     //  -It belongs to the same chunk of virtual memory
    if(!(target->free_map) || (freed_item->location < target->free_map->location)) {
        //Is a merge with the first map item possible ?
        if((freed_item->location+freed_item->size == target->free_map->location) &&
          (freed_item->belongs_to == target->free_map->belongs_to)) {
            //Merge with the first map item
            target->free_map->location-= freed_item->size;
            target->free_map->size+= freed_item->size;
            
            //Clean things up
            *freed_item = MallocMap();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
        } else {
            //Just put the freed item in the free_map
            freed_item->next_item = target->free_map;
            target->free_map = freed_item;
        }
    } else {
        MallocMap* map_parser = target->free_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > freed_item->location) break;
            map_parser = map_parser->next_item;
        }
        //At this point, map_parser is the map item before the place where freed_item should be
        //inserted. Try merging with this item.
        if((freed_item->location == map_parser->location+map_parser->size) &&
          (freed_item->belongs_to == map_parser->belongs_to)) {
            //Merge map_parser with freed_item and clean freed_item up
            map_parser->size+= freed_item->size;
            *freed_item = MallocMap();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
            
            //Try to merge map_parser with map_parser->next_item
            if((map_parser->location+map_parser->size == map_parser->next_item->location) &&
              (map_parser->belongs_to == map_parser->next_item->belongs_to)) {
                freed_item = map_parser->next_item;
                map_parser->size+= freed_item->size;
                *freed_item = MallocMap();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            }
        } else {
            //Try merging with the item after.
            if((freed_item->location+freed_item->size == map_parser->next_item->location) &&
              (freed_item->belongs_to == map_parser->next_item->belongs_to)) {
                //Merge freed_item with map_parser->next_item
                map_parser->next_item->location-= freed_item->size;
                map_parser->next_item->size+= freed_item->size;
                
                //Clean things up
                *freed_item = MallocMap();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            } else {
                //No merge possible, just put the freed item at its place
                freed_item->next_item = map_parser->next_item;
                map_parser->next_item = freed_item;
            }
        }
    }
    
    //All good
    return location;
}

addr_t MemAllocator::share(const addr_t location,
                           const MallocPIDList* source,
                           MallocPIDList* target,
                           const VirMemFlags flags) {
    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) return NULL;
    }

    //Setup chunk sharing
    if(source->busy_map == NULL) return NULL;
    MallocMap* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) return NULL;
    PID target_pid = target->map_owner;
    PhyMemMap* phy_item = shared_item->belongs_to->points_to;
    phy_item = phymem->owneradd(phy_item, target_pid);
    if(!phy_item) return NULL;
    VirMemMap* shared_chunk = virmem->map(phy_item, target_pid, flags);
    if(!shared_chunk) return NULL;
    
    //Make the chunks to be inserted in target->busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    MallocMap* busy_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    
    //Insert the newly created item in target's busy_map
    if(!(target->busy_map) || (busy_item->location < target->busy_map->location)) {
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
    
    return busy_item->location;
}

addr_t MemAllocator::knl_allocator(const addr_t size) {
    //This works a lot like allocator(), except that it uses knl_free_map and knl_busy_map and that
    //since paging is disabled for the kernel, the chunk is allocated through
    //PhyMemManager::alloc_contigchunk, and not mapped through virmem.

    PhyMemMap* phy_chunk = NULL;
    
    //Steps 1 : Look for a suitable hole in knl_free_map
    KnlMallocMap* hole = NULL;
    if(knl_free_map) hole = knl_free_map->find_contigchunk(size);
    
    //Step 2 : If there's none, create it
    if(!hole) {
        //Allocating enough memory
        phy_chunk = phymem->alloc_contigchunk(align_pgup(size), PID_KERNEL);
        if(!phy_chunk) return NULL;
        
        //Putting that memory in a MallocMap block
        if(!free_mapitems) {
            alloc_mapitems();
            if(!free_mapitems) {
                phymem->free(phy_chunk);
                return NULL;
            }
        }
        hole = (KnlMallocMap*) free_mapitems;
        free_mapitems = free_mapitems->next_item;
        hole->next_item = NULL;
        hole->location = phy_chunk->location;
        hole->size = phy_chunk->size;
        hole->belongs_to = phy_chunk;
        
        //Put that block in knl_free_map
        if(!knl_free_map || (hole->location < knl_free_map->location)) {
            hole->next_item = knl_free_map;
            knl_free_map = hole;
        } else {
            KnlMallocMap* map_parser = knl_free_map;
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
            return NULL;
        }
    }
    KnlMallocMap* allocated = (KnlMallocMap*) free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = hole->location;
    allocated->size = size;
    allocated->belongs_to = hole->belongs_to;
    hole->size-= size;
    hole->location+= size;
    
    //Putting the newly allocated memory at its place in knl_busy_map
    if(!knl_busy_map || (allocated->location < knl_busy_map->location)) {
        allocated->next_item = knl_busy_map;
        knl_busy_map = allocated;
    } else {
        KnlMallocMap* map_parser = knl_busy_map;
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
        if(hole == knl_free_map) {
            knl_free_map = hole->next_item;
        } else {
            KnlMallocMap* map_parser = knl_free_map;
            while(map_parser->next_item) {
                if(map_parser->next_item == hole) break;
                map_parser = map_parser->next_item;
            }
            //At this stage, map_parser->next_item is hole
            map_parser->next_item = hole->next_item;
        }
        
        //Free it
        *hole = KnlMallocMap();
        hole->next_item = (KnlMallocMap*) free_mapitems;
        free_mapitems = (MallocMap*) hole;
    }

    return allocated->location;
}

addr_t MemAllocator::knl_allocator_shareable(addr_t size) {
    //allocator_shareable(), but modified in the same way as above
    
    //Allocating memory
    PhyMemMap* phy_chunk = phymem->alloc_chunk(align_pgup(size), PID_KERNEL);
    if(!phy_chunk) return NULL;
    
    //Putting that memory in a MallocMap block
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            phymem->free(phy_chunk);
            return NULL;
        }
    }
    KnlMallocMap* allocated = (KnlMallocMap*) free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = phy_chunk->location;
    allocated->size = phy_chunk->size;
    allocated->belongs_to = phy_chunk;
        
    //Putting that block in knl_busy_map
    if(!knl_busy_map || (allocated->location < knl_busy_map->location)) {
        allocated->next_item = knl_busy_map;
        knl_busy_map = allocated;
    } else {
        KnlMallocMap* map_parser = knl_busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > allocated->location) break;
            map_parser = map_parser->next_item;
        }
        allocated->next_item = map_parser->next_item;
        map_parser->next_item = allocated;
    }

    return allocated->location;
}

addr_t MemAllocator::knl_liberator(const addr_t location) {
    //This works a lot like liberator(), except that it uses knl_free_map and knl_busy_map and that
    //   -Since paging is disabled for the kernel, the chunk is only liberated through phymem
    //   -The kernel's structures are part of MemAllocator, they are never removed.

    KnlMallocMap *freed_item, *previous_item;

    //Step 1 : Finding the item in knl_busy_map.
    //We can't use MallocMap->find_thischunk() here, since we want the previous item too.
    if(!knl_busy_map) return NULL;
    if(knl_busy_map->location == location) {
        freed_item = knl_busy_map;
        previous_item = NULL;
    } else {
        previous_item = knl_busy_map;
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
    if(previous_item && (previous_item->belongs_to == freed_item->belongs_to)) item_is_alone = false;
    if(freed_item->belongs_to == freed_item->next_item->belongs_to) item_is_alone = false;
    
    if(item_is_alone) {
        //Step 3a : Liberate the chunk and any item of free_map belonging to them.
        //To begin with, take our item out of knl_busy_map.
        if(previous_item) {
            previous_item->next_item = freed_item->next_item;
        } else {
            knl_busy_map = freed_item->next_item;
        }
        
        //Then free it and liberate the physical chunk associated with it if possible.
        //We use phymem->ownerdel instead of phymem->free here, since the process might have shared
        //this data with other processes, in which case freeing it would be a bit brutal for those.
        PhyMemMap* belonged_to = freed_item->belongs_to;
        phymem->ownerdel(belonged_to, PID_KERNEL);
        *freed_item = KnlMallocMap();
        freed_item->next_item = (KnlMallocMap*) free_mapitems;
        free_mapitems = (MallocMap*) freed_item;
        
        //Then parse knl_free_map, looking for free items which belong to this chunk, if any.
        //Remove them from knl_free_map and free them too.
        if(knl_free_map) {
            freed_item = NULL;
            while(knl_free_map->belongs_to == belonged_to) {
                freed_item = knl_free_map;
                knl_free_map = knl_free_map->next_item;
                *freed_item = KnlMallocMap();
                freed_item->next_item = (KnlMallocMap*) free_mapitems;
                free_mapitems = (MallocMap*) freed_item;
            }
            previous_item = knl_free_map;
            while(previous_item->next_item) {
                if(previous_item->next_item->belongs_to == belonged_to) {
                    freed_item = previous_item->next_item;
                    previous_item->next_item = freed_item->next_item;
                    *freed_item = KnlMallocMap();
                    freed_item->next_item = (KnlMallocMap*) free_mapitems;
                    free_mapitems = (MallocMap*) freed_item;
                }
                previous_item = previous_item->next_item;
            }
        }
        
        return location;
    }
    
    //Step 3b : Move freed_item in knl_free_map, merging it with neighbours if possible
    //A merge is possible if the following conditions are met :
     //  -There's a contiguous item in knl_free_map
     //  -It belongs to the same chunk of physical memory
    if(!knl_free_map || (freed_item->location < knl_free_map->location)) {
        //Is a merge with the first map item possible ?
        if((freed_item->location+freed_item->size == knl_free_map->location) &&
          (freed_item->belongs_to == knl_free_map->belongs_to)) {
            //Merge with the first map item
            knl_free_map->location-= freed_item->size;
            knl_free_map->size+= freed_item->size;
            
            //Clean things up
            *freed_item = KnlMallocMap();
            freed_item->next_item = (KnlMallocMap*) free_mapitems;
            free_mapitems = (MallocMap*) freed_item;
        } else {
            //Just put the freed item in knl_free_map
            freed_item->next_item = knl_free_map;
            knl_free_map = freed_item;
        }
    } else {
        KnlMallocMap* map_parser = knl_free_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > freed_item->location) break;
            map_parser = map_parser->next_item;
        }
        //At this point, map_parser is the map item before the place where freed_item should be
        //inserted. Try merging with this item.
        if((freed_item->location == map_parser->location+map_parser->size) &&
          (freed_item->belongs_to == map_parser->belongs_to)) {
            //Merge map_parser with freed_item and clean freed_item up
            map_parser->size+= freed_item->size;
            *freed_item = KnlMallocMap();
            freed_item->next_item = (KnlMallocMap*) free_mapitems;
            free_mapitems = (MallocMap*) freed_item;
            
            //Try to merge map_parser with map_parser->next_item
            if((map_parser->location+map_parser->size == map_parser->next_item->location) &&
              (map_parser->belongs_to == map_parser->next_item->belongs_to)) {
                freed_item = map_parser->next_item;
                map_parser->size+= freed_item->size;
                *freed_item = KnlMallocMap();
                freed_item->next_item = (KnlMallocMap*) free_mapitems;
                free_mapitems = (MallocMap*) freed_item;
            }
        } else {
            //Try merging with the item after.
            if((freed_item->location+freed_item->size == map_parser->next_item->location) &&
              (freed_item->belongs_to == map_parser->next_item->belongs_to)) {
                //Merge freed_item with map_parser->next_item
                map_parser->next_item->location-= freed_item->size;
                map_parser->next_item->size+= freed_item->size;
                
                //Clean things up
                *freed_item = KnlMallocMap();
                freed_item->next_item = (KnlMallocMap*) free_mapitems;
                free_mapitems = (MallocMap*) freed_item;
            } else {
                //No merge possible, just put the freed item at its place
                freed_item->next_item = map_parser->next_item;
                map_parser->next_item = freed_item;
            }
        }
    }
    
    //All good
    return location;
}

addr_t MemAllocator::share_from_knl(const addr_t location,
                                    MallocPIDList* target,
                                    const VirMemFlags flags) {
    //Like sharer(), but the original chunk belongs to the kernel

    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) return NULL;
    }

    //Setup chunk sharing
    if(!knl_busy_map) return NULL;
    KnlMallocMap* shared_item = knl_busy_map->find_thischunk(location);
    if(!shared_item) return NULL;
    PhyMemMap* phy_item = shared_item->belongs_to;
    PID target_pid = target->map_owner;
    phy_item = phymem->owneradd(phy_item, target_pid);
    if(!phy_item) return NULL;
    VirMemMap* shared_chunk = virmem->map(phy_item, target_pid, flags);
    if(!shared_chunk) return NULL;
    
    //Make the chunks to be inserted in target->busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    MallocMap* busy_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    
    //Insert the newly created item in target's busy_map
    if(!(target->busy_map) || (busy_item->location < target->busy_map->location)) {
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
    
    return busy_item->location;
}

addr_t MemAllocator::share_to_knl(const addr_t location, const MallocPIDList* source) {
    //Like sharer(), but the kernel is the recipient
    
    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) return NULL;
    }

    //Setup chunk sharing
    if(source->busy_map == NULL) return NULL;
    MallocMap* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) return NULL;
    PhyMemMap* shared_chunk = shared_item->belongs_to->points_to;
    shared_chunk = phymem->owneradd(shared_chunk, PID_KERNEL);
    if(!shared_chunk) return NULL;
    
    
    //Make the chunks to be inserted in knl_busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    KnlMallocMap* busy_item = (KnlMallocMap*) free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    
    //Insert the newly created item in knl_busy_map
    if(!knl_busy_map || (busy_item->location < knl_busy_map->location)) {
        busy_item->next_item = knl_busy_map;
        knl_busy_map = busy_item;
    } else {
        KnlMallocMap* map_parser = knl_busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > busy_item->location) break;
            map_parser = map_parser->next_item;
        }
        busy_item->next_item = map_parser->next_item;
        map_parser->next_item = busy_item;
    }
    
    return busy_item->location;
}

MallocPIDList* MemAllocator::find_pid(const PID target) {
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
        if(!result) return NULL;
        previous_item->next_item = result->next_item;
    }
    
    //Free its entry
    while(result->busy_map) liberator(result->busy_map->location, result);
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

addr_t MemAllocator::malloc(const addr_t size, PID target, const VirMemFlags flags) {
    MallocPIDList* list_item;
    addr_t result;
    
    if(target == PID_KERNEL) {
        //Kernel does not support paging, so only default flags are supported
        if(flags != VMEM_FLAGS_RW) return NULL;
        knl_mutex.grab_spin();
        
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

            result = allocator(size, list_item, flags);
        
        list_item->mutex.release();
    }
    
    return result;
}

addr_t MemAllocator::malloc_shareable(addr_t size, PID target, const VirMemFlags flags) {
    MallocPIDList* list_item;
    addr_t result;
    
    if(target == PID_KERNEL) {
        //Kernel does not support paging, so only default flags are supported
        if(flags != VMEM_FLAGS_RW) return NULL;
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
                
                //Allocate that chunk
                result = allocator_shareable(size, list_item, flags);
                
                if(!result) {
                    //If mapping has failed, we might have created a PID without an address space,
                    //which is a waste of precious memory space. Liberate it.
                    if(!(list_item->free_map) && !(list_item->busy_map)) remove_pid(target);
                }
            
            list_item->mutex.release();
        
        maplist_mutex.release();
    }
    
    return result;
}

addr_t MemAllocator::free(const addr_t location, PID target) {
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

addr_t MemAllocator::owneradd(const addr_t location,
                              const PID source,
                              PID target,
                              const VirMemFlags flags) {
    MallocPIDList *source_item, *target_item;
    addr_t result = NULL;
                                                
    if(source == PID_KERNEL) {
        //Kernel shares something with PID target
        maplist_mutex.grab_spin();
            
            target_item = find_or_create_pid(target);
            if(!target_item) {
                maplist_mutex.release();
                return NULL;
            }
            
            target_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
            knl_mutex.grab_spin();          //the same order. As knl_mutex has higher chances of
                                            //being sollicitated, grab it for the shortest time
                
                result = share_from_knl(location, target_item, flags);
                
                if(!result) {
                    //If mapping has failed, we might have created a PID without an address space,
                    //which is a waste of precious memory space. Liberate it.
                    if(!(target_item->free_map) && !(target_item->busy_map)) remove_pid(target);
                }
            
            knl_mutex.release();
            target_item->mutex.release();
            
        maplist_mutex.release();
    } if(target == PID_KERNEL) {
        //PID source shares something with kernel
        
        //Kernel does not support paging, so only default flags are supported
        if(flags != VMEM_FLAGS_RW) return NULL;
        maplist_mutex.grab_spin();
            
            source_item = find_pid(source);
            if(!source_item) {
                maplist_mutex.release();
                return NULL;
            }
            
        source_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
        knl_mutex.grab_spin();          //the same order. As knl_mutex has higher chances of being
        maplist_mutex.release();        //sollicitated, grab it for the shortest time
                
                result = share_to_knl(location, source_item);
            
        knl_mutex.release();
        source_item->mutex.release();
    } if((source != PID_KERNEL) && (target != PID_KERNEL)) {
        //PID source shares something with PID target
        maplist_mutex.grab_spin();
            
            source_item = find_pid(source);
            if(!source_item) {
                maplist_mutex.release();
                return NULL;
            }
            target_item = find_or_create_pid(target);
            if(!target_item) {
                maplist_mutex.release();
                return NULL;
            }
            
            source_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
            target_item->mutex.grab_spin(); //the same order. As knl_mutex has higher chances of
            knl_mutex.grab_spin();          //being sollicitated, grab it for the shortest time.
                                            //About source-target, it's just the logical order.
                
                result = share(location, source_item, target_item, flags);
                
                if(!result) {
                    //If mapping has failed, we might have created a PID without an address space,
                    //which is a waste of precious memory space. Liberate it.
                    if(!(target_item->free_map) && !(target_item->busy_map)) remove_pid(target);
                }
            
            knl_mutex.release();
            target_item->mutex.release();
            source_item->mutex.release();
            
        maplist_mutex.release();
    }

    return result;
}

void MemAllocator::kill(PID target) {
    if(target == PID_KERNEL) return; //Find a more constructive way to commit suicide
    
    remove_pid(target);
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

void MemAllocator::print_busymap(const PID owner) {
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

void MemAllocator::print_freemap(const PID owner) {
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

MemAllocator* kernel_allocator = NULL;

void setup_kalloc(MemAllocator& allocator) {
    kernel_allocator = &allocator;
}

void* kalloc(const addr_t size, PID target, const VirMemFlags flags) {
    return (void*) kernel_allocator->malloc(size, target, flags);
}

void* kalloc_shareable(addr_t size, PID target, const VirMemFlags flags) {
    return (void*) kernel_allocator->malloc_shareable(size, target, flags);
}

void* kfree(const void* location, PID target) {
    return (void*) kernel_allocator->free((addr_t) location, target);
}

void* kowneradd(const void* location, const PID source, PID target, const VirMemFlags flags) {
    return (void*) kernel_allocator->owneradd((addr_t) location, source, target, flags);
}