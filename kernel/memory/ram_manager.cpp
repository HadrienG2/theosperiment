 /*  Arch-independent part of RAM management

    Copyright (C) 2011-2013  Hadrien Grasland

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

#include <new.h>
#include <ram_manager.h>

#include <dbgstream.h>


RAMManager* ram_manager = NULL;

bool RAMManager::alloc_mapitems(RAMChunk* free_mem_override) {
    size_t remaining_freemem = 0;
    RAMChunk *allocated_chunk, *current_item, *free_chunk = NULL;

    //Get some free memory to store mapitems or abort
    if(!free_mem_override) {
        allocated_chunk = free_mem;
        if(!allocated_chunk) return false;
        remaining_freemem = allocated_chunk->size - PG_SIZE;
        allocated_chunk->size = PG_SIZE;
    } else {
        allocated_chunk = free_mem_override;
    }
    allocated_chunk->owners = PID_KERNEL;

    //Store our brand new free memory map items in the allocated mem
    current_item = (RAMChunk*) (allocated_chunk->location);
    for(size_t used_mem = sizeof(RAMChunk); used_mem <= allocated_chunk->size; used_mem+= sizeof(RAMChunk)) {
        current_item = new(current_item) RAMChunk();
        current_item->next_mapitem = current_item+1;
        ++current_item;
    }
    --current_item;
    current_item->next_mapitem = free_mapitems;
    free_mapitems = (RAMChunk*) (allocated_chunk->location);

    //Maybe there is some spare memory left after allocating our chunk ?
    //If so, don't leak it.
    if(remaining_freemem) {
        free_chunk = free_mapitems;
        free_mapitems = free_mapitems->next_mapitem;
        free_chunk->location = allocated_chunk->location+allocated_chunk->size;
        free_chunk->size = remaining_freemem;
        free_chunk->next_mapitem = allocated_chunk->next_mapitem;
        free_chunk->next_buddy = allocated_chunk->next_buddy;
        allocated_chunk->next_mapitem = free_chunk;
    }

    //Update free memory information
    if(!free_mem_override) {
        if(free_chunk) {
            free_mem = free_chunk;
        } else {
            free_mem = allocated_chunk->next_buddy;
        }
        find_process(PID_KERNEL)->memory_usage+= allocated_chunk->size;
    }
    allocated_chunk->next_buddy = NULL;

    return true;
}


bool RAMManager::alloc_pids() {
    size_t remaining_freemem;
    PIDs *current_item;
    RAMChunk *allocated_chunk, *free_chunk = NULL;

    //Get some free memory to store PIDs in or abort
    allocated_chunk = free_mem;
    if(!allocated_chunk) return false;
    remaining_freemem = allocated_chunk->size - PG_SIZE;
    allocated_chunk->size = PG_SIZE;
    allocated_chunk->owners = PID_KERNEL;

    //Store our brand new PIDs in the allocated mem
    current_item = (PIDs*) (allocated_chunk->location);
    for(size_t used_mem = sizeof(PIDs); used_mem <= allocated_chunk->size; used_mem+= sizeof(PIDs)) {
        current_item = new(current_item) PIDs();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    --current_item;
    current_item->next_item = free_pids;
    free_pids = (PIDs*) (allocated_chunk->location);

    //Maybe there is some spare memory after allocating our chunk ?
    //If so, don't forget it.
    if(remaining_freemem) {
        free_chunk = free_mapitems;
        free_mapitems = free_mapitems->next_mapitem;
        free_chunk->location = allocated_chunk->location+PG_SIZE;
        free_chunk->size = remaining_freemem;
        free_chunk->next_mapitem = allocated_chunk->next_mapitem;
        free_chunk->next_buddy = allocated_chunk->next_buddy;
        allocated_chunk->next_mapitem = free_chunk;
    }

    //Update free memory information
    if(free_chunk) {
        free_mem = free_chunk;
    } else {
        free_mem = allocated_chunk->next_buddy;
    }
    find_process(PID_KERNEL)->memory_usage+= allocated_chunk->size;
    allocated_chunk->next_buddy = NULL;

    return true;
}

bool RAMManager::alloc_procitems() {
    size_t remaining_freemem;
    RAMManagerProcess* current_item;
    RAMChunk *allocated_chunk, *free_chunk = NULL;

    //Get some free memory to store process descriptors in or abort
    allocated_chunk = free_mem;
    if(!allocated_chunk) return false;
    remaining_freemem = allocated_chunk->size - PG_SIZE;
    allocated_chunk->size = PG_SIZE;
    allocated_chunk->owners = PID_KERNEL;

    //Store our brand new process descritors in the allocated mem
    current_item = (RAMManagerProcess*) (allocated_chunk->location);
    for(size_t used_mem = sizeof(RAMManagerProcess); used_mem <= allocated_chunk->size; used_mem+= sizeof(RAMManagerProcess)) {
        current_item = new(current_item) RAMManagerProcess();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    --current_item;
    current_item->next_item = free_procitems;
    free_procitems = (RAMManagerProcess*) (allocated_chunk->location);

    //Maybe there is some spare memory after allocating our chunk ?
    //If so, don't forget it.
    if(remaining_freemem) {
        free_chunk = free_mapitems;
        free_mapitems = free_mapitems->next_mapitem;
        free_chunk->location = allocated_chunk->location+PG_SIZE;
        free_chunk->size = remaining_freemem;
        free_chunk->next_mapitem = allocated_chunk->next_mapitem;
        free_chunk->next_buddy = allocated_chunk->next_buddy;
        allocated_chunk->next_mapitem = free_chunk;
    }

    //Update free memory information
    if(free_chunk) {
        free_mem = free_chunk;
    } else {
        free_mem = allocated_chunk->next_buddy;
    }
    find_process(PID_KERNEL)->memory_usage+= allocated_chunk->size;
    allocated_chunk->next_buddy = NULL;

    return true;
}

RAMChunk* RAMManager::chunk_allocator(RAMManagerProcess* owner,
                                      const size_t size,
                                      RAMChunk*& free_mem_used,
                                      bool contiguous) {
    size_t remaining_freemem = 0, to_be_allocd = 0;
    RAMChunk *previous_free_chunk = NULL, *new_free_chunk = NULL;
    RAMChunk *current_chunk, *previous_chunk, *result;
    
    //Check if we can allocate the requested memory without busting caps
    if(owner->memory_usage + size > owner->memory_cap) return NULL;

    //Make sure there's enough space for storing a new memory map item
    if(!free_mapitems) {
        if(!alloc_mapitems()) return NULL; //Memory is full
    }

    //Find a suitable free chunk of memory, store it in current_chunk.
    current_chunk = free_mem_used;
    if(!current_chunk) return NULL;
    if(contiguous) {
        //To enforce contiguity, extra steps are needed. We will noticeably need to know which
        //free memory chunk stood before the one we'll be dealing with, if any.
        current_chunk = current_chunk->find_contigchunk(size);
        if(!current_chunk) return NULL;
        if(current_chunk != free_mem_used) {
            previous_free_chunk = free_mem_used;
            while(previous_free_chunk->next_buddy != current_chunk) {
                previous_free_chunk = previous_free_chunk->next_buddy;
            }
        }
    }

    //Allocate current_chunk. At this point, we may find ourselves in three situations :
    // 1-We allocated too much and must split current_chunk in an allocated part and a free part.
    // 2-We allocated exactly the right amount of memory.
    // 3-We allocated too little memory and need to allocate more.
    current_chunk->owners = owner->identifier;
    result = current_chunk;
    if(current_chunk->size > size) {
        remaining_freemem = current_chunk->size-size;
    } else {
        to_be_allocd = size-current_chunk->size;
    }

    //First, if we allocated too little memory...
    while(to_be_allocd) {
        //...allocate another chunk of memory, abort if memory is full.
        previous_chunk = current_chunk;
        current_chunk = current_chunk->next_buddy;
        if(!current_chunk) {
            chunk_liberator(result);
            return NULL;
        }
        current_chunk->owners = owner->identifier;

        //Update to_be_allocd and remaining_freemem
        if(current_chunk->size < to_be_allocd) {
            to_be_allocd-= current_chunk->size;
        } else {
            remaining_freemem = current_chunk->size-to_be_allocd;
            to_be_allocd = 0;
        }

        //If the newly allocated chunk follows the previous one, merge them together
        if(current_chunk->location == previous_chunk->location+previous_chunk->size) {
            merge_with_next(previous_chunk);
            current_chunk = previous_chunk;
        }
    }

    //At this point, check if we allocated too much memory, and if so correct this
    if(remaining_freemem) {
        current_chunk->size-= remaining_freemem;
        new_free_chunk = free_mapitems;
        free_mapitems = free_mapitems->next_mapitem;
        new_free_chunk->location = current_chunk->location+current_chunk->size;
        new_free_chunk->size = remaining_freemem;
        new_free_chunk->next_mapitem = current_chunk->next_mapitem;
        new_free_chunk->next_buddy = current_chunk->next_buddy;
        current_chunk->next_mapitem = new_free_chunk;
    }

    //Update free memory information
    if(!previous_free_chunk) {
        if(new_free_chunk) {
            free_mem_used = new_free_chunk;
        } else {
            free_mem_used = current_chunk->next_buddy;
        }
    } else {
        if(new_free_chunk) {
            previous_free_chunk->next_buddy = new_free_chunk;
        } else {
            previous_free_chunk->next_buddy = current_chunk->next_buddy;
        }
    }
    current_chunk->next_buddy = NULL;

    //Update process memory usage if allocation has been successful
    owner->memory_usage+= size;

    return result;
}


bool RAMManager::chunk_owneradd(RAMManagerProcess* new_owner, RAMChunk* chunk) {
    RAMChunk* current_item = chunk;
    
    //Check if we can allocate the requested memory without busting caps
    if(new_owner->memory_usage + chunk->size > new_owner->memory_cap) return false;

    while(current_item) {
        //Check if there are some PIDs left in free_pids, attempt to allocate if needed
        if(!free_pids) {
            if(!alloc_pids()) break;
        }

        //Add a new owner to the chunk
        PIDs& chunk_owners = current_item->owners;
        PIDs* old_next_owner = chunk_owners.next_item;
        chunk_owners.next_item = free_pids;
        free_pids = free_pids->next_item;
        chunk_owners.next_item->next_item = old_next_owner;

        //Set up the new owner to an appropriate value
        *(chunk_owners.next_item) = new_owner->identifier;

        //Examine next chunk buddy
        current_item = current_item->next_buddy;
    }

    //If the operation fails, reverts changes.
    if(current_item) {
        chunk_ownerdel(new_owner, chunk);
        return false;
    }
    
    //Update process memory usage if allocation has been successful
    new_owner->memory_usage+= chunk->size;

    return true;
}


bool RAMManager::chunk_ownerdel(RAMManagerProcess* former_owner, RAMChunk* chunk) {
    RAMChunk* current_chunk = chunk;

    while(current_chunk) {
        PIDs& current_owners = current_chunk->owners;
        PIDs* former_pids = NULL;

        //Find out if a PIDs is to be liberated, if yes mark it with the former_pids pointer
        if(current_owners.current_pid == former_owner->identifier) {
            if(!current_owners.next_item) {
                current_owners.current_pid = PID_INVALID;
            } else {
                former_pids = current_owners.next_item;
                if(former_pids) {
                    current_owners.current_pid = former_pids->current_pid;
                    current_owners.next_item = former_pids->next_item;
                }
            }
        } else {
            PIDs* parser = &current_owners;
            while(parser->next_item) {
                if(parser->next_item->current_pid == former_owner->identifier) {
                    former_pids = parser->next_item;
                    parser->next_item = former_pids->next_item;
                    break;
                }
                parser = parser->next_item;
            }
        }

        //Free former_pids, if it exists
        if(former_pids) {
            former_pids = new(former_pids) PIDs;
            former_pids->next_item = free_pids;
            free_pids = former_pids;
        }

        //Go to next item in the buddy list
        current_chunk = current_chunk->next_buddy;
    }
    
    //Update process memory usage
    former_owner->memory_usage-= chunk->size;

    //If chunk has no owners anymore, liberate it
    if(chunk->has_owner(PID_INVALID)) chunk_liberator(chunk);

    return true;
}


void RAMManager::discard_empty_chunks() {
    //This function removes empty chunks from the memory map, keeping buddy groups intact.
    //Run as part of RAMManager initialization, it makes the following assumptions :
    //  1/There is only one group of buddies, made of free map items
    //  2/This group is sorted in chronological order

    RAMChunk *deleted_item, *last_buddy_owner = NULL, *previous_item;

    while(ram_map && (ram_map->size == 0)) {
        //Remove empty chunks at the beginning of the memory map
        deleted_item = ram_map;
        ram_map = ram_map->next_mapitem;

        //Free up the extracted map item.
        deleted_item = new(deleted_item) RAMChunk;
        deleted_item->next_mapitem = free_mapitems;
        free_mapitems = deleted_item;
    }

    previous_item = ram_map;

    while(previous_item->next_mapitem) {
        //Update last_buddy_owner, keeping track of the last known-good chunk that has buddies
        if(previous_item->next_buddy) {
            last_buddy_owner = previous_item;
        }

        //Remove all empty chunks immediately after previous_item
        //RAMChunk*& current_item = previous_item->next_mapitem;
        while((previous_item->next_mapitem) && (previous_item->next_mapitem->size == 0)) {
            //Take current item out of the memory map
            deleted_item = previous_item->next_mapitem;
            previous_item->next_mapitem = previous_item->next_mapitem->next_mapitem;

            //Take care of buddies
            if(previous_item->next_mapitem == last_buddy_owner->next_buddy) {
                last_buddy_owner->next_buddy = previous_item->next_mapitem->next_buddy;
            }

            //Free up the extracted map item
            deleted_item = new(deleted_item) RAMChunk;
            deleted_item->next_mapitem = free_mapitems;
            free_mapitems = deleted_item;
        }

        //Move to next map item
        if(previous_item->next_mapitem) previous_item = previous_item->next_mapitem;
    }
}

void RAMManager::fill_mmap(const KernelInformation& kinfo) {
    size_t index;
    RAMChunk *current_item, *last_free = NULL, *previous_item;

    //Generate the first memory map item
    index = 0;
    ram_map = generate_chunk(kinfo, index);
    previous_item = ram_map;

    //If it is free, add it to the free memory pool
    if(ram_map->has_owner(PID_INVALID) && ram_map->allocatable) {
        if(last_free) {
            last_free->next_buddy = ram_map;
        } else {
            free_lowmem = ram_map;
        }
        last_free = ram_map;
    }

    //Continue to fill the memory map
    while(index < kinfo.kmmap_size) {
        //Create and append a chunk associated to the next kmmap item(s)
        current_item = generate_chunk(kinfo, index);
        fix_overlap(previous_item, current_item);
        previous_item->next_mapitem = current_item;

        //If the new chunk is free, add it to the free memory pool
        if(current_item->has_owner(PID_INVALID) && current_item->allocatable) {
            if(last_free) {
                last_free->next_buddy = current_item;
            } else {
                free_lowmem = current_item;
            }
            last_free = current_item;
        }
        previous_item = previous_item->next_mapitem;
    }
}


RAMManagerProcess* RAMManager::find_process(const PID target) {
    RAMManagerProcess* proc_parser = process_list;
    while(proc_parser) {
        if(proc_parser->identifier == target) break;
        proc_parser = proc_parser->next_item;
    }

    return proc_parser;
}


void RAMManager::fix_overlap(RAMChunk* first_chunk, RAMChunk* second_chunk) {
    //If the two chunks do not overlap, there is no problem : abort.
    if(first_chunk->location + first_chunk->size <= second_chunk->location) return;

    //If one chunk is allocatable and not the other, non-allocatable chunk has priority
    if((first_chunk->allocatable == true) && (second_chunk->allocatable == false)) {
        first_chunk->size = second_chunk->location - first_chunk->location;
        return;
    }
    if((first_chunk->allocatable == false) && (second_chunk->allocatable == true)) {
        second_chunk->location = first_chunk->location + first_chunk->size;
        return;
    }

    //If one chunk is allocated and not the other, allocated chunk has priority
    if((first_chunk->has_owner(PID_INVALID) == true) && (second_chunk->has_owner(PID_INVALID) == false)) {
        first_chunk->size = second_chunk->location - first_chunk->location;
        return;
    }
    if((first_chunk->has_owner(PID_INVALID) == false) && (second_chunk->has_owner(PID_INVALID) == true)) {
        second_chunk->location = first_chunk->location + first_chunk->size;
        return;
    }

    //When everything else has failed, first chunk has priority over second chunk
    second_chunk->location = first_chunk->location + first_chunk->size;
}


RAMChunk* RAMManager::generate_chunk(const KernelInformation& kinfo, size_t& index) {
    //Called during RAMManager initialization, this function generates a RAMChunk from a set
    //of kmmap items, using these rules :
    //  -Starting address of the initial kmmap item is aligned down on a page boundary, ending
    //   address is aligned up, all kmmap items inbetween are considered
    //  -Reserved memory is not allocatable
    //  -Bootstrap and kernel pages are marked as owned by the kernel
    //  -Most restrictive attribute prevails over least restrictive one

    KernelMemoryMap* kmmap = kinfo.kmmap;

    //Allocate chunk
    RAMChunk* result = free_mapitems;
    free_mapitems = free_mapitems->next_mapitem;
    result->next_mapitem = NULL;

    //Determine chunk boundaries
    size_t chunk_start = align_pgdown(kmmap[index].location);
    size_t chunk_end = align_pgup(kmmap[index].location+kmmap[index].size);
    result->location = chunk_start;
    result->size = chunk_end-chunk_start;

    //Examine all kmmap items comprised in the allocated chunk to set chunk attributes
    while((kmmap[index].location < chunk_end) && (index < kinfo.kmmap_size)) {
        if(kmmap[index].nature>=NATURE_BSK) {
            result->owners = PID_KERNEL;
        }
        if(kmmap[index].nature==NATURE_RES) {
            result->allocatable = false;
        }
        ++index;
    }

    //Set index on the next map item to be mapped
    if(kmmap[index-1].location + kmmap[index-1].size > chunk_end) --index;

    return result;
}


bool RAMManager::initialize_process_list() {
    //This function generates the "process list", which at this point only includes a kernel entry
    static RAMManagerProcess kernel_process;    
    RAMChunk* map_parser;
    
    process_list = &kernel_process;

    process_list->identifier = PID_KERNEL;
    map_parser = ram_map;
    while(map_parser) {
        if(map_parser->has_owner(PID_KERNEL)) process_list->memory_usage+= map_parser->size;
        map_parser = map_parser->next_mapitem;
    }
    
    return true;
}


void RAMManager::killer(RAMManagerProcess* target) {
    RAMChunk* parser = ram_map;

    while(parser) {
        if(parser->has_owner(target->identifier)) chunk_ownerdel(target, parser);
        parser = parser->next_mapitem;
    }
}


void RAMManager::merge_with_next(RAMChunk* first_item) {
    RAMChunk* next_item = first_item->next_mapitem;

    //Merging assumes that the first item and his neighbour are really identical.
    first_item->size+= next_item->size;
    first_item->next_mapitem = next_item->next_mapitem;
    first_item->next_buddy = next_item->next_buddy;

    //Once done, trash "next_mapitem" in our free_mapitems reservoir.
    next_item = new(next_item) RAMChunk();
    next_item->next_mapitem = free_mapitems;
    free_mapitems = next_item;
}


void RAMManager::pids_liberator(PIDs& target) {
    PIDs *to_delete = target.next_item, *following_one;

    while(to_delete) {
        following_one = to_delete->next_item;
        to_delete = new(to_delete) PIDs;
        to_delete->next_item = free_pids;
        free_pids = to_delete;
        to_delete = following_one;
    }
    target.next_item = NULL;
}


bool RAMManager::split_chunk(RAMChunk* chunk, const size_t position) {
    //This function splits a free memory chunk in two halves at a specified position

    //Allocate a new memory chunk
    if(!free_mapitems) {
        if(!alloc_mapitems()) return false; //Memory is full
    }
    RAMChunk* new_chunk = free_mapitems;
    free_mapitems = free_mapitems->next_mapitem;

    //Give it the right properties
    new_chunk->location = chunk->location + position;
    new_chunk->size = chunk->size - position;
    new_chunk->next_mapitem = chunk->next_mapitem;
    new_chunk->next_buddy = chunk->next_buddy;

    //Set up the new properties of the old chunk
    chunk->size = position;
    chunk->next_mapitem = new_chunk;
    if(chunk->next_buddy) chunk->next_buddy = new_chunk;

    return true;
}


bool RAMManager::init_process(ProcessManager& procman) {
    //Initialize process management-related functionality here
    process_manager = &procman;

    //Setup an insulator associated to RAMManager
    InsulatorDescriptor ram_manager_insulator_desc;
    ram_manager_insulator_desc.insulator_name = "RAMManager";
    //TODO: ram_manager_insulator_desc.add_process = ram_manager_add_process;
    ram_manager_insulator_desc.remove_process = ram_manager_remove_process;
    //TODO: ram_manager_insulator_desc.update_process = ram_manager_update_process;
    //TODO: process_manager->add_insulator(PID_KERNEL, ram_manager_insulator_desc);

    return true;
}


void RAMManager::remove_process(PID target) {
    RAMManagerProcess* process;

    proclist_mutex.grab_spin();

        //Find the RAMManagerProcess associated to the requested PID
        process = find_process(target);
        if(!process) {
            proclist_mutex.release();
            return;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        mmap_mutex.grab_spin();

            //Kill the process
            killer(process);

        mmap_mutex.release();

    process->mutex.release();
}


RAMChunk* RAMManager::alloc_chunk(const PID owner, const size_t size, bool contiguous) {
    RAMChunk* result;
    RAMManagerProcess* process;

    proclist_mutex.grab_spin();

        //Find the RAMManagerProcess associated to the requested PID
        process = find_process(owner);
        if(!process) {
            proclist_mutex.release();
            return NULL;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        mmap_mutex.grab_spin();

            //Allocate a chunk of memory
            result = chunk_allocator(process,
                                     align_pgup(size),
                                     free_mem,
                                     contiguous);

        mmap_mutex.release();

    process->mutex.release();

    return result;
}


bool RAMManager::share_chunk(const PID new_owner,
                              size_t chunk_beginning) {
    bool result;
    RAMManagerProcess* process;

    proclist_mutex.grab_spin();

        //Find the RAMManagerProcess associated to the requested PID
        process = find_process(new_owner);
        if(!process) {
            proclist_mutex.release();
            return false;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        mmap_mutex.grab_spin();

            //Find the chunk that is to be shared
            RAMChunk* chunk = ram_map->find_thischunk(chunk_beginning);
            if(!chunk) {
                mmap_mutex.release();
                return false;
            }

            //Share the chunk with its new owner
            result = chunk_owneradd(process, chunk);

        mmap_mutex.release();

    process->mutex.release();

    return result;
}

bool RAMManager::free_chunk(const PID former_owner,
                               size_t chunk_beginning) {
    bool result;
    RAMManagerProcess* process;

    proclist_mutex.grab_spin();

        //Find the RAMManagerProcess associated to the requested PID
        process = find_process(former_owner);
        if(!process) {
            proclist_mutex.release();
            return false;
        }

    process->mutex.grab_spin();
    proclist_mutex.release();

        mmap_mutex.grab_spin();

            //Find the chunk that is to be shared
            RAMChunk* chunk = ram_map->find_thischunk(chunk_beginning);
            if(!chunk) {
                mmap_mutex.release();
                return false;
            }

            //Free the chunk
            result = chunk_ownerdel(process, chunk);

        mmap_mutex.release();

    process->mutex.release();

    return result;
}


void RAMManager::print_mmap() {
    mmap_mutex.grab_spin();

        dbgout << *ram_map;

    mmap_mutex.release();
}

void RAMManager::print_proclist() {
    proclist_mutex.grab_spin();
 
        dbgout << *process_list;   
       
    proclist_mutex.release();
}

/*PID ram_manager_add_process(PID id, ProcessProperties properties) {
    if(!ram_manager) {
        return PID_INVALID;
    } else {
        return ram_manager->add_process(id, properties);
    }
}*/

void ram_manager_remove_process(PID target) {
    if(!ram_manager) {
        return;
    } else {
        ram_manager->remove_process(target);
    }
}

/*PID ram_manager_update_process(PID old_process, PID new_process) {
    if(!ram_manager) {
        return PID_INVALID;
    } else {
        return ram_manager->update_process(old_process, new_process);
    }
}*/
