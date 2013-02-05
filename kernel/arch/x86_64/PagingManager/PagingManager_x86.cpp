 /* Arch-specific part of paging management

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

#include <align.h>
#include <kmath.h>
#include <new.h>
#include <PagingManager.h>
#include <x86paging.h>
#include <x86paging_parser.h>

#include <dbgstream.h>
#include <display_paging.h>


PagingManager* paging_manager = NULL;

PageChunk* PagingManager::chunk_mapper_contig(PagingManagerProcess* target,
                                                const RamChunk* ram_chunk,
                                                const PageFlags flags) {
    size_t total_size, offset, tmp;
    RamChunk *chunk_parser;
    PageChunk *result;

    //Find total page chunk size
    chunk_parser = (RamChunk*) ram_chunk;
    total_size = 0;
    while(chunk_parser) {
        total_size+= chunk_parser->size;
        chunk_parser = chunk_parser->next_buddy;
    }

    //Allocate a contiguous chunk of virtual address space to map our RAM chunk
    result = alloc_virtual_address_space(target, total_size);
    if(!result) return NULL;

    //Finish setting up the allocated chunk
    result->flags = flags;
    result->points_to = (RamChunk*) ram_chunk;

    //Allocate paging structures. For pages with the K flag enabled, also identity-map them.
    //If allocation fails, don't forget to restore the map into a clean state before returning NULL.
    tmp = x86paging::setup_4kpages(result->location,
                                   result->size,
                                   target->pml4t_location,
                                   ram_manager);
    if(!tmp) {
        chunk_liberator(target, result);
        return NULL;
    }

    //Fill those structures which we just allocated
    chunk_parser = (RamChunk*) ram_chunk;
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

PageChunk* PagingManager::chunk_mapper_identity(PagingManagerProcess* target,
                                                  const RamChunk* ram_chunk,
                                                  const PageFlags flags,
                                                  size_t offset) {
    size_t tmp;
    RamChunk *chunk_parser;
    PageChunk *result, *current_pagechunk;

    //For identity-mapping, we map each part of the chunk separately.
    chunk_parser = (RamChunk*) ram_chunk;
    while(chunk_parser) {
        //Allocate virtual address space for the current part of the RAM chunk
        current_pagechunk = alloc_virtual_address_space(target,
                                                       chunk_parser->size,
                                                       chunk_parser->location+offset);
        if(!current_pagechunk) return NULL;
        if(chunk_parser == ram_chunk) result = current_pagechunk;

        //Finish setting up the allocated chunk
        current_pagechunk->flags = flags;
        current_pagechunk->points_to = chunk_parser;

        //Allocate paging structures. For pages with the K flag enabled, also identity-map them.
        tmp = x86paging::setup_4kpages(current_pagechunk->location,
                                       current_pagechunk->size,
                                       target->pml4t_location,
                                       ram_manager);
        if(!tmp) {
            chunk_liberator(target, result);
            return NULL;
        }

        //Fill those structures which we just allocated
        x86paging::fill_4kpaging(chunk_parser->location,
                                 current_pagechunk->location,
                                 chunk_parser->size,
                                 x86flags(current_pagechunk->flags),
                                 target->pml4t_location);

        //Go to next part of the RAM chunk
        chunk_parser = chunk_parser->next_buddy;
    }

    return result;
}

bool PagingManager::chunk_liberator(PagingManagerProcess* target,
                                    PageChunk* chunk) {
    PageChunk *current_mapitem, *current_item = chunk, *next_item;

    //First, manage K pages : non-kernel processes cannot get rid of them, and if they
    //are ditched by the kernel they are ditched by all other processes too.
    if(chunk->flags & PAGE_FLAG_K) {
        if(target->identifier != PID_KERNEL) {
            if(target->may_free_kpages==0) return false;
        } else {
            unmap_k_chunk(chunk);
        }
    }

    //Remove item from the map
    while(current_item) {
        next_item = current_item->next_buddy;

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
                                 ram_manager);

        //Remove the rest
        current_item = new(current_item) PageChunk();
        current_item->next_buddy = free_mapitems;
        free_mapitems = current_item;
        current_item = next_item;
    }

    return true;
}

PageChunk* PagingManager::flag_adjust(PagingManagerProcess* target,
                                PageChunk* chunk,
                                const PageFlags flags,
                                const PageFlags mask) {
    PageChunk* current_item;

    //Only the kernel may alter the status of K pages, and the only use case which is taken into
    //account for now is the loss of the K flag.
    if(mask & PAGE_FLAG_K) {
        if(target->identifier != PID_KERNEL) return NULL;
        if((flags & PAGE_FLAG_K) == 0) unmap_k_chunk(chunk);
    }

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

    return chunk;
}

bool PagingManager::map_kernel() {
    size_t phy_knl_rx_loc, phy_knl_r_loc, phy_knl_rw_loc;
    size_t vir_knl_rx_loc, vir_knl_r_loc, vir_knl_rw_loc, kernel_pml4t;
    RamChunk* ram_map;

    //Find out about the kernel's physical memory location, as it is not identity-mapped.
    extern char knl_rx_start, knl_r_start, knl_rw_start;
    vir_knl_rx_loc = (size_t) &knl_rx_start;
    vir_knl_r_loc = (size_t) &knl_r_start;
    vir_knl_rw_loc = (size_t) &knl_rw_start;
    kernel_pml4t = x86paging::get_pml4t();
    phy_knl_rx_loc = x86paging::get_target(vir_knl_rx_loc, kernel_pml4t);
    phy_knl_r_loc = x86paging::get_target(vir_knl_r_loc, kernel_pml4t);
    phy_knl_rw_loc = x86paging::get_target(vir_knl_rw_loc, kernel_pml4t);

    //Parse and map the kernel's virtual address space properly (save for the address associated
    //to the NULL pointer, which must remain invalid)
    ram_map = ram_manager->dump_mmap();
    while(ram_map) {
        if(ram_map->has_owner(PID_KERNEL)) {
            if(ram_map->location == phy_knl_rx_loc) {
                chunk_mapper(process_list, ram_map, PAGE_FLAGS_RX + PAGE_FLAG_K, vir_knl_rx_loc);
            } else if(ram_map->location == phy_knl_r_loc) {
                chunk_mapper(process_list, ram_map, PAGE_FLAG_R + PAGE_FLAG_K, vir_knl_r_loc);
            } else if(ram_map->location == phy_knl_rw_loc) {
                chunk_mapper(process_list, ram_map, PAGE_FLAGS_RW + PAGE_FLAG_K, vir_knl_rw_loc);
            } else if(ram_map->location) {
                chunk_mapper(process_list, ram_map, PAGE_FLAGS_RW + PAGE_FLAG_K, ram_map->location);
            }
        }

        ram_map = ram_map->next_mapitem;
    }

    return true;
}

bool PagingManager::remove_all_paging(PagingManagerProcess* target) {
    using namespace x86paging;

    size_t total_address_space = PG_SIZE*PTABLE_LENGTH; //Size of a page table
    total_address_space*= PTABLE_LENGTH; //Size of a PD
    total_address_space*= PTABLE_LENGTH; //Size of a PDPT
    total_address_space*= PTABLE_LENGTH; //Size of the whole PML4T
    return remove_paging(0, total_address_space, target->pml4t_location, ram_manager);
}

bool PagingManager::remove_pid(PID target) {
    PagingManagerProcess *deleted_item, *previous_item;

    //target can't be the first item of the map list, as this item is the kernel. One can't kill the kernel.
    previous_item = process_list;
    while(previous_item->next_item) {
        if(previous_item->next_item->identifier == target) break;
        previous_item = previous_item->next_item;
    }
    deleted_item = previous_item->next_item;
    if(!deleted_item) return false;
    previous_item->next_item = deleted_item->next_item;

    //Since the target is currently being freed, allow the liberation of K pages
    deleted_item->may_free_kpages = 1;

    //Free all its paging structures and its entry
    while(deleted_item->map_pointer) chunk_liberator(deleted_item, deleted_item->map_pointer);
    remove_all_paging(deleted_item);
    ram_manager->free_chunk(PID_KERNEL, deleted_item->pml4t_location);
    deleted_item = new(deleted_item) PagingManagerProcess();
    deleted_item->next_item = free_process_descs;
    free_process_descs = deleted_item;

    return true;
}

PagingManagerProcess* PagingManager::setup_pid(PID target) {
    PagingManagerProcess* result;
    RamChunk* pml4t_page;

    //Allocate management structures
    if(!free_process_descs) {
        alloc_process_descs();
        if(!free_process_descs) return NULL;
    }
    pml4t_page = ram_manager->alloc_chunk(PID_KERNEL);
    if(!pml4t_page) return NULL;

    //Fill them
    result = free_process_descs;
    free_process_descs = free_process_descs->next_item;
    result->next_item = NULL;
    result->identifier = target;
    result->pml4t_location = pml4t_page->location;
    x86paging::create_pml4t(result->pml4t_location);

    //Map K pages in the process' user space
    bool tmp = map_k_chunks(result);
    if(!tmp) {
        result->next_item = process_list->next_item;
        process_list->next_item = result;
        remove_pid(result->identifier);
        return NULL;
    }

    return result;
}

void PagingManager::unmap_k_chunk(PageChunk *chunk) {
    PageChunk* k_chunk;
    PagingManagerProcess* process_parser = process_list;

    while(process_parser->next_item) {
        //Find non-kernel processes which have the K chunk mapped in their address space.
        process_parser = process_parser->next_item;
        k_chunk = process_parser->map_pointer->find_thischunk(chunk->location);
        if(!k_chunk) continue;

        //Remove it from there.
        uint16_t old_free_kpages = process_parser->may_free_kpages;
        process_parser->may_free_kpages = 1;
        chunk_liberator(process_parser, k_chunk);
        process_parser->may_free_kpages = old_free_kpages;
    }
}

uint64_t PagingManager::x86flags(PageFlags flags) {
    using namespace x86paging;

    uint64_t result = PBIT_PRESENT + PBIT_USERACCESS + PBIT_NOEXECUTE;

    if(flags & PAGE_FLAG_A) result-= PBIT_PRESENT;
    if(flags & PAGE_FLAG_W) result+= PBIT_WRITABLE;
    if(flags & PAGE_FLAG_X) result-= PBIT_NOEXECUTE;
    if(flags & PAGE_FLAG_K) {
        result+= PBIT_GLOBALPAGE;
        result-= PBIT_USERACCESS;
    }

    return result;
}

PagingManager::PagingManager(RamManager& ram_man) : ram_manager(&ram_man),
                                                    process_manager(NULL),
                                                    free_mapitems(NULL),
                                                    free_process_descs(NULL) {
    //Allocate some data storage space.
    alloc_process_descs();
    alloc_mapitems();

    //Create management structures for the kernel.
    process_list = free_process_descs;
    free_process_descs = free_process_descs->next_item;
    process_list->next_item = NULL;
    process_list->identifier = PID_KERNEL;
    process_list->pml4t_location = x86paging::get_pml4t();

    //Map the kernel's virtual address space
    map_kernel();

    //Activate global paging management service
    paging_manager = this;
}

uint64_t PagingManager::cr3_value(const PID target) {
    uint64_t result = NULL;

    proclist_mutex.grab_spin();

        PagingManagerProcess* list_item = find_pid(target);
        if(list_item) result = list_item->pml4t_location;

    proclist_mutex.release();

    return result;
}

void PagingManager::print_pml4t(PID owner) {
    PagingManagerProcess* list_item;

    proclist_mutex.grab_spin();

        list_item = find_pid(owner);
        if(!list_item || !(list_item->pml4t_location)) {
            dbgout << txtcolor(TXT_RED) << "Error : PML4T does not exist";
            dbgout << txtcolor(TXT_DEFAULT);
        } else {
            list_item->mutex.grab_spin();
        }

    proclist_mutex.release();

    if(list_item && list_item->pml4t_location) {
        x86paging::dbg_print_pml4t(list_item->pml4t_location);
        list_item->mutex.release();
    }
}

/*PID paging_manager_add_process(PID id, ProcessProperties properties) {
    if(!paging_manager) {
        return PID_INVALID;
    } else {
        return paging_manager->add_process(id, properties);
    }
}*/

void paging_manager_remove_process(PID target) {
    if(!paging_manager) {
        return;
    } else {
        paging_manager->remove_process(target);
    }
}

/*PID paging_manager_update_process(PID old_process, PID new_process) {
    if(!paging_manager) {
        return PID_INVALID;
    } else {
        return paging_manager->update_process(old_process, new_process);
    }
}*/
