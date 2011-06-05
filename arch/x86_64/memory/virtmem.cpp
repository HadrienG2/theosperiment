 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

    Copyright (C) 2010-2011    Hadrien Grasland

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

#include <align.h>
#include <kmaths.h>
#include <virtmem.h>
#include <x86paging.h>
#include <x86paging_parser.h>
#include <dbgstream.h>
#include <display_paging.h>

bool VirMemManager::alloc_mapitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    VirMemMap* current_item;

    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized map items
    free_mapitems = (VirMemMap*) (allocated_chunk->location);
    current_item = free_mapitems;
    for(used_space = sizeof(VirMemMap); used_space < PG_SIZE; used_space+=sizeof(VirMemMap)) {
        *current_item = VirMemMap();
        current_item->next_buddy = current_item+1;
        ++current_item;
    }
    *current_item = VirMemMap();
    current_item->next_buddy = NULL;

    //All good !
    return true;
}

bool VirMemManager::alloc_listitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    VirMapList* current_item;

    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return false;

    //Fill it with initialized list items
    free_listitems = (VirMapList*) (allocated_chunk->location);
    current_item = free_listitems;
    for(used_space = sizeof(VirMapList); used_space < PG_SIZE; used_space+=sizeof(VirMapList)) {
        *current_item = VirMapList();
        current_item->next_item = current_item+1;
        ++current_item;
    }
    *current_item = VirMapList();
    current_item->next_item = NULL;

    //All good !
    return true;
}

