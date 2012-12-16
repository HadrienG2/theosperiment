 /* Arch-specific part of RAM management

    Copyright (C) 2010-2013    Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#include <new.h>
#include <ram_manager.h>

#include <dbgstream.h>


bool RAMManager::chunk_liberator(RAMChunk* chunk) {
    RAMChunk *current_chunk, *next_chunk = chunk;

    do {
        //Keep track of the next memory map item in the chunk
        current_chunk = next_chunk;
        next_chunk = current_chunk->next_buddy;

        //Free current item from its owners and buddies. For non-allocatable items, that's all.
        pids_liberator(current_chunk->owners);
        current_chunk->next_buddy = NULL;
        if(!(current_chunk->allocatable)) continue;

        //For allocatable items, find out which region of memory they belong to...
        RAMChunk** current_free_mem_ptr;
        if(current_chunk->location >= 0x100000) {
            current_free_mem_ptr = &free_mem;
        } else {
            current_free_mem_ptr = &free_lowmem;
        }
        RAMChunk*& current_free_mem = *current_free_mem_ptr;

        //...then add them to the appropriate list of free chunks, at the right place.
        if(current_free_mem == NULL) {
            current_free_mem = current_chunk;
            continue;
        }
        if(current_free_mem->location > current_chunk->location) {
            current_chunk->next_buddy = current_free_mem;
            current_free_mem = current_chunk;
        } else {
            RAMChunk* previous_free_chunk = current_free_mem;
            while(previous_free_chunk->next_buddy) {
                if(previous_free_chunk->next_buddy->location > current_chunk->location) break;
                previous_free_chunk = previous_free_chunk->next_buddy;
            }
            current_chunk->next_buddy = previous_free_chunk->next_buddy;
            previous_free_chunk->next_buddy = current_chunk;
        }
    } while(next_chunk);

    return true;
}

RAMManager::RAMManager(const KernelInformation& kinfo) : process_manager(NULL),
                                                          ram_map(NULL),
                                                          highmem_map(NULL),
                                                          free_lowmem(NULL),
                                                          free_mem(NULL),
                                                          process_list(NULL),
                                                          free_mapitems(NULL),
                                                          free_pids(NULL),
                                                          free_procitems(NULL) {
    //This function...
    //  1/Determines the amount of memory necessary to store the management structures
    //  2/Find this amount of free space in the memory map
    //  3/Fill those stuctures, including with themselves, using the following rules
    //      -Mark reserved memory as non-allocatable
    //      -Pages of nature Bootstrap and Kernel belong to the kernel
    //      -Pages of nature Free and Reserved belong to nobody (PID_INVALID)

    size_t mapitems_location, mapitems_size, storage_index;
    const KernelMemoryMap* kmmap = kinfo.kmmap;
    RAMChunk *current_item, *last_free = NULL;

    //Find out how much map items we will need, at most, to store our memory map items
    mapitems_size = align_pgup((kinfo.kmmap_size+2)*sizeof(RAMChunk));

    //Find an empty chunk of high memory large enough to store our mess... We assume there's one.
    for(storage_index=0; storage_index<kinfo.kmmap_size; ++storage_index) {
        if(kmmap[storage_index].location < 0x100000) continue;
        if(kmmap[storage_index].nature != NATURE_FRE) continue;
        if(kmmap[storage_index].location+kmmap[storage_index].size-align_pgup(kmmap[storage_index].location) >= mapitems_size) {
            break;
        }
    }
    mapitems_location = align_pgup(kmmap[storage_index].location);

    //Allocate map items in this space
    RAMChunk mapitems_storage;
    mapitems_storage.location = mapitems_location;
    mapitems_storage.size = mapitems_size;
    alloc_mapitems(&mapitems_storage);

    //Fill the memory map using information from the kinfo structure
    fill_mmap(kinfo);

    //Remove empty map items
    discard_empty_chunks();

    //Locate the beginning of high memory
    current_item = ram_map;
    last_free = NULL;
    while(current_item->location < 0x100000) {
        if(current_item->has_owner(PID_INVALID) && current_item->allocatable) {
            last_free = current_item;
        }
        current_item = current_item->next_mapitem;
    }
    free_mem = last_free->next_buddy;
    last_free->next_buddy = NULL;
    highmem_map = current_item;

    //Find the region where we have allocated our map items...
    last_free = NULL;
    while(current_item) {
        if(current_item->location+current_item->size > mapitems_location) break;
        if(current_item->has_owner(PID_INVALID) && current_item->allocatable) {
            last_free = current_item;
        }
        current_item = current_item->next_mapitem;
    }

    //...to mark it as allocated (taking into account free space before and after)
    if(current_item->location < mapitems_location) {
        split_chunk(current_item, mapitems_location-current_item->location);
        last_free = current_item;
        current_item = current_item->next_mapitem;
    }
    RAMChunk* new_free;
    if(current_item->size > mapitems_size) {
        split_chunk(current_item, mapitems_size);
        new_free = current_item->next_mapitem;
    } else {
        new_free = current_item->next_buddy;
    }
    if(last_free) {
        last_free->next_buddy = new_free;
    } else {
        free_mem = new_free;
    }
    current_item->owners = PID_KERNEL;
    current_item->next_buddy = NULL;
    
    //Startup process management services
    initialize_process_list();

    //Allocate extra map items and PIDs right away
    alloc_mapitems();
    alloc_pids();
    alloc_procitems();

    //Activate global RAM memory management service
    ram_manager = this;
}


RAMChunk* RAMManager::alloc_lowchunk(const PID initial_owner,
                                     const size_t size,
                                     bool contiguous) {
    RAMChunk *result, *lowmem_end = ram_map;
    RAMManagerProcess* process;

    proclist_mutex.grab_spin();

        //Find the RAMManagerProcess associated to the requested PID
        process = find_process(initial_owner);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        mmap_mutex.grab_spin();

            //Find the end of low memory
            while(lowmem_end->next_mapitem!=highmem_map) lowmem_end = lowmem_end->next_mapitem;

            //Temporarily make high memory disappear
            lowmem_end->next_mapitem = NULL;

            //Do the allocation job
            result = chunk_allocator(process,
                                     align_pgup(size),
                                     free_lowmem,
                                     contiguous);

            //Make high memory come back
            lowmem_end->next_mapitem = highmem_map;

        mmap_mutex.release();

    process->mutex.release();

    return result;
}

void RAMManager::print_highmmap() {
    mmap_mutex.grab_spin();

        dbgout << *highmem_map;

    mmap_mutex.release();
}

void RAMManager::print_lowmmap() {
    RAMChunk *lowmem_end = ram_map;

    mmap_mutex.grab_spin();

        //Find the end of low memory, temporary make high memory disappear
        while(lowmem_end->next_mapitem!=highmem_map) lowmem_end = lowmem_end->next_mapitem;
        lowmem_end->next_mapitem = NULL;

        dbgout << *ram_map;

        //Make high memory come back
        lowmem_end->next_mapitem = highmem_map;

    mmap_mutex.release();
}
