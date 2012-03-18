 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

    Copyright (C) 2012  Hadrien Grasland

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
#include <virmem_manager.h>

#include <dbgstream.h>

bool VirMemManager::alloc_mapitems() {
    size_t used_space;
    PhyMemChunk* allocated_page;
    VirMemChunk* current_item;

    //Allocate a page of memory
    allocated_page = phymem_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_page) return false;

    //Fill it with initialized map items
    current_item = (VirMemChunk*) (allocated_page->location);
    for(used_space = sizeof(VirMemChunk); used_space < PG_SIZE; used_space+=sizeof(VirMemChunk)) {
        current_item = new(current_item) VirMemChunk;
        current_item->next_buddy = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) VirMemChunk;
    current_item->next_buddy = free_mapitems;
    free_mapitems = (VirMemChunk*) (allocated_page->location);

    //All good !
    return true;
}

bool VirMemManager::alloc_process_descs() {
    size_t used_space;
    PhyMemChunk* allocated_page;
    VirMemProcess* current_item;

    //Allocate a page of memory
    allocated_page = phymem_manager->alloc_chunk(PID_KERNEL);
    if(!allocated_page) return false;

    //Fill it with initialized list items
    current_item = (VirMemProcess*) (allocated_page->location);
    for(used_space = sizeof(VirMemProcess); used_space < PG_SIZE; used_space+=sizeof(VirMemProcess)) {
        current_item = new(current_item) VirMemProcess();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) VirMemProcess();
    current_item->next_item = free_process_descs;
    free_process_descs = (VirMemProcess*) (allocated_page->location);

    //All good !
    return true;
}

VirMemChunk* VirMemManager::alloc_virtual_address_space(VirMemProcess* target,
                                                        size_t location,
                                                        size_t size) {
    VirMemChunk *result, *map_parser = NULL, *last_item;

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

    //Put the chunk in the map
    if(!location) {
        //Find a location where the chunk should be put
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
                    result = new(result) VirMemChunk();
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
                    result = new(result) VirMemChunk();
                    result->next_buddy = free_mapitems;
                    free_mapitems = result;
                    return NULL;
                }
                //Check collisions with the item after it, if there's any
                if(map_parser) {
                    if(map_parser->location < location + size) {
                        //Required location is not available
                        result = new(result) VirMemChunk();
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

VirMemProcess* VirMemManager::find_or_create_pid(PID target) {
    VirMemProcess* list_item;

    list_item = map_list;
    while(list_item->next_item) {
        if(list_item->owner == target) break;
        list_item = list_item->next_item;
    }
    if(list_item->owner != target) {
        list_item->next_item = setup_pid(target);
        list_item = list_item->next_item;
    }

    return list_item;
}

VirMemProcess* VirMemManager::find_pid(const PID target) {
    VirMemProcess* list_item;

    list_item = map_list;
    do {
        if(list_item->owner == target) break;
        list_item = list_item->next_item;
    } while(list_item);

    return list_item;
}

VirMemChunk* VirMemManager::map_chunk(const PID target,
                                      const PhyMemChunk* phys_chunk,
                                      const VirMemFlags flags) {
    VirMemProcess* list_item;
    VirMemChunk* result;
    size_t location = NULL;

    maplist_mutex.grab_spin();

        list_item = find_or_create_pid(target);
        if(!list_item) {
            maplist_mutex.release();
            return NULL;
        }

        list_item->mutex.grab_spin();

            //Map that chunk
            result = chunk_mapper(list_item, phys_chunk, flags, location);

            if(!result) {
                //If mapping has failed, we might have created a PID without an address space, which is
                //a waste of precious memory space. Liberate it.
                if(list_item->map_pointer == NULL) {
                    maplist_mutex.grab_spin();
                    remove_pid(target);
                }
            }

        list_item->mutex.release();

    maplist_mutex.release();

    return result;
}

bool VirMemManager::free_chunk(const PID target, size_t chunk_beginning) {
    bool result;
    VirMemProcess* chunk_owner;
    VirMemChunk* chunk;

    maplist_mutex.grab_spin();

        chunk_owner = find_pid(target);

        chunk_owner->mutex.grab_spin();

            chunk = chunk_owner->map_pointer->find_thischunk(chunk_beginning);
            result = chunk_liberator(chunk_owner, chunk); //Free that chunk

        if(chunk_owner->pml4t_location) chunk_owner->mutex.release();

    maplist_mutex.release();

    return result;
}

VirMemChunk* VirMemManager::adjust_chunk_flags(const PID target,
                                               size_t chunk_beginning,
                                               const VirMemFlags flags,
                                               const VirMemFlags mask) {
    VirMemProcess* chunk_owner;
    VirMemChunk *chunk, *result;

    maplist_mutex.grab_spin();

        chunk_owner = find_pid(target);

    chunk_owner->mutex.grab_spin();
    maplist_mutex.release();

        chunk = chunk_owner->map_pointer->find_thischunk(chunk_beginning);
        result = flag_adjust(chunk_owner, chunk, flags, mask); //Adjust these flags

    chunk_owner->mutex.release();

    return result;
}

void VirMemManager::remove_process(PID target) {
    maplist_mutex.grab_spin();

        remove_pid(target);

    maplist_mutex.release();
}

void VirMemManager::print_maplist() {
    maplist_mutex.grab_spin();

        dbgout << *map_list;

    maplist_mutex.release();
}

void VirMemManager::print_mmap(PID owner) {
    VirMemProcess* list_item;

    maplist_mutex.grab_spin();

        list_item = find_pid(owner);
        if(!list_item || !(list_item->map_pointer)) {
            dbgout << txtcolor(TXT_RED) << "Error : Map does not exist";
            dbgout << txtcolor(TXT_LIGHTGRAY);
        } else {
            list_item->mutex.grab_spin();
        }

    maplist_mutex.release();

    if(list_item && list_item->map_pointer) {
        dbgout << *(list_item->map_pointer);
        list_item->mutex.release();
    }
}
