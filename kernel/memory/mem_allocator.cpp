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
#include <mem_allocator.h>
#include <kstring.h>
#include <new.h>

#include <dbgstream.h>
#include <panic.h>

MemAllocator* mem_allocator = NULL;

bool MemAllocator::alloc_mapitems() {
    size_t used_space;
    PhyMemChunk* allocated_page;
    MemoryChunk* current_item;

    //Allocate a page of memory
    allocated_page = phymem_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_page) return false;

    //Fill it with initialized map items
    free_mapitems = (MemoryChunk*) (allocated_page->location);
    current_item = free_mapitems;
    for(used_space = sizeof(MemoryChunk); used_space < PG_SIZE; used_space+=sizeof(MemoryChunk)) {
        current_item = new(current_item) MemoryChunk();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) MemoryChunk();
    current_item->next_item = NULL;

    //All good !
    return true;
}

bool MemAllocator::alloc_process_descs() {
    size_t used_space;
    PhyMemChunk* allocated_page;
    MallocProcess* current_item;

    //Allocate a page of memory
    allocated_page = phymem_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_page) return false;

    //Fill it with initialized list items
    free_process_descs = (MallocProcess*) (allocated_page->location);
    current_item = free_process_descs;
    for(used_space = sizeof(MallocProcess); used_space < PG_SIZE; used_space+=sizeof(MallocProcess)) {
        current_item = new(current_item) MallocProcess();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) MallocProcess();
    current_item->next_item = NULL;

    //All good !
    return true;
}

size_t MemAllocator::allocator(MallocProcess* target,
                               const size_t size,
                               const VirMemFlags flags,
                               const bool force) {
    //How it works :
    //  1.Look for a suitable hole in the target's free_map linked list
    //  2.If there is none, allocate a chunk through phymem and map it through virmem, then put
    //    it in free_map. Return NULL if it fails
    //  3.Take the requested space from the chunk found/created in free_map, update free_map and
    //    busy_map

    PhyMemChunk* phy_chunk = NULL;
    VirMemChunk* vir_chunk = NULL;

    //Steps 1 : Look for a suitable hole in target->free_map
    MemoryChunk* hole = NULL;
    if(target->free_map) hole = target->free_map->find_contigchunk(size, flags);

    //Step 2 : If there's none, create it
    if(!hole) {
        //Allocating enough memory
        phy_chunk = phymem_manager->alloc_chunk(target->identifier, align_pgup(size));
        if(!phy_chunk) {
            if(!force) return NULL;
            liberate_memory();
            return allocator(target, size, flags, force);
        }
        vir_chunk = virmem_manager->map_chunk(target->identifier, phy_chunk, flags);
        if(!vir_chunk) {
            phymem_manager->free_chunk(target->identifier, phy_chunk->location);
            if(!force) return NULL;
            liberate_memory();
            return allocator(target, size, flags, force);
        }

        //Putting that memory in a MemoryChunk block
        if(!free_mapitems) {
            alloc_mapitems();
            if(!free_mapitems) {
                virmem_manager->free_chunk(target->identifier, vir_chunk->location);
                phymem_manager->free_chunk(target->identifier, phy_chunk->location);
                if(!force) return NULL;
                liberate_memory();
                return allocator(target, size, flags, force);
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
            MemoryChunk* map_parser = target->free_map;
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
            if(vir_chunk) virmem_manager->free_chunk(target->identifier, vir_chunk->location);
            if(phy_chunk) phymem_manager->free_chunk(target->identifier, phy_chunk->location);
            if(!force) return NULL;
            liberate_memory();
            return allocator(target, size, flags, force);
        }
    }
    MemoryChunk* allocated = free_mapitems;
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
        MemoryChunk* map_parser = target->busy_map;
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
            MemoryChunk* map_parser = target->free_map;
            while(map_parser->next_item) {
                if(map_parser->next_item == hole) break;
                map_parser = map_parser->next_item;
            }
            //At this stage, map_parser->next_item is hole
            map_parser->next_item = hole->next_item;
        }

        //Free it
        hole = new(hole) MemoryChunk();
        hole->next_item = free_mapitems;
        free_mapitems = hole;
    }

    return allocated->location;
}

