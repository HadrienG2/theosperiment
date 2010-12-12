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

    //Stub !
    return NULL;
}

addr_t MemAllocator::liberator(addr_t location, MallocPIDList* target) {
    //How it works :
    //  1.Find the relevant item in busy_map. If it fails, return NULL.
    //  2.In the way, check if that item is the sole item belonging to its chunk of virtual memory
    //    in busy_map (examining the neighbours should be sufficient, since busy_map is sorted).
    // 3a.If so, liberate the chunk and remove any item of free_map belonging to it. If this results
    //    in busy_map being empty, free_map is necessarily empty too : remove that PID.
    // 3b.If not, move the busy_map item in free_map.

    //Stub !
    return NULL;
}

addr_t MemAllocator::knl_allocator(addr_t size) {
    //This works a lot like allocator(), except that it uses knl_free_map and knl_busy_map and that
    //since paging is disabled for the kernel, the chunk is allocated through
    //PhyMemManager::alloc_contigchunk, and not mapped through virmem.

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
        maplist_mutex.release();
            
            //Liberate that chunk
            result = liberator(location, list_item);
        
        list_item->mutex.release();
    }
    
    return result;
}