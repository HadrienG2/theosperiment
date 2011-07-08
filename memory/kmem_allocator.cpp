 /* A PhyMemManager- and VirMemManager-based memory allocator

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
#include <kstring.h>
#include <new.h>

#include <dbgstream.h>
#include <panic.h>

bool MemAllocator::alloc_mapitems() {
    size_t used_space;
    PhyMemMap* allocated_chunk;
    MallocMap* current_item;

    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized map items
    free_mapitems = (MallocMap*) (allocated_chunk->location);
    current_item = free_mapitems;
    for(used_space = sizeof(MallocMap); used_space < PG_SIZE; used_space+=sizeof(MallocMap)) {
        current_item = new(current_item) MallocMap();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) MallocMap();
    current_item->next_item = NULL;

    //All good !
    return true;
}

bool MemAllocator::alloc_listitems() {
    size_t used_space;
    PhyMemMap* allocated_chunk;
    MallocPIDList* current_item;

    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized list items
    free_listitems = (MallocPIDList*) (allocated_chunk->location);
    current_item = free_listitems;
    for(used_space = sizeof(MallocPIDList); used_space < PG_SIZE; used_space+=sizeof(MallocPIDList)) {
        current_item = new(current_item) MallocPIDList();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) MallocPIDList();
    current_item->next_item = NULL;

    //All good !
    return true;
}

size_t MemAllocator::allocator(const size_t size,
                               MallocPIDList* target,
                               const VirMemFlags flags,
                               const bool force) {
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
        phy_chunk = phymem->alloc_chunk(target->map_owner, align_pgup(size));
        if(!phy_chunk) {
            if(!force) return NULL;
            liberate_memory();
            return allocator(size, target, flags, force);
        }
        vir_chunk = virmem->map(phy_chunk, target->map_owner, flags);
        if(!vir_chunk) {
            phymem->free(phy_chunk);
            if(!force) return NULL;
            liberate_memory();
            return allocator(size, target, flags, force);
        }

        //Putting that memory in a MallocMap block
        if(!free_mapitems) {
            alloc_mapitems();
            if(!free_mapitems) {
                virmem->free(vir_chunk);
                phymem->free(phy_chunk);
                if(!force) return NULL;
                liberate_memory();
                return allocator(size, target, flags, force);
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
            if(!force) return NULL;
            liberate_memory();
            return allocator(size, target, flags, force);
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
        hole = new(hole) MallocMap();
        hole->next_item = free_mapitems;
        free_mapitems = hole;
    }

    return allocated->location;
}

size_t MemAllocator::allocator_shareable(size_t size,
                                         MallocPIDList* target,
                                         const VirMemFlags flags,
                                         const bool force) {
    //Same as above, but always allocates a new chunk and does not put extra memory in free_map

    //Allocating memory
    PhyMemMap* phy_chunk = phymem->alloc_chunk(target->map_owner, align_pgup(size));
    if(!phy_chunk) {
        if(!force) return NULL;
        liberate_memory();
        return allocator_shareable(size, target, flags, force);
    }
    VirMemMap* vir_chunk = virmem->map(phy_chunk, target->map_owner, flags);
    if(!vir_chunk) {
        phymem->free(phy_chunk);
        if(!force) return NULL;
        liberate_memory();
        return allocator_shareable(size, target, flags, force);
    }

    //Putting that memory in a MallocMap block
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            virmem->free(vir_chunk);
            phymem->free(phy_chunk);
            if(!force) return NULL;
            liberate_memory();
            return allocator_shareable(size, target, flags, force);
        }
    }
    MallocMap* allocated = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = vir_chunk->location;
    allocated->size = vir_chunk->size;
    allocated->belongs_to = vir_chunk;
    allocated->shareable = true;

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

bool MemAllocator::liberator(const size_t location, MallocPIDList* target) {
    //How it works :
    //  1.Find the relevant item in busy_map. If it fails, return false. Keep its predecessor around
    //    too : that will be useful in step 2. Check the item's share_count : if it is higher than
    //    1, decrement it and abort. If not, remove the item from busy_map.
    //  2.In the way, check if that item is the sole item belonging to its chunk of virtual memory
    //    in busy_map (examining the neighbours should be sufficient, since busy_map is sorted).
    // 3a.If so, liberate the chunk and remove any item of free_map belonging to it. If this results
    //    in busy_map being empty, free_map is necessarily empty too : remove that PID.
    // 3b.If not, move the busy_map item in free_map, merging it with neighbors if possible.

    MallocMap *freed_item, *previous_item = NULL;

    //Step 1 : Finding the item in busy_map and taking it out of said map.
    if(target->busy_map == NULL) return false;
    if(target->busy_map->location == location) {
        freed_item = target->busy_map;
        if(freed_item->share_count > 1) {
            freed_item->share_count-= 1;
            return true;
        }
        target->busy_map = freed_item->next_item;
    } else {
        previous_item = target->busy_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->location == location) {
                freed_item = previous_item->next_item;
                break;
            }
            previous_item = previous_item->next_item;
        }
        if(!freed_item) return false;
        if(freed_item->share_count > 1) {
            freed_item->share_count-= 1;
            return true;
        }
        previous_item->next_item = freed_item->next_item;
    }

    //Step 2 : Check if it was the sole busy item in this chunk by looking at its nearest neighbours
    bool item_is_alone = true;
    if(previous_item && (previous_item->belongs_to == freed_item->belongs_to)) item_is_alone = false;
    if(freed_item->belongs_to == freed_item->next_item->belongs_to) item_is_alone = false;

    if(item_is_alone) {
        //Step 3a : Liberate the chunks and any item of free_map belonging to them.

        //Liberate the physical/virtual chunks associated with our item if possible.
        //We use phymem->ownerdel instead of phymem->free here, since the process might have shared
        //this data with other processes, in which case freeing it would be a bit brutal for those.
        VirMemMap* belonged_to = freed_item->belongs_to;
        phymem->ownerdel(belonged_to->points_to, target->map_owner);
        virmem->free(belonged_to);
        freed_item = new(freed_item) MallocMap();
        freed_item->next_item = free_mapitems;
        free_mapitems = freed_item;

        //Then parse free_map, looking for free items which belong to this chunk, if there are any.
        //Remove them from free_map and free them too.
        while((target->free_map) && (target->free_map->belongs_to == belonged_to)) {
            freed_item = target->free_map;
            target->free_map = freed_item->next_item;
            freed_item = new(freed_item) MallocMap();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
        }
        if(target->free_map == NULL) {
            //Nothing left in the target's free map, so our job is done.
            //If this PID's busy_map is now empty, its free_map is empty too : remove it
            if(target->busy_map == NULL) remove_pid(target->map_owner);
            return true;
        }
        previous_item = target->free_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->belongs_to == belonged_to) {
                freed_item = previous_item->next_item;
                previous_item->next_item = freed_item->next_item;
                freed_item = new(freed_item) MallocMap();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            }
            if(previous_item->next_item) previous_item = previous_item->next_item;
        }

        //If this PID's busy_map is now empty, its free_map is empty too : remove it
        if(target->busy_map == NULL) remove_pid(target->map_owner);
        return true;
    }

    //Step 3b : Move freed_item in free_map, merging it with neighbours if possible
    //A merge is possible if the following conditions are met :
     //  -There's a contiguous item in free_map
     //  -It belongs to the same chunk of virtual memory
    if(!(target->free_map) || (freed_item->location < target->free_map->location)) {
        //The item should be put before the first item of free_map
        //Is a merge with the first map item possible ?
        if((freed_item->location+freed_item->size == target->free_map->location) &&
          (freed_item->belongs_to == target->free_map->belongs_to)) {
            //Merge with the first map item
            target->free_map->location-= freed_item->size;
            target->free_map->size+= freed_item->size;

            //Clean things up
            freed_item = new(freed_item) MallocMap();
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
            freed_item = new(freed_item) MallocMap();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;

            //Try to merge map_parser with map_parser->next_item
            if((map_parser->location+map_parser->size == map_parser->next_item->location) &&
              (map_parser->belongs_to == map_parser->next_item->belongs_to)) {
                freed_item = map_parser->next_item;
                map_parser->size+= freed_item->size;
                freed_item = new(freed_item) MallocMap();
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
                freed_item = new(freed_item) MallocMap();
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
    return true;
}

size_t MemAllocator::share(const size_t location,
                           MallocPIDList* source,
                           MallocPIDList* target,
                           const VirMemFlags flags,
                           const bool force) {
    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) {
            if(!force) return NULL;
            liberate_memory();
            return share(location, source, target, flags, force);
        }
    }

    //Setup chunk sharing
    if(source->busy_map == NULL) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    MallocMap* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    if(shared_item->shareable == false) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    PID target_pid = target->map_owner;
    PhyMemMap* phy_item = shared_item->belongs_to->points_to;
    //Check that the chunk is not shared with target already, if so simply increment the share_count
    //of the shared object in target's address space and quit. If not, pursue sharing operation
    MallocMap* already_shared = shared_already(phy_item, target);
    if(already_shared) {
        //Manage share_count overflows
        if(already_shared->share_count == 0xffffffff) {
            if(force) panic(PANIC_MAXIMAL_SHARING_REACHED);
            return NULL;
        }
        already_shared->share_count+= 1;
        return already_shared->location;
    }
    if(phymem->owneradd(phy_item, target_pid) == false) {
        //If a new item has been allocated for sharing, delete it
        if(shared_item != source->busy_map->find_thischunk(location)) {
            liberator(shared_item->location, source);
        }
        if(!force) return NULL;
        liberate_memory();
        return share(location, source, target, flags, force);
    }
    VirMemMap* shared_chunk;
    if(flags | VMEM_FLAGS_SAME) {
        shared_chunk = virmem->map(phy_item, target_pid, shared_item->belongs_to->flags);
    } else {
        shared_chunk = virmem->map(phy_item, target_pid, flags);
    }
    if(!shared_chunk) {
        phymem->ownerdel(phy_item, target_pid);
        //If a new item has been allocated for sharing, delete it
        if(shared_item != source->busy_map->find_thischunk(location)) {
            liberator(shared_item->location, source);
        }
        if(!force) return NULL;
        liberate_memory();
        return share(location, source, target, flags, force);

    }

    //Make the chunks to be inserted in target->busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    MallocMap* busy_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    busy_item->shareable = true;
    busy_item->share_count = 1;

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

MallocMap* MemAllocator::shared_already(PhyMemMap* to_share, MallocPIDList* target_owner) {
    //This function checks if a physical page "to_share" is already shared with "target_owner", and
    //if so returns a pointer to the MallocMap object associated with the shared object.
    MallocMap* map_parser = target_owner->busy_map;
    while(map_parser) {
        if(map_parser->belongs_to->points_to == to_share) break;
        map_parser = map_parser->next_item;
    }
    
    return map_parser;
}

size_t MemAllocator::knl_allocator(const size_t size, const bool force) {
    //This works a lot like allocator(), except that it uses knl_free_map and knl_busy_map and that
    //since paging is disabled for the kernel, the chunk is allocated through
    //PhyMemManager::alloc_chunk with contiguous flag on, and not mapped through virmem.

    PhyMemMap* phy_chunk = NULL;

    //Step 1 : Look for a suitable hole in knl_free_map
    KnlMallocMap* hole = NULL;
    if(knl_free_map) hole = knl_free_map->find_contigchunk(size);

    //Step 2 : If there's none, create it
    if(!hole) {
        //Allocating enough contiguous memory
        phy_chunk = phymem->alloc_chunk(PID_KERNEL, align_pgup(size), true);
        if(!phy_chunk) {
            if(!force) return NULL;
            liberate_memory();
            return knl_allocator(size, force);
        }

        //Putting that memory in a MallocMap block
        if(!free_mapitems) {
            alloc_mapitems();
            if(!free_mapitems) {
                phymem->free(phy_chunk);
                if(!force) return NULL;
                liberate_memory();
                return knl_allocator(size, force);
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
            if(!force) return NULL;
            liberate_memory();
            return knl_allocator(size, force);
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
        hole = new(hole) KnlMallocMap();
        hole->next_item = (KnlMallocMap*) free_mapitems;
        free_mapitems = (MallocMap*) hole;
    }

    return allocated->location;
}

size_t MemAllocator::knl_allocator_shareable(size_t size, const bool force) {
    //allocator_shareable(), but modified in the same way as above

    //Allocating memory
    PhyMemMap* phy_chunk = phymem->alloc_chunk(PID_KERNEL, align_pgup(size), true);
    if(!phy_chunk) {
        if(!force) return NULL;
        liberate_memory();
        return knl_allocator_shareable(size, force);
    }

    //Putting that memory in a MallocMap block
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            phymem->free(phy_chunk);
            if(!force) return NULL;
            liberate_memory();
            return knl_allocator_shareable(size, force);
        }
    }
    KnlMallocMap* allocated = (KnlMallocMap*) free_mapitems;
    free_mapitems = free_mapitems->next_item;
    allocated->next_item = NULL;
    allocated->location = phy_chunk->location;
    allocated->size = phy_chunk->size;
    allocated->belongs_to = phy_chunk;
    allocated->shareable = true;

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

bool MemAllocator::knl_liberator(const size_t location) {
    //This works a lot like liberator(), except that it uses knl_free_map and knl_busy_map and that
    //   -Since paging is disabled for the kernel, the chunk is only liberated through phymem
    //   -The kernel's structures are part of MemAllocator, they are never removed.
    
    KnlMallocMap *freed_item, *previous_item = NULL;

    //Step 1 : Find the item in knl_busy_map and take it out of said map.
    if(!knl_busy_map) return false;
    if(knl_busy_map->location == location) {
        freed_item = knl_busy_map;
        if(freed_item->share_count > 1) {
            freed_item->share_count-= 1;
            return true;
        }
        knl_busy_map = freed_item->next_item;
    } else {
        previous_item = knl_busy_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->location == location) {
                freed_item = previous_item->next_item;
                break;
            }
            previous_item = previous_item->next_item;
        }
        if(!freed_item) return false;
        if(freed_item->share_count > 1) {
            freed_item->share_count-= 1;
            return true;
        }
        previous_item->next_item = freed_item->next_item;
    }

    //Step 2 : Check if it was the sole busy item in this chunk by looking at its nearest neighbours
    bool item_is_alone = true;
    if(previous_item && (previous_item->belongs_to == freed_item->belongs_to)) item_is_alone = false;
    if(freed_item->belongs_to == freed_item->next_item->belongs_to) item_is_alone = false;

    if(item_is_alone) {
        //Step 3a : Liberate the chunk and any item of free_map belonging to them.

        //We use phymem->ownerdel instead of phymem->free here, since the process might have shared
        //this data with other processes, in which case freeing it would be a bit brutal for those.
        PhyMemMap* belonged_to = freed_item->belongs_to;
        phymem->ownerdel(belonged_to, PID_KERNEL);
        freed_item = new(freed_item) KnlMallocMap();
        freed_item->next_item = (KnlMallocMap*) free_mapitems;
        free_mapitems = (MallocMap*) freed_item;

        //Then parse knl_free_map, looking for free items which belong to this chunk, if any.
        //Remove them from knl_free_map and free them too.
        while(knl_free_map && (knl_free_map->belongs_to == belonged_to)) {
            freed_item = knl_free_map;
            knl_free_map = knl_free_map->next_item;
            freed_item = new(freed_item) KnlMallocMap();
            freed_item->next_item = (KnlMallocMap*) free_mapitems;
            free_mapitems = (MallocMap*) freed_item;
        }
        if(!knl_free_map) return true; //knl_free_map is empty so the job is done
        previous_item = knl_free_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->belongs_to == belonged_to) {
                freed_item = previous_item->next_item;
                previous_item->next_item = freed_item->next_item;
                freed_item = new(freed_item) KnlMallocMap();
                freed_item->next_item = (KnlMallocMap*) free_mapitems;
                free_mapitems = (MallocMap*) freed_item;
            }
            if(previous_item->next_item) previous_item = previous_item->next_item;
        }

        return true;
    }

    //Step 3b : Move freed_item in knl_free_map, merging it with neighbours if possible
    //A merge is possible if the following conditions are met :
     //  -There's a contiguous item in knl_free_map
     //  -It belongs to the same chunk of physical memory
    if(!knl_free_map || (freed_item->location < knl_free_map->location)) {
        //Our item should be in the beginning of knl_free_map.
        //Is a merge with the first map item possible ?
        if((freed_item->location+freed_item->size == knl_free_map->location) &&
          (freed_item->belongs_to == knl_free_map->belongs_to)) {
            //Merge with the first map item
            knl_free_map->location-= freed_item->size;
            knl_free_map->size+= freed_item->size;

            //Clean things up
            freed_item = new(freed_item) KnlMallocMap();
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
            freed_item = new(freed_item) KnlMallocMap();
            freed_item->next_item = (KnlMallocMap*) free_mapitems;
            free_mapitems = (MallocMap*) freed_item;

            //Try to merge map_parser with map_parser->next_item
            if((map_parser->location+map_parser->size == map_parser->next_item->location) &&
              (map_parser->belongs_to == map_parser->next_item->belongs_to)) {
                freed_item = map_parser->next_item;
                map_parser->size+= freed_item->size;
                freed_item = new(freed_item) KnlMallocMap();
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
                freed_item = new(freed_item) KnlMallocMap();
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
    return true;
}

size_t MemAllocator::share_from_knl(const size_t location,
                                    MallocPIDList* target,
                                    const VirMemFlags flags,
                                    const bool force) {
    //Like sharer(), but the original chunk belongs to the kernel

    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) {
            if(!force) return NULL;
            liberate_memory();
            return share_from_knl(location, target, flags, force);
        }
    }

    //Setup chunk sharing
    if(!knl_busy_map) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    KnlMallocMap* shared_item = knl_busy_map->find_thischunk(location);
    if(!shared_item) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    if(shared_item->shareable == false) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    PhyMemMap* phy_item = shared_item->belongs_to;
    //Check that the chunk is not shared with target already, if so simply increment the share_count
    //of the shared object in target's address space and quit. If not, pursue sharing operation
    MallocMap* already_shared = shared_already(phy_item, target);
    if(already_shared) {
        //Manage share_count overflows
        if(already_shared->share_count == 0xffffffff) {
            if(force) panic(PANIC_MAXIMAL_SHARING_REACHED);
            return NULL;
        }
        already_shared->share_count+= 1;
        return already_shared->location;
    }
    PID target_pid = target->map_owner;
    if(phymem->owneradd(phy_item, target_pid) == false) {
        //If a new item has been allocated for sharing, delete it
        if(shared_item != knl_busy_map->find_thischunk(location)) {
            knl_liberator(shared_item->location);
        }
        if(!force) return NULL;
        liberate_memory();
        return share_from_knl(location, target, flags, force);
    }
    VirMemMap* shared_chunk;
    if(flags | VMEM_FLAGS_SAME) {
        shared_chunk = virmem->map(phy_item, target_pid, VMEM_FLAGS_RW);
    } else {
        shared_chunk = virmem->map(phy_item, target_pid, flags);
    }
    if(!shared_chunk) {
        phymem->ownerdel(phy_item, target_pid);
        //If a new item has been allocated for sharing, delete it
        if(shared_item != knl_busy_map->find_thischunk(location)) {
            knl_liberator(shared_item->location);
        }
        if(!force) return NULL;
        liberate_memory();
        return share_from_knl(location, target, flags, force);
    }

    //Make the chunks to be inserted in target->busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    MallocMap* busy_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    busy_item->shareable = true;
    busy_item->share_count = 1;

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

size_t MemAllocator::share_to_knl(const size_t location,
                                  MallocPIDList* source,
                                  const VirMemFlags flags,
                                  const bool force) {
    //Like sharer(), but the kernel is the recipient

    //Allocate management structures (we need at most two MallocMaps)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) {
            if(!force) return NULL;
            liberate_memory();
            return share_to_knl(location, source, flags, force);
        }
    }

    //Setup chunk sharing
    if(source->busy_map == NULL) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    MallocMap* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    if(shared_item->shareable == false) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    //If "same" virtual memory flags are used, check that the original memory chunk has RW memory
    //flags, as paging is disabled for the kernel
    if((flags | VMEM_FLAGS_SAME) && (shared_item->belongs_to->flags != VMEM_FLAGS_RW)) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    PhyMemMap* shared_chunk = shared_item->belongs_to->points_to;
    //Check that the chunk is not shared with target already, if so simply increment the share_count
    //of the shared object in target's address space and quit. If not, pursue sharing operation
    KnlMallocMap* already_shared = shared_to_knl_already(shared_chunk);
    if(already_shared) {
        //Manage share_count overflows
        if(already_shared->share_count == 0xffffffff) {
            if(force) panic(PANIC_MAXIMAL_SHARING_REACHED);
            return NULL;
        }
        already_shared->share_count+= 1;
        return already_shared->location;
    }
    if(phymem->owneradd(shared_chunk, PID_KERNEL) == false) {
        //If a new item has been allocated for sharing, delete it
        if(shared_item != source->busy_map->find_thischunk(location)) {
            liberator(shared_item->location, source);
        }
        if(!force) return NULL;
        liberate_memory();
        return share_to_knl(location, source, flags, force);
    }


    //Make the chunks to be inserted in knl_busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    KnlMallocMap* busy_item = (KnlMallocMap*) free_mapitems;
    free_mapitems = free_mapitems->next_item;
    busy_item->location = shared_chunk->location;
    busy_item->size = shared_chunk->size;
    busy_item->belongs_to = shared_chunk;
    busy_item->shareable = true;
    busy_item->share_count = 1;

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

KnlMallocMap* MemAllocator::shared_to_knl_already(PhyMemMap* to_share) {
    //This function checks if a physical page "to_share" is already shared with the kernel, and
    //if so returns a pointer to the MallocMap object associated with the shared object.
    KnlMallocMap* map_parser = knl_busy_map;
    while(map_parser) {
        if(map_parser->belongs_to == to_share) break;
        map_parser = map_parser->next_item;
    }
    
    return map_parser;
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

MallocPIDList* MemAllocator::find_or_create_pid(PID target, const bool force) {
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
    if((force) && (list_item == NULL)) {
        liberate_memory();
        return find_or_create_pid(target, force);
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

bool MemAllocator::remove_pid(PID target) {
    MallocPIDList *deleted_item, *previous_item;

    //Remove the PID from the map list
    if(map_list->map_owner == target) {
        deleted_item = map_list;
        map_list = map_list->next_item;
    } else {
        previous_item = map_list;
        while(previous_item->next_item) {
            if(previous_item->next_item->map_owner == target) break;
            previous_item = previous_item->next_item;
        }
        deleted_item = previous_item->next_item;
        if(!deleted_item) return false;
        previous_item->next_item = deleted_item->next_item;
    }

    //Free its entry
    while(deleted_item->busy_map) liberator(deleted_item->busy_map->location, deleted_item);
    deleted_item = new(deleted_item) MallocPIDList();
    deleted_item->next_item = free_listitems;
    free_listitems = deleted_item;

    return true;
}

void MemAllocator::liberate_memory() {
    //Draft !
    panic(PANIC_OUT_OF_MEMORY);
}

MemAllocator::MemAllocator(PhyMemManager& physmem, VirMemManager& virtmem) : phymem(&physmem),
                                                                             virmem(&virtmem),
                                                                             map_list(NULL),
                                                                             knl_free_map(NULL),
                                                                             knl_busy_map(NULL),
                                                                             knl_pool(NULL),
                                                                             free_mapitems(NULL),
                                                                             free_listitems(NULL) {
    alloc_mapitems();
    alloc_listitems();
}

size_t MemAllocator::malloc(const size_t size, PID target, const VirMemFlags flags, const bool force) {
    if(!size) return NULL;
    MallocPIDList* list_item;
    size_t result;
    
    if(target == PID_KERNEL) {
        //Kernel does not support paging, so only default flags are supported
        if(flags != VMEM_FLAGS_RW) {
            if(force) panic(PANIC_IMPOSSIBLE_KERNEL_FLAGS);
            return NULL;
        }
        knl_mutex.grab_spin();
            
            if(knl_pool) {
                result = knl_pool;
                knl_pool+= size;
            } else {
                result = knl_allocator(size, force);
            }
            
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
            
            list_item = find_or_create_pid(target, force);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            
        list_item->mutex.grab_spin();
        maplist_mutex.release();
            
            if(list_item->pool) {
                result = list_item->pool;
                list_item->pool+= size;
            } else {
                result = allocator(size, list_item, flags, force);
            }
            
        list_item->mutex.release();
    }
    
    return result;
}

size_t MemAllocator::malloc_shareable(size_t size,
                                      PID target,
                                      const VirMemFlags flags,
                                      const bool force) {
    MallocPIDList* list_item;
    size_t result;

    if(target == PID_KERNEL) {
        //Kernel does not support paging, so only default flags are supported
        if(flags != VMEM_FLAGS_RW) {
            if(force) panic(PANIC_IMPOSSIBLE_KERNEL_FLAGS);
            return NULL;
        }
        knl_mutex.grab_spin();
            
            if(knl_pool) {
                result = knl_pool;
                knl_pool+= size;
            } else {
                result = knl_allocator_shareable(size, force);
            }
            
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
            
            list_item = find_or_create_pid(target, force);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            
            list_item->mutex.grab_spin();
                
                if(list_item->pool) {
                    result = list_item->pool;
                    list_item->pool+= size;
                } else {
                    result = allocator_shareable(size, list_item, flags, force);
                    if(!result) {
                        //If mapping has failed, we might have created a PID without an address space,
                        //which is a waste of precious memory space. Liberate it.
                        if(!(list_item->free_map) && !(list_item->busy_map)) remove_pid(target);
                    }
                }
                
            list_item->mutex.release();
            
        maplist_mutex.release();
    }
    
    return result;
}

bool MemAllocator::free(const size_t location, PID target) {
    MallocPIDList* list_item;
    bool result;

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

size_t MemAllocator::owneradd(const size_t location,
                              PID source,
                              PID target,
                              const VirMemFlags flags,
                              const bool force) {
    MallocPIDList *source_item, *target_item;
    size_t result = NULL;

    if(source == PID_KERNEL) {
        //Kernel shares something with PID target
        maplist_mutex.grab_spin();

            target_item = find_or_create_pid(target, force);
            if(!target_item) {
                maplist_mutex.release();
                return NULL;
            }

            target_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
            knl_mutex.grab_spin();          //the same order. As knl_mutex has higher chances of
                                            //being sollicitated, grab it for the shortest time

                result = share_from_knl(location, target_item, flags, force);

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
        if((flags != VMEM_FLAGS_RW) && (flags != VMEM_FLAGS_SAME)) {
            if(force) panic(PANIC_IMPOSSIBLE_KERNEL_FLAGS); //Stub !
            return NULL;
        }
        maplist_mutex.grab_spin();

            source_item = find_pid(source);
            if(!source_item) {
                maplist_mutex.release();
                if(force) panic(PANIC_IMPOSSIBLE_SHARING); //Stub !
                return NULL;
            }

        source_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
        knl_mutex.grab_spin();          //the same order. As knl_mutex has higher chances of being
        maplist_mutex.release();        //sollicitated, grab it for the shortest time

                result = share_to_knl(location, source_item, flags, force);

        knl_mutex.release();
        source_item->mutex.release();
    } if((source != PID_KERNEL) && (target != PID_KERNEL)) {
        //PID source shares something with PID target
        maplist_mutex.grab_spin();

            source_item = find_pid(source);
            if(!source_item) {
                maplist_mutex.release();
                if(force) panic(PANIC_IMPOSSIBLE_SHARING); //Stub !
                return NULL;
            }
            target_item = find_or_create_pid(target, force);
            if(!target_item) {
                maplist_mutex.release();
                return NULL;
            }

            source_item->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
            target_item->mutex.grab_spin(); //the same order. As knl_mutex has higher chances of
            knl_mutex.grab_spin();          //being sollicitated, grab it for the shortest time.
                                            //About source-target, it's just the logical order.

                result = share(location, source_item, target_item, flags, force);

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

size_t MemAllocator::init_pool(const size_t size,
                               PID target,
                               const VirMemFlags flags,
                               const bool force) {
    size_t pool = malloc(size, target, flags, force);
    if(!pool) return NULL;
    set_pool(pool, target);
    return pool;
}

size_t MemAllocator::init_pool_shareable(const size_t size,
                                         PID target,
                                         const VirMemFlags flags,
                                         const bool force) {
    size_t pool = malloc_shareable(size, target, flags, force);
    if(!pool) return NULL;
    set_pool(pool, target);
    return pool;
}

size_t MemAllocator::leave_pool(PID target) {
    MallocPIDList* list_item;
    size_t pool_state;
    
    if(target == PID_KERNEL) {
        knl_mutex.grab_spin();
            
            pool_state = knl_pool;
            knl_pool = NULL;
            
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
            
            list_item = find_pid(target);
            if(!list_item) {
                maplist_mutex.release();
                return NULL;
            }
            
        list_item->mutex.grab_spin();
        maplist_mutex.release();
            
            pool_state = list_item->pool;
            list_item->pool = NULL;
            
        list_item->mutex.release();
    }
    
    return pool_state;
}

bool MemAllocator::set_pool(size_t pool, PID target, const bool force) {
    MallocPIDList* list_item;
    
    if(target == PID_KERNEL) {
        knl_mutex.grab_spin();
            
            knl_pool = pool;
            
        knl_mutex.release();
    } else {
        maplist_mutex.grab_spin();
            
            list_item = find_or_create_pid(target, force);
            if(!list_item) {
                maplist_mutex.release();
                return false;
            }
            
        list_item->mutex.grab_spin();
        maplist_mutex.release();
            
            list_item->pool = pool;
            
        list_item->mutex.release();
    }
    
    return true;
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

void* kalloc(const size_t size, PID target, const VirMemFlags flags, const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) kernel_allocator->malloc(size, target, flags, force);
}

void* kalloc_shareable(size_t size, PID target, const VirMemFlags flags, const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) kernel_allocator->malloc_shareable(size, target, flags, force);
}

bool kfree(void* location, PID target) {
    if(!kernel_allocator) return false;
    return (void*) kernel_allocator->free((size_t) location, target);
}

void* kowneradd(const void* location,
                const PID source,
                PID target,
                const VirMemFlags flags,
                const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) kernel_allocator->owneradd((size_t) location, source, target, flags, force);
}

void* kinit_pool(const size_t size,
                 PID target,
                 const VirMemFlags flags,
                 const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) kernel_allocator->init_pool(size, target, flags, force);
}

void* kinit_pool_shareable(const size_t size,
                           PID target,
                           const VirMemFlags flags,
                           const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) kernel_allocator->init_pool_shareable(size, target, flags, force);
}

void* kleave_pool(PID target) {
    if(!kernel_allocator) return NULL;
    return (void*) kernel_allocator->leave_pool(target);
}

bool kset_pool(void* pool, PID target, const bool force) {
    if(!kernel_allocator) {
        if(!force) return NULL;
        //Stub !
        panic(PANIC_MM_UNINITIALIZED);
    }
    return kernel_allocator->set_pool((size_t) pool, target, force);
}