size_t MemAllocator::allocator_shareable(MallocProcess* target,
                                         size_t size,
                                         const VirMemFlags flags,
                                         const bool force) {
    //Same as above, but always allocates a new chunk and does not put extra memory in free_map

    //Allocating memory
    PhyMemChunk* phy_chunk = phymem_manager->alloc_chunk(target->identifier, align_pgup(size));
    if(!phy_chunk) {
        if(!force) return NULL;
        liberate_memory();
        return allocator_shareable(target, size, flags, force);
    }
    VirMemChunk* vir_chunk = virmem_manager->map_chunk(target->identifier, phy_chunk, flags);
    if(!vir_chunk) {
        phymem_manager->free_chunk(target->identifier, phy_chunk->location);
        if(!force) return NULL;
        liberate_memory();
        return allocator_shareable(target, size, flags, force);
    }

    //Putting that memory in a MemoryChunk block
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) {
            virmem_manager->free_chunk(target->identifier, vir_chunk->location);
            phymem_manager->free_chunk(target->identifier, phy_chunk->location);
            if(!force) return NULL;
            liberate_memory();
            return allocator_shareable(target, size, flags, force);
        }
    }
    MemoryChunk* allocated = free_mapitems;
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
        MemoryChunk* map_parser = target->busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > allocated->location) break;
            map_parser = map_parser->next_item;
        }
        allocated->next_item = map_parser->next_item;
        map_parser->next_item = allocated;
    }

    return allocated->location;
}