VirMemMap* VirMemManager::chunk_mapper(const PhyMemMap* phys_chunk,
                                       const VirMemFlags flags,
                                       VirMapList* target,
                                       addr_t location) {
    //We have two goals here :
    // -To map that chunk of physical memory in the virtual memory map of the target.
    // -To put it in its virtual address space, too.
    //The location parameter is optional. By default, it is NULL, meaning that the chunk may be
    //mapped at any location. If it is not NULL, it means that the chunk must be mapped at this
    //precise location, and that allocation will fail if that is not doable.

    addr_t total_size, offset, tmp;
    VirMemMap *result, *map_parser = NULL, *last_item;
    PhyMemMap *chunk_parser;

    //Allocate the new memory map item
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) return NULL;
    }
    result = free_mapitems;
    free_mapitems = free_mapitems->next_buddy;
    result->location = location;
    result->flags = flags;
    result->owner = target;
    result->points_to = (PhyMemMap*) phys_chunk;
    result->next_buddy = NULL;

    //Find virtual chunk size
    chunk_parser = (PhyMemMap*) phys_chunk;
    total_size = 0;
    while(chunk_parser) {
        total_size+= chunk_parser->size;
        chunk_parser = chunk_parser->next_buddy;
    }
    result->size = total_size;

    //Put the chunk in the map
    if(!location) {
        //Find a location where the chunk should be put
        if(target->map_pointer == NULL) {
            result->location = PG_SIZE; //First page includes the NULL pointer. Not to be used.
            target->map_pointer = result;
        } else {
            if(target->map_pointer->location >= total_size+PG_SIZE) {
                result->location = target->map_pointer->location-total_size;
                result->next_mapitem = target->map_pointer;
                target->map_pointer = result;
            } else {
                last_item = target->map_pointer;
                map_parser = target->map_pointer->next_mapitem;
                while(map_parser) {
                    if(map_parser->location-last_item->location-last_item->size >= total_size) {
                        result->location = map_parser->location-total_size;
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
                if(target->map_pointer->location < location+total_size) {
                    //Required location is not available
                    *result = VirMemMap();
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
                    *result = VirMemMap();
                    result->next_buddy = free_mapitems;
                    free_mapitems = result;
                    return NULL;
                }
                //Check collisions with the item after it, if there's any
                if(map_parser) {
                    if(map_parser->location < location + total_size) {
                        //Required location is not available
                        *result = VirMemMap();
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

    //Allocate paging structures
    //If allocation fails, don't forget to restore the map into a clean state before returning NULL.
    tmp = x86paging::setup_4kpages(result->location, result->size, target->pml4t_location, phymem);
    if(!tmp) {
        chunk_liberator(result);
        return NULL;
    }

    //Fill those structures we just allocated
    chunk_parser = (PhyMemMap*) phys_chunk;
    offset = 0;
    while(chunk_parser) {
        x86paging::fill_4kpaging(chunk_parser->location,
                                 result->location+offset,
                                 chunk_parser->size,
                                 x86flags(result->flags),
                                 target->pml4t_location);
        offset+= chunk_parser->size;
        chunk_parser = chunk_parser->next_buddy;
    }

    return result;
}

bool VirMemManager::chunk_liberator(VirMemMap* chunk) {
    VirMemMap *current_mapitem, *current_item = chunk, *next_item;
    VirMapList* target = chunk->owner;

    while(current_item) {
        next_item = current_item->next_buddy;

        //Remove item from the map
        if(target->map_pointer == current_item) {
            target->map_pointer = current_item->next_mapitem;
        } else {
            current_mapitem = target->map_pointer;
            while(current_mapitem->next_mapitem) {
                if(current_mapitem->next_mapitem == current_item) break;
                current_mapitem = current_mapitem->next_mapitem;
            }
            if(current_mapitem->next_mapitem) {
                current_mapitem->next_mapitem = current_mapitem->next_mapitem->next_mapitem;
            }
        }

        //Manage impact on paging structures : delete page table entries, liberate unused paging
        //structures...
        x86paging::remove_paging(current_item->location,
                                 current_item->size,
                                 target->pml4t_location,
                                 phymem);

        //Remove the rest
        *current_item = VirMemMap();
        current_item->next_buddy = free_mapitems;
        free_mapitems = current_item;
        current_item = next_item;
    }

    //Check if this PID's address space is empty, if so delete it.
    if(target->map_pointer == NULL) remove_pid(target->map_owner);

    return true;
}

VirMapList* VirMemManager::find_or_create_pid(PID target) {
    VirMapList* list_item;

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

VirMapList* VirMemManager::find_pid(const PID target) {
    VirMapList* list_item;

    list_item = map_list;
    do {
        if(list_item->map_owner == target) break;
        list_item = list_item->next_item;
    } while(list_item);

    return list_item;
}

VirMapList* VirMemManager::setup_pid(PID target) {
    VirMapList* result;
    PhyMemMap* pml4t_chunk;

    //Allocate management structures
    if(!free_listitems) {
        alloc_listitems();
        if(!free_listitems) return NULL;
    }
    pml4t_chunk = phymem->alloc_page(PID_KERNEL);
    if(!pml4t_chunk) return NULL;

    //Fill them
    result = free_listitems;
    free_listitems = free_listitems->next_item;
    result->map_owner = target;
    result->pml4t_location = pml4t_chunk->location;
    x86paging::create_pml4t(result->pml4t_location);
    result->next_item = NULL;

    //Map the kernel in the process' user space
    VirMemMap* tmp;
    tmp = chunk_mapper(phy_knl_rx,
                       VMEM_FLAGS_RX + VMEM_FLAG_K,
                       result,
                       knl_rx_loc);
    if(tmp) tmp = chunk_mapper(phy_knl_r,
                               VMEM_FLAG_R + VMEM_FLAG_K,
                               result,
                               knl_r_loc);
    if(tmp) tmp = chunk_mapper(phy_knl_rw,
                               VMEM_FLAGS_RW + VMEM_FLAG_K,
                               result,
                               knl_rw_loc);
    if(!tmp) {
        result->next_item = map_list->next_item;
        map_list->next_item = result;
        remove_pid(result->map_owner);
        return NULL;
    }

    return result;
}

bool VirMemManager::remove_pid(PID target) {
    VirMapList *deleted_item, *previous_item;

    //target can't be the first item of the map list, as this item is the kernel.
    //Paging is disabled for the kernel
    previous_item = map_list;
    while(previous_item->next_item) {
        if(previous_item->next_item->map_owner == target) break;
        previous_item = previous_item->next_item;
    }
    deleted_item = previous_item->next_item;
    if(!deleted_item) return false;
    previous_item->next_item = deleted_item->next_item;

    //Free all its paging structures and its entry
    while(deleted_item->map_pointer) chunk_liberator(deleted_item->map_pointer);
    remove_all_paging(deleted_item);
    phymem->free(deleted_item->pml4t_location);
    *deleted_item = VirMapList();
    deleted_item->next_item = free_listitems;
    free_listitems = deleted_item;

    return true;
}

bool VirMemManager::flag_adjust(VirMemMap* chunk,
                                const VirMemFlags flags,
                                const VirMemFlags mask,
                                VirMapList* target) {
    VirMemMap* current_item;

    //Adjust flags of the chunk itself
    chunk->flags = (flags & mask)+((chunk->flags) & (~mask));

    //Adjust those flags in paging structures, too.
    current_item = chunk;
    do {
        x86paging::set_flags(current_item->location,
                             current_item->size,
                             x86flags(current_item->flags),
                             target->pml4t_location);
        current_item = current_item->next_buddy;
    } while(current_item);

    return true;
}

bool VirMemManager::remove_all_paging(VirMapList* target) {
    using namespace x86paging;

    addr_t total_address_space = PG_SIZE*PTABLE_LENGTH; //Size of a page table
    total_address_space*= PTABLE_LENGTH; //Size of a PD
    total_address_space*= PTABLE_LENGTH; //Size of a PDPT
    total_address_space*= PTABLE_LENGTH; //Size of the whole PML4T
    return remove_paging(0, total_address_space, target->pml4t_location, phymem);
}

uint64_t VirMemManager::x86flags(VirMemFlags flags) {
    using namespace x86paging;

    uint64_t result = PBIT_PRESENT + PBIT_USERACCESS + PBIT_NOEXECUTE;

    if(flags & VMEM_FLAG_A) result-= PBIT_PRESENT;
    if(flags & VMEM_FLAG_W) result+= PBIT_WRITABLE;
    if(flags & VMEM_FLAG_X) result-= PBIT_NOEXECUTE;
    if(flags & VMEM_FLAG_K) {
        result+= PBIT_GLOBALPAGE;
        result-= PBIT_USERACCESS;
    }

    return result;
}

VirMemManager::VirMemManager(PhyMemManager& physmem) : phymem(&physmem),
                                                       free_mapitems(NULL),
                                                       free_listitems(NULL) {
    using namespace x86paging;

    //Allocate some data storage space.
    alloc_listitems();
    alloc_mapitems();

    //Create management structures for the kernel.
    map_list = free_listitems;
    free_listitems = free_listitems->next_item;
    map_list->map_owner = PID_KERNEL;
    map_list->map_pointer = NULL; //Paging is disabled for the kernel (turns out it makes things
                                  //insanely complicated for a small benefit in the end)
    map_list->next_item = NULL;
    map_list->pml4t_location = x86paging::get_pml4t();

    //Know where the kernel is in the physical and virtual address space
    extern char knl_rx_start, knl_r_start, knl_rw_start;
    knl_rx_loc = (addr_t) &knl_rx_start;
    knl_r_loc = (addr_t) &knl_r_start;
    knl_rw_loc = (addr_t) &knl_rw_start;
    addr_t kernel_pml4t = get_pml4t();
    phy_knl_rx = phymem->find_thischunk(get_target(knl_rx_loc, kernel_pml4t));
    phy_knl_r = phymem->find_thischunk(get_target(knl_r_loc, kernel_pml4t));
    phy_knl_rw = phymem->find_thischunk(get_target(knl_rw_loc, kernel_pml4t));
}

VirMemMap* VirMemManager::map(const PhyMemMap* phys_chunk,
                              const PID target,
                              const VirMemFlags flags) {
    VirMapList* list_item;
    VirMemMap* result;

    if(target == PID_KERNEL) return NULL; //Paging is disabled for the kernel

    maplist_mutex.grab_spin();

        list_item = find_or_create_pid(target);
        if(!list_item) {
            maplist_mutex.release();
            return NULL;
        }

        list_item->mutex.grab_spin();

            //Map that chunk
            result = chunk_mapper(phys_chunk, flags, list_item);

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

bool VirMemManager::free(VirMemMap* chunk) {
    bool result;

    if(chunk->owner == map_list) return false; //Paging is disabled for the kernel

    VirMapList* chunk_owner = chunk->owner; //We won't have access to this once the chunk is
                                            //liberated, so better keep a copy around.

    maplist_mutex.grab_spin(); //The map list may be modified too, if the process' item is liberated

        chunk_owner->mutex.grab_spin();

            //Free that chunk
            result = chunk_liberator(chunk);

        if(chunk_owner->pml4t_location) chunk_owner->mutex.release();

    maplist_mutex.release();

    return result;
}

bool VirMemManager::adjust_flags(VirMemMap* chunk,
                                 const VirMemFlags flags,
                                 const VirMemFlags mask) {
    bool result;

    if(chunk->owner == map_list) return false; //Paging is disabled for the kernel

    chunk->owner->mutex.grab_spin();

        //Adjust these flags
        result = flag_adjust(chunk, flags, mask, chunk->owner);

    chunk->owner->mutex.release();

    return result;
}

void VirMemManager::kill(PID target) {
    maplist_mutex.grab_spin();

        remove_pid(target);

    maplist_mutex.release();
}

uint64_t VirMemManager::cr3_value(const PID target) {
    uint64_t result = NULL;

    maplist_mutex.grab_spin();

        VirMapList* list_item = find_pid(target);
        if(list_item) result = list_item->pml4t_location;

    maplist_mutex.release();

    return result;
}

void VirMemManager::print_maplist() {
    maplist_mutex.grab_spin();

        dbgout << *map_list;

    maplist_mutex.release();
}

void VirMemManager::print_mmap(PID owner) {
    VirMapList* list_item;

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

void VirMemManager::print_pml4t(PID owner) {
    VirMapList* list_item;

    maplist_mutex.grab_spin();

        list_item = find_pid(owner);
        if(!list_item || !(list_item->pml4t_location)) {
            dbgout << txtcolor(TXT_RED) << "Error : PML4T does not exist";
            dbgout << txtcolor(TXT_LIGHTGRAY);
        } else {
            list_item->mutex.grab_spin();
        }

    maplist_mutex.release();

    if(list_item && list_item->pml4t_location) {
        x86paging::dbg_print_pml4t(list_item->pml4t_location);
        list_item->mutex.release();
    }
}
