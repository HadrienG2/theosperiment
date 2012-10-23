 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

    Copyright (C) 2010-2012    Hadrien Grasland

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
#include <new.h>
#include <virmem_manager.h>
#include <x86paging.h>
#include <x86paging_parser.h>

#include <dbgstream.h>
#include <display_paging.h>


VirMemManager* virmem_manager = NULL;

VirMemChunk* VirMemManager::chunk_mapper(VirMemProcess* target,
                                         const PhyMemChunk* phys_chunk,
                                         const VirMemFlags flags,
                                         size_t location) {
    //We have two goals here :
    // -To map that chunk of physical memory in the virtual memory map of the target.
    // -To put it in its virtual address space, too.
    //
    //The location parameter is optional. By default, it is NULL, meaning that the chunk may be
    //mapped at any location and that non-contiguous physical memory chunks will be mapped as a
    //contiguous virtual memory chunk.
    //If location is nonzero, the physical chunk will be mapped at the specified location in a fashion
    //that strictly follows its physical layout with only an address offset, and that this will happen.

    //First, prevent non-kernel processes from handling K pages
    if((target->identifier != PID_KERNEL) && (flags & VIRMEM_FLAG_K)) return NULL;

    if(location == NULL) {
        return chunk_mapper_contig(target, phys_chunk, flags);
    } else {
        return chunk_mapper_identity(target, phys_chunk, flags, location-phys_chunk->location);
    }
}

VirMemChunk* VirMemManager::chunk_mapper_contig(VirMemProcess* target,
                                                const PhyMemChunk* phys_chunk,
                                                const VirMemFlags flags) {
    size_t total_size, offset, tmp;
    PhyMemChunk *chunk_parser;
    VirMemChunk *result;

    //Find total virtual chunk size
    chunk_parser = (PhyMemChunk*) phys_chunk;
    total_size = 0;
    while(chunk_parser) {
        total_size+= chunk_parser->size;
        chunk_parser = chunk_parser->next_buddy;
    }

    //Allocate a contiguous chunk of virtual address space to map our physical chunk
    result = alloc_virtual_address_space(target, total_size);
    if(!result) return NULL;

    //Finish setting up the allocated chunk
    result->flags = flags;
    result->points_to = (PhyMemChunk*) phys_chunk;

    //Allocate paging structures. For pages with the K flag enabled, also identity-map them.
    //If allocation fails, don't forget to restore the map into a clean state before returning NULL.
    tmp = x86paging::setup_4kpages(result->location,
                                   result->size,
                                   target->pml4t_location,
                                   phymem_manager);
    if(!tmp) {
        chunk_liberator(target, result);
        return NULL;
    }

    //Fill those structures which we just allocated
    chunk_parser = (PhyMemChunk*) phys_chunk;
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

VirMemChunk* VirMemManager::chunk_mapper_identity(VirMemProcess* target,
                                                  const PhyMemChunk* phys_chunk,
                                                  const VirMemFlags flags,
                                                  size_t offset) {
    size_t tmp;
    PhyMemChunk *chunk_parser;
    VirMemChunk *result, *current_virchunk;

    //For identity-mapping, we map each part of the chunk separately.
    chunk_parser = (PhyMemChunk*) phys_chunk;
    while(chunk_parser) {
        //Allocate virtual address space for the current part of the physical chunk
        current_virchunk = alloc_virtual_address_space(target,
                                                       chunk_parser->size,
                                                       chunk_parser->location+offset);
        if(!current_virchunk) return NULL;
        if(chunk_parser == phys_chunk) result = current_virchunk;

        //Finish setting up the allocated chunk
        current_virchunk->flags = flags;
        current_virchunk->points_to = chunk_parser;

        //Allocate paging structures. For pages with the K flag enabled, also identity-map them.
        tmp = x86paging::setup_4kpages(current_virchunk->location,
                                       current_virchunk->size,
                                       target->pml4t_location,
                                       phymem_manager);
        if(!tmp) {
            chunk_liberator(target, result);
            return NULL;
        }

        //Fill those structures which we just allocated
        x86paging::fill_4kpaging(chunk_parser->location,
                                 current_virchunk->location,
                                 chunk_parser->size,
                                 x86flags(current_virchunk->flags),
                                 target->pml4t_location);

        //Go to next part of the physical chunk
        chunk_parser = chunk_parser->next_buddy;
    }

    return result;
}

bool VirMemManager::chunk_liberator(VirMemProcess* target,
                                    VirMemChunk* chunk) {
    VirMemChunk *current_mapitem, *current_item = chunk, *next_item;

    //First, manage K pages : non-kernel processes cannot get rid of them, and if they
    //are ditched by the kernel they are ditched by all other processes too.
    if(chunk->flags & VIRMEM_FLAG_K) {
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
                                 phymem_manager);

        //Remove the rest
        current_item = new(current_item) VirMemChunk();
        current_item->next_buddy = free_mapitems;
        free_mapitems = current_item;
        current_item = next_item;
    }

    //Check if this PID's address space is empty, if so delete it.
    if(target->map_pointer == NULL) remove_pid(target->identifier);

    return true;
}