bool MemAllocator::liberator(MallocProcess* target, const size_t location) {
    //How it works :
    //  1.Find the relevant item in busy_map. If it fails, return false. Keep its predecessor around
    //    too : that will be useful in step 2. Check the item's share_count : if it is higher than
    //    1, decrement it and abort. If not, remove the item from busy_map.
    //  2.In the way, check if that item is the sole item belonging to its chunk of virtual memory
    //    in busy_map (examining the neighbours should be sufficient, since busy_map is sorted).
    // 3a.If so, liberate the chunk and remove any item of free_map belonging to it. If this results
    //    in busy_map being empty, free_map is necessarily empty too : remove that PID.
    // 3b.If not, move the busy_map item in free_map, merging it with neighbors if possible.

    MemoryChunk *freed_item, *previous_item = NULL;

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
        //We use phymem->identifierdel instead of phymem->free here, since the process might have shared
        //this data with other processes, in which case freeing it would be a bit brutal for those.
        VirMemChunk* belonged_to = freed_item->belongs_to;
        phymem_manager->free_chunk(target->identifier, belonged_to->points_to->location);
        virmem_manager->free_chunk(target->identifier, belonged_to->location);
        freed_item = new(freed_item) MemoryChunk();
        freed_item->next_item = free_mapitems;
        free_mapitems = freed_item;

        //Then parse free_map, looking for free items which belong to this chunk, if there are any.
        //Remove them from free_map and free them too.
        while((target->free_map) && (target->free_map->belongs_to == belonged_to)) {
            freed_item = target->free_map;
            target->free_map = freed_item->next_item;
            freed_item = new(freed_item) MemoryChunk();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
        }
        if(target->free_map == NULL) {
            //Nothing left in the target's free map, so our job is done.
            //If this PID's busy_map is now empty, its free_map is empty too : remove it
            if(target->busy_map == NULL) remove_pid(target->identifier);
            return true;
        }
        previous_item = target->free_map;
        while(previous_item->next_item) {
            if(previous_item->next_item->belongs_to == belonged_to) {
                freed_item = previous_item->next_item;
                previous_item->next_item = freed_item->next_item;
                freed_item = new(freed_item) MemoryChunk();
                freed_item->next_item = free_mapitems;
                free_mapitems = freed_item;
            }
            if(previous_item->next_item) previous_item = previous_item->next_item;
        }

        //If this PID's busy_map is now empty, its free_map is empty too : remove it
        if(target->busy_map == NULL) remove_pid(target->identifier);
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
            freed_item = new(freed_item) MemoryChunk();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;
        } else {
            //Just put the freed item in the free_map
            freed_item->next_item = target->free_map;
            target->free_map = freed_item;
        }
    } else {
        MemoryChunk* map_parser = target->free_map;
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
            freed_item = new(freed_item) MemoryChunk();
            freed_item->next_item = free_mapitems;
            free_mapitems = freed_item;

            //Try to merge map_parser with map_parser->next_item
            if((map_parser->location+map_parser->size == map_parser->next_item->location) &&
              (map_parser->belongs_to == map_parser->next_item->belongs_to)) {
                freed_item = map_parser->next_item;
                map_parser->size+= freed_item->size;
                freed_item = new(freed_item) MemoryChunk();
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
                freed_item = new(freed_item) MemoryChunk();
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

size_t MemAllocator::share(MallocProcess* source,
                           const size_t location,
                           MallocProcess* target,
                           const VirMemFlags flags,
                           const bool force) {
    //This function shares a memory block of source with target, giving target "flags" access flags

    //Allocate management structures (we need at most two MemoryChunks)
    if(!free_mapitems || !(free_mapitems->next_item)) {
        alloc_mapitems();
        if(!free_mapitems || !(free_mapitems->next_item)) {
            if(!force) return NULL;
            liberate_memory();
            return share(source, location, target, flags, force);
        }
    }

    //Setup chunk sharing
    if(source->busy_map == NULL) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    MemoryChunk* shared_item = source->busy_map->find_thischunk(location);
    if(!shared_item) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    if(shared_item->shareable == false) {
        if(force) panic(PANIC_IMPOSSIBLE_SHARING);
        return NULL;
    }
    PID target_pid = target->identifier;
    PhyMemChunk* phy_item = shared_item->belongs_to->points_to;
    //Check that the chunk is not shared with target already, if so simply increment the share_count
    //of the shared object in target's address space and quit. If not, pursue sharing operation
    MemoryChunk* already_shared = shared_already(phy_item, target);
    if(already_shared) {
        //Manage share_count overflows
        if(already_shared->share_count == 0xffffffff) {
            if(force) panic(PANIC_MAXIMAL_SHARING_REACHED);
            return NULL;
        }
        already_shared->share_count+= 1;
        return already_shared->location;
    }
    if(phymem_manager->share_chunk(target_pid, phy_item->location) == false) {
        //If a new item has been allocated for sharing, delete it
        if(shared_item != source->busy_map->find_thischunk(location)) {
            liberator(source, shared_item->location);
        }
        if(!force) return NULL;
        liberate_memory();
        return share(source, location, target, flags, force);
    }
    VirMemChunk* shared_chunk;
    if(flags | VIRMEM_FLAGS_SAME) {
        shared_chunk = virmem_manager->map_chunk(target_pid, phy_item, shared_item->belongs_to->flags);
    } else {
        shared_chunk = virmem_manager->map_chunk(target_pid, phy_item, flags);
    }
    if(!shared_chunk) {
        phymem_manager->free_chunk(target_pid, phy_item->location);
        //If a new item has been allocated for sharing, delete it
        if(shared_item != source->busy_map->find_thischunk(location)) {
            liberator(source, shared_item->location);
        }
        if(!force) return NULL;
        liberate_memory();
        return share(source, location, target, flags, force);

    }

    //Make the chunks to be inserted in target->busy_map. Do not store the remaining memory
    //in shared_chunk as free memory, as it would lead to unwanted data sharing
    MemoryChunk* busy_item = free_mapitems;
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
        MemoryChunk* map_parser = target->busy_map;
        while(map_parser->next_item) {
            if(map_parser->next_item->location > busy_item->location) break;
            map_parser = map_parser->next_item;
        }
        busy_item->next_item = map_parser->next_item;
        map_parser->next_item = busy_item;
    }

    return busy_item->location;
}

MemoryChunk* MemAllocator::shared_already(PhyMemChunk* to_share, MallocProcess* target_identifier) {
    //This function checks if a physical page "to_share" is already shared with "target_identifier", and
    //if so returns a pointer to the MemoryChunk object associated with the shared object.
    MemoryChunk* map_parser = target_identifier->busy_map;
    while(map_parser) {
        if(map_parser->belongs_to->points_to == to_share) break;
        map_parser = map_parser->next_item;
    }

    return map_parser;
}

MallocProcess* MemAllocator::find_pid(const PID target) {
    MallocProcess* process;

    process = process_list;
    while(process) {
        if(process->identifier == target) break;
        process = process->next_item;
    }

    return process;
}

MallocProcess* MemAllocator::find_or_create_pid(PID target, const bool force) {
    MallocProcess* process;

    if(!process_list) {
        process_list = setup_pid(target);
        return process_list;
    }
    process = process_list;
    while(process->next_item) {
        if(process->identifier == target) break;
        process = process->next_item;
    }
    if(process->identifier != target) {
        process->next_item = setup_pid(target);
        process = process->next_item;
    }
    if((force) && (process == NULL)) {
        liberate_memory();
        return find_or_create_pid(target, force);
    }

    return process;
}

MallocProcess* MemAllocator::setup_pid(PID target) {
    MallocProcess* result;

    //Allocate management structures
    if(!free_process_descs) {
        alloc_process_descs();
        if(!free_process_descs) return NULL;
    }

    //Fill them
    result = free_process_descs;
    free_process_descs = free_process_descs->next_item;
    result->identifier = target;
    result->next_item = NULL;

    return result;
}

bool MemAllocator::remove_pid(PID target) {
    MallocProcess *deleted_process, *previous_process;

    //Remove the PID from the map list
    if(process_list->identifier == target) {
        deleted_process = process_list;
        process_list = process_list->next_item;
    } else {
        previous_process = process_list;
        while(previous_process->next_item) {
            if(previous_process->next_item->identifier == target) break;
            previous_process = previous_process->next_item;
        }
        deleted_process = previous_process->next_item;
        if(!deleted_process) return false;
        previous_process->next_item = deleted_process->next_item;
    }

    //Free its entry
    while(deleted_process->busy_map) liberator(deleted_process, deleted_process->busy_map->location);
    deleted_process = new(deleted_process) MallocProcess();
    deleted_process->next_item = free_process_descs;
    free_process_descs = deleted_process;

    return true;
}

bool MemAllocator::set_pool(MallocProcess* target, size_t pool_location) {
    //If pool stacking is attempted, do nothing but take note of it.
    if(target->pool_stack) {
        target->pool_stack+=1;
        return false;
    }

    //Locate the pool, check if it exists.
    MemoryChunk* pool = target->busy_map->find_thischunk(pool_location);
    if(!pool) return false;

    //Setup pooled memory allocation.
    target->pool_location = pool->location;
    target->pool_size = pool->size - (pool_location - pool->location);
    target->pool_stack = 1;

    return true;
}

void MemAllocator::liberate_memory() {
    //Draft !
    panic(PANIC_OUT_OF_MEMORY);
}

MemAllocator::MemAllocator(PhyMemManager& phymem, VirMemManager& virmem) : phymem_manager(&phymem),
                                                                           process_manager(NULL),
                                                                           virmem_manager(&virmem),
                                                                           process_list(NULL),
                                                                           free_mapitems(NULL),
                                                                           free_process_descs(NULL) {
    alloc_mapitems();
    alloc_process_descs();

    //Activate global memory allocation service
    mem_allocator = this;
    phymem_manager->init_malloc();
    virmem_manager->init_malloc();
}

bool MemAllocator::init_process(ProcessManager& procman) {
    //Initialize process management-related functionality in virmem_manager and phymem_manager
    phymem_manager->init_process(procman);
    virmem_manager->init_process(procman);

    //Initialize process management-related functionality
    process_manager = &procman;

    //Setup an insulator associated to MemAllocator
    InsulatorDescriptor mallocator_insulator_desc;
    mallocator_insulator_desc.insulator_name = "MemAllocator";
    //mallocator_insulator_desc.add_process = (void*) mem_allocator_add_process;
    mallocator_insulator_desc.remove_process = (void*) mem_allocator_remove_process;
    //mallocator_insulator_desc.update_process = (void*) mem_allocator_update_process;
    //process_manager->add_insulator(PID_KERNEL, mallocator_insulator_desc);

    return true;
}

size_t MemAllocator::malloc(PID target, const size_t size, const VirMemFlags flags, const bool force) {
    if(!size) return NULL;
    MallocProcess* process;
    size_t result;

    proclist_mutex.grab_spin();

        process = find_or_create_pid(target, force);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        if(process->pool_location) {
            if(process->pool_size < size) {
                result = NULL;
                if(force) panic(PANIC_OUT_OF_MEMORY); //TODO : Make this less brutal
            } else {
                result = process->pool_location;
                process->pool_location+= size;
                process->pool_size-= size;
            }
        } else {
            result = allocator(process, size, flags, force);
        }

    process->mutex.release();

    return result;
}

size_t MemAllocator::malloc_shareable(PID target,
                                      size_t size,
                                      const VirMemFlags flags,
                                      const bool force) {
    MallocProcess* process;
    size_t result;

    proclist_mutex.grab_spin();

        process = find_or_create_pid(target, force);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

        process->mutex.grab_spin();

            if(process->pool_location) {
                if(process->pool_size < size) {
                    result = NULL;
                    if(force) panic(PANIC_OUT_OF_MEMORY); //TODO : Make this less brutal
                } else {
                    result = process->pool_location;
                    process->pool_location+= size;
                    process->pool_size-= size;
                }
            } else {
                result = allocator_shareable(process, size, flags, force);
                if(!result) {
                    //If mapping has failed, we might have created a PID without an address space,
                    //which is a waste of precious memory space. Liberate it.
                    if(!(process->free_map) && !(process->busy_map)) remove_pid(target);
                }
            }

        process->mutex.release();

    proclist_mutex.release();

    return result;
}

bool MemAllocator::free(PID target, const size_t location) {
    MallocProcess* process;
    bool result;

    proclist_mutex.grab_spin();

        process = find_pid(target);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

        process->mutex.grab_spin();

            //Liberate that chunk
            result = liberator(process, location);

        process->mutex.release();

    proclist_mutex.release();

    return result;
}

size_t MemAllocator::share(PID source,
                           const size_t location,
                           PID target,
                           const VirMemFlags flags,
                           const bool force) {
    MallocProcess *source_process, *target_process;
    size_t result = NULL;

    //PID source shares something with PID target
    proclist_mutex.grab_spin();

        source_process = find_pid(source);
        if(!source_process) {
            proclist_mutex.release();
            if(force) panic(PANIC_IMPOSSIBLE_SHARING); //Stub !
            return NULL;
        }
        target_process = find_or_create_pid(target, force);
        if(!target_process) {
            proclist_mutex.release();
            return NULL;
        }

        source_process->mutex.grab_spin(); //To prevent deadlocks, we must always grab mutexes in
        target_process->mutex.grab_spin(); //the same order.

            result = share(source_process, location, target_process, flags, force);

            if(!result) {
                //If mapping has failed, we might have created a PID without an address space,
                //which is a waste of precious memory space. Liberate it.
                if(!(target_process->free_map) && !(target_process->busy_map)) remove_pid(target);
            }

        target_process->mutex.release();
        source_process->mutex.release();

    proclist_mutex.release();

    return result;
}

void MemAllocator::remove_process(PID target) {
    if(target == PID_KERNEL) return; //Find a more constructive way to commit suicide

    remove_pid(target);
}

bool MemAllocator::enter_pool(PID target, size_t pool_location) {
    bool result;

    proclist_mutex.grab_spin();

        MallocProcess* process = find_pid(target);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

        process->mutex.grab_spin();

            result = set_pool(process, pool_location);

        process->mutex.release();

    proclist_mutex.release();

    return result;
}

size_t MemAllocator::leave_pool(PID target) {
    size_t result;

    proclist_mutex.grab_spin();

        MallocProcess* process = find_pid(target);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

        process->mutex.grab_spin();

            if(process->pool_stack > 1) {
                result = NULL;
                process->pool_stack-= 1;
            } else {
                result = process->pool_location;
                process->pool_stack = 0;
                process->pool_location = NULL;
                process->pool_size = NULL;
            }

        process->mutex.release();

    proclist_mutex.release();

    return result;
}

void MemAllocator::print_maplist() {
    proclist_mutex.grab_spin();

        if(!process_list) {
            dbgout << txtcolor(TXT_RED) << "Error : Map list does not exist";
            dbgout << txtcolor(TXT_DEFAULT);
        } else {
            dbgout << *process_list;
        }

    proclist_mutex.release();
}

void MemAllocator::print_busymap(const PID identifier) {
    proclist_mutex.grab_spin();

        MallocProcess* process = find_pid(identifier);
        if(!process) {
            dbgout << txtcolor(TXT_RED) << "Error : PID not registered";
            dbgout << txtcolor(TXT_DEFAULT);
            proclist_mutex.release();
            return;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        if(process->busy_map) {
            dbgout << *(process->busy_map);
        } else {
            dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
            dbgout << txtcolor(TXT_DEFAULT);
        }

    process->mutex.release();
}

void MemAllocator::print_freemap(const PID identifier) {
    proclist_mutex.grab_spin();

        MallocProcess* process = find_pid(identifier);
        if(!process) {
            dbgout << txtcolor(TXT_RED) << "Error : PID not registered";
            dbgout << txtcolor(TXT_DEFAULT);
            proclist_mutex.release();
            return;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        if(process->free_map) {
            dbgout << *(process->free_map);
        } else {
            dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
            dbgout << txtcolor(TXT_DEFAULT);
        }

    process->mutex.release();
}

void* kalloc(PID target, const size_t size, const VirMemFlags flags, const bool force) {
    if(!mem_allocator) {
        if(!force) return NULL;
        //TODO : Something cleaner
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) mem_allocator->malloc(target, size, flags, force);
}

void* kalloc_shareable(PID target, size_t size, const VirMemFlags flags, const bool force) {
    if(!mem_allocator) {
        if(!force) return NULL;
        //TODO : Something cleaner
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) mem_allocator->malloc_shareable(target, size, flags, force);
}

bool kfree(PID target, void* location) {
    if(!mem_allocator) return false;
    return (void*) mem_allocator->free(target, (size_t) location);
}

void* kshare(const PID source,
             const void* location,
             PID target,
             const VirMemFlags flags,
             const bool force) {
    if(!mem_allocator) {
        if(!force) return NULL;
        //TODO : Clean this up
        panic(PANIC_MM_UNINITIALIZED);
    }
    return (void*) mem_allocator->share(source, (size_t) location, target, flags, force);
}

bool kenter_pool(PID target, void* pool) {
    if(!mem_allocator) return false;
    return mem_allocator->enter_pool(target, (size_t) pool);
}

void* kleave_pool(PID target) {
    if(!mem_allocator) return NULL;
    return (void*) mem_allocator->leave_pool(target);
}

/*PID mem_allocator_add_process(PID id, ProcessProperties properties) {
    if(!mem_allocator) {
        return PID_INVALID;
    } else {
        return mem_allocator->add_process(id, properties);
    }
}*/

void mem_allocator_remove_process(PID target) {
    if(!mem_allocator) {
        return;
    } else {
        mem_allocator->remove_process(target);
    }
}

/*PID mem_allocator_update_process(PID old_process, PID new_process) {
    if(!mem_allocator) {
        return PID_INVALID;
    } else {
        return mem_allocator->update_process(old_process, new_process);
    }
}*/
