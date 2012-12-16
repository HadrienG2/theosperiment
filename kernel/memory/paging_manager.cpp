 /* Paging management classes

    Copyright (C) 2012-2013  Hadrien Grasland

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
#include <paging_manager.h>

#include <dbgstream.h>

bool PagingManager::alloc_mapitems() {
    RAMChunk* allocated_chunk;
    PageChunk* current_item;

    //Allocate a chunk of memory
    allocated_chunk = ram_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized map items
    current_item = (PageChunk*) (allocated_chunk->location);
    for(size_t used_mem = sizeof(PageChunk); used_mem <= allocated_chunk->size; used_mem+= sizeof(PageChunk)) {
        current_item = new(current_item) PageChunk;
        current_item->next_buddy = current_item+1;
        ++current_item;
    }
    --current_item;
    current_item->next_buddy = free_mapitems;
    free_mapitems = (PageChunk*) (allocated_chunk->location);

    //All good !
    return true;
}

bool PagingManager::alloc_process_descs() {
    RAMChunk* allocated_chunk;
    PagingManagerProcess* current_item;

    //Allocate a page of memory
    allocated_chunk = ram_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized list items
    current_item = (PagingManagerProcess*) (allocated_chunk->location);
    for(size_t used_mem = sizeof(PagingManagerProcess); used_mem <= allocated_chunk->size; used_mem+= sizeof(PagingManagerProcess)) {
        current_item = new(current_item) PagingManagerProcess();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    --current_item;
    current_item->next_item = free_process_descs;
    free_process_descs = (PagingManagerProcess*) (allocated_chunk->location);

    //All good !
    return true;
}

PageChunk* PagingManager::alloc_virtual_address_space(PagingManagerProcess* target,
                                                        size_t size,
                                                        size_t location) {
    PageChunk *result, *map_parser = NULL, *last_item;

    //Allocate the new memory map item
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) return NULL;
    }
    result = free_mapitems;
    free_mapitems = free_mapitems->next_buddy;
    result->next_buddy = NULL;

    //Fill part of its contents
    result->location = location;
    result->size = size;

    //Put the chunk in the map
    if(!location) {
        //Find the first location where the chunk could be put
        if(target->map_pointer == NULL) {
            result->location = PG_SIZE; //First page includes the NULL pointer. Not to be used.
            target->map_pointer = result;
        } else {
            if(target->map_pointer->location >= size+PG_SIZE) {
                result->location = target->map_pointer->location-size;
                result->next_mapitem = target->map_pointer;
                target->map_pointer = result;
            } else {
                last_item = target->map_pointer;
                map_parser = target->map_pointer->next_mapitem;
                while(map_parser) {
                    if(map_parser->location-(last_item->location+last_item->size) >= size) {
                        result->location = map_parser->location-size;
                        result->next_mapitem = map_parser;
                        last_item->next_mapitem = result;
                        break;
                    }
                    last_item = map_parser;
                    map_parser = map_parser->next_mapitem;
                }
                if(!map_parser) {
                    result->location = last_item->location + last_item->size;
                    last_item->next_mapitem = result;
                }
            }
        }
    } else {
        //We know where we want to put our chunk.
        //Just find the place in the map and check that there's nothing there.
        if(target->map_pointer == NULL) {
            //Chunk should be inserted as the first item of the map
            target->map_pointer = result;
        } else {
            if(target->map_pointer->location >= location) {
                //Chunk should be inserted before the first item of the map.
                //Check collisions with it
                if(target->map_pointer->location < location+size) {
                    //Required location is not available
                    result = new(result) PageChunk();
                    result->next_buddy = free_mapitems;
                    free_mapitems = result;
                    return NULL;
                }
                result->next_mapitem = target->map_pointer;
                target->map_pointer = result;
            } else {
                last_item = target->map_pointer;
                map_parser = last_item->next_mapitem;
                while(map_parser) {
                    if(map_parser->location >= location) break;
                    last_item = map_parser;
                    map_parser = map_parser->next_mapitem;
                }
                //At this point, we've found where the chunk should be inserted.
                //Check collisions with the item before it
                if(last_item->location+last_item->size > location) {
                    //Required location is not available
                    result = new(result) PageChunk();
                    result->next_buddy = free_mapitems;
                    free_mapitems = result;
                    return NULL;
                }
                //Check collisions with the item after it, if any
                if(map_parser) {
                    if(map_parser->location < location + size) {
                        //Required location is not available
                        result = new(result) PageChunk();
                        result->next_buddy = free_mapitems;
                        free_mapitems = result;
                        return NULL;
                    }
                    result->next_mapitem = map_parser;
                    last_item->next_mapitem = result;
                } else {
                    last_item->next_mapitem = result;
                }
            }
        }
    }

    return result;
}

PagingManagerProcess* PagingManager::find_or_create_pid(PID target) {
    PagingManagerProcess* list_item;

    list_item = process_list;
    while(list_item->next_item) {
        if(list_item->identifier == target) {
            break;
        }
        list_item = list_item->next_item;
    }
    if(list_item->identifier != target) {
        list_item->next_item = setup_pid(target);
        list_item = list_item->next_item;
    }

    return list_item;
}

PagingManagerProcess* PagingManager::find_pid(const PID target) {
    PagingManagerProcess* list_item;

    list_item = process_list;
    do {
        if(list_item->identifier == target) break;
        list_item = list_item->next_item;
    } while(list_item);

    return list_item;
}

bool PagingManager::map_k_chunks(PagingManagerProcess* target) {
    PagingManagerProcess* kernel = process_list; //Kernel is the first item of the process list.
    PageChunk *kernel_map_parser = kernel->map_pointer, *result;

    while(kernel_map_parser) {
        if(kernel_map_parser->flags & PAGE_FLAG_K) {
            result = chunk_mapper(target,
                                  kernel_map_parser->points_to,
                                  kernel_map_parser->flags,
                                  kernel_map_parser->location);
            if(!result) return false;
        }

        kernel_map_parser = kernel_map_parser->next_mapitem;
    }

    return true;
}

bool PagingManager::init_malloc() {
    //For now, there are no malloc-based features in PagingManager, so we just set a flag on.
    malloc_active = true;

    return true;
}

bool PagingManager::init_process(ProcessManager& procman) {
    //Initialize process management-related functionality
    process_manager = &procman;

    //Setup an insulator associated to PagingManager
    InsulatorDescriptor paging_manager_insulator_desc;
    paging_manager_insulator_desc.insulator_name = "PagingManager";
    //paging_manager_insulator_desc.add_process = (void*) paging_manager_add_process;
    paging_manager_insulator_desc.remove_process = (void*) paging_manager_remove_process;
    //paging_manager_insulator_desc.update_process = (void*) paging_manager_update_process;
    //process_manager->add_insulator(PID_KERNEL, paging_manager_insulator_desc);

    return true;
}

PageChunk* PagingManager::map_chunk(const PID target,
                                    const RAMChunk* ram_chunk,
                                    const PageFlags flags) {
    PagingManagerProcess* list_item;
    PageChunk* result;
    size_t location = NULL;
    if(target == PID_KERNEL) location = ram_chunk->location; //Kernel chunks are identity-mapped

    proclist_mutex.grab_spin();

        list_item = find_or_create_pid(target);
        if(!list_item) {
            proclist_mutex.release();
            return NULL;
        }

        list_item->mutex.grab_spin();

            //Map that chunk
            result = chunk_mapper(list_item, ram_chunk, flags, location);

            if(!result) {
                //If mapping has failed, we might have created a PID without an address space, which is
                //a waste of precious memory space. Liberate it.
                if(list_item->map_pointer == NULL) {
                    proclist_mutex.grab_spin();
                    remove_pid(target);
                }
            }

        list_item->mutex.release();

    proclist_mutex.release();

    return result;
}

bool PagingManager::free_chunk(const PID target, size_t chunk_beginning) {
    bool result;
    PagingManagerProcess* chunk_owner;
    PageChunk* chunk;

    proclist_mutex.grab_spin();

        chunk_owner = find_pid(target);

        chunk_owner->mutex.grab_spin();

            chunk = chunk_owner->map_pointer->find_thischunk(chunk_beginning);
            result = chunk_liberator(chunk_owner, chunk); //Free that chunk

        if(chunk_owner->pml4t_location) chunk_owner->mutex.release();

    proclist_mutex.release();

    return result;
}

PageChunk* PagingManager::adjust_chunk_flags(const PID target,
                                               size_t chunk_beginning,
                                               const PageFlags flags,
                                               const PageFlags mask) {
    PagingManagerProcess* chunk_owner;
    PageChunk *chunk, *result;

    proclist_mutex.grab_spin();

        chunk_owner = find_pid(target);

    chunk_owner->mutex.grab_spin();
    proclist_mutex.release();

        chunk = chunk_owner->map_pointer->find_thischunk(chunk_beginning);
        result = flag_adjust(chunk_owner, chunk, flags, mask); //Adjust these flags

    chunk_owner->mutex.release();

    return result;
}

void PagingManager::remove_process(PID target) {
    if(target == PID_KERNEL) return; //Find a more constructive way to commit suicide.

    proclist_mutex.grab_spin();

        remove_pid(target);

    proclist_mutex.release();
}

void PagingManager::print_maplist() {
    proclist_mutex.grab_spin();

        dbgout << *process_list;

    proclist_mutex.release();
}

void PagingManager::print_mmap(PID owner) {
    PagingManagerProcess* list_item;

    proclist_mutex.grab_spin();

        list_item = find_pid(owner);
        if(!list_item || !(list_item->map_pointer)) {
            dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
            dbgout << txtcolor(TXT_DEFAULT);
        } else {
            list_item->mutex.grab_spin();
        }

    proclist_mutex.release();

    if(list_item && list_item->map_pointer) {
        dbgout << *(list_item->map_pointer);
        list_item->mutex.release();
    }
}