VirMemChunk* VirMemManager::flag_adjust(VirMemProcess* target,
                                VirMemChunk* chunk,
                                const VirMemFlags flags,
                                const VirMemFlags mask) {
    VirMemChunk* current_item;

    //Only the kernel may alter the status of K pages, and the only use case which is taken into
    //account for now is the loss of the K flag.
    if(mask & VIRMEM_FLAG_K) {
        if(target->identifier != PID_KERNEL) return NULL;
        if((flags & VIRMEM_FLAG_K) == 0) unmap_k_chunk(chunk);
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

bool VirMemManager::map_kernel() {
    size_t phy_knl_rx_loc, phy_knl_r_loc, phy_knl_rw_loc;
    size_t vir_knl_rx_loc, vir_knl_r_loc, vir_knl_rw_loc, kernel_pml4t;
    PhyMemChunk* phy_mmap;

    //Find out about the kernel's memory location, because it is not identity-mapped.
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
    phy_mmap = phymem_manager->dump_mmap();
    while(phy_mmap) {
        if(phy_mmap->has_owner(PID_KERNEL)) {
            if(phy_mmap->location == phy_knl_rx_loc) {
                chunk_mapper(process_list, phy_mmap, VIRMEM_FLAGS_RX + VIRMEM_FLAG_K, vir_knl_rx_loc);
            } else if(phy_mmap->location == phy_knl_r_loc) {
                chunk_mapper(process_list, phy_mmap, VIRMEM_FLAG_R + VIRMEM_FLAG_K, vir_knl_r_loc);
            } else if(phy_mmap->location == phy_knl_rw_loc) {
                chunk_mapper(process_list, phy_mmap, VIRMEM_FLAGS_RW + VIRMEM_FLAG_K, vir_knl_rw_loc);
            } else if(phy_mmap->location) {
                chunk_mapper(process_list, phy_mmap, VIRMEM_FLAGS_RW + VIRMEM_FLAG_K, phy_mmap->location);
            }
        }

        phy_mmap = phy_mmap->next_mapitem;
    }

    return true;
}

bool VirMemManager::remove_all_paging(VirMemProcess* target) {
    using namespace x86paging;

    size_t total_address_space = PG_SIZE*PTABLE_LENGTH; //Size of a page table
    total_address_space*= PTABLE_LENGTH; //Size of a PD
    total_address_space*= PTABLE_LENGTH; //Size of a PDPT
    total_address_space*= PTABLE_LENGTH; //Size of the whole PML4T
    return remove_paging(0, total_address_space, target->pml4t_location, phymem_manager);
}

bool VirMemManager::remove_pid(PID target) {
    VirMemProcess *deleted_item, *previous_item;

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
    phymem_manager->free_chunk(PID_KERNEL, deleted_item->pml4t_location);
    deleted_item = new(deleted_item) VirMemProcess();
    deleted_item->next_item = free_process_descs;
    free_process_descs = deleted_item;

    return true;
}

VirMemProcess* VirMemManager::setup_pid(PID target) {
    VirMemProcess* result;
    PhyMemChunk* pml4t_page;

    //Allocate management structures
    if(!free_process_descs) {
        alloc_process_descs();
        if(!free_process_descs) return NULL;
    }
    pml4t_page = phymem_manager->alloc_chunk(PID_KERNEL);
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

void VirMemManager::unmap_k_chunk(VirMemChunk *chunk) {
    VirMemChunk* k_chunk;
    VirMemProcess* process_parser = process_list;

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

uint64_t VirMemManager::x86flags(VirMemFlags flags) {
    using namespace x86paging;

    uint64_t result = PBIT_PRESENT + PBIT_USERACCESS + PBIT_NOEXECUTE;

    if(flags & VIRMEM_FLAG_A) result-= PBIT_PRESENT;
    if(flags & VIRMEM_FLAG_W) result+= PBIT_WRITABLE;
    if(flags & VIRMEM_FLAG_X) result-= PBIT_NOEXECUTE;
    if(flags & VIRMEM_FLAG_K) {
        result+= PBIT_GLOBALPAGE;
        result-= PBIT_USERACCESS;
    }

    return result;
}

VirMemManager::VirMemManager(PhyMemManager& physmem) : phymem_manager(&physmem),
                                                       process_manager(NULL),
                                                       free_mapitems(NULL),
                                                       free_process_descs(NULL),
                                                       malloc_active(false) {
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

    //Activate global virtual memory management service
    virmem_manager = this;
}

uint64_t VirMemManager::cr3_value(const PID target) {
    uint64_t result = NULL;

    proclist_mutex.grab_spin();

        VirMemProcess* list_item = find_pid(target);
        if(list_item) result = list_item->pml4t_location;

    proclist_mutex.release();

    return result;
}

void VirMemManager::print_pml4t(PID owner) {
    VirMemProcess* list_item;

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

/*PID virmem_manager_add_process(PID id, ProcessProperties properties) {
    if(!virmem_manager) {
        return PID_INVALID;
    } else {
        return virmem_manager->add_process(id, properties);
    }
}*/

void virmem_manager_remove_process(PID target) {
    if(!virmem_manager) {
        return;
    } else {
        virmem_manager->remove_process(target);
    }
}

/*PID virmem_manager_update_process(PID old_process, PID new_process) {
    if(!virmem_manager) {
        return PID_INVALID;
    } else {
        return virmem_manager->update_process(old_process, new_process);
    }
}*/
