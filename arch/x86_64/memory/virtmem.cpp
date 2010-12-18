 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

    Copyright (C) 2010    Hadrien Grasland

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

#include <dbgstream.h>
#include <display_paging.h>

addr_t VirMemManager::alloc_mapitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    VirMemMap* current_item;
    
    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return NULL;
    
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
    return allocated_chunk->location;
}

addr_t VirMemManager::alloc_listitems() {
    addr_t used_space;
    PhyMemMap* allocated_chunk;
    VirMapList* current_item;
    
    //Allocate a page of memory
    allocated_chunk = phymem->alloc_page(PID_KERNEL);
    if(!allocated_chunk) return NULL;
    
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
    return allocated_chunk->location;
}

VirMemMap* VirMemManager::chunk_liberator(VirMemMap* chunk, VirMapList* target) {
    VirMemMap *current_mapitem, *current_item = chunk, *next_item;
    
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
        remove_paging(current_item->location, current_item->size, target->pml4t_location);
        
        //Remove the rest
        *current_item = VirMemMap();
        current_item->next_buddy = free_mapitems;
        free_mapitems = current_item;
        current_item = next_item;
    }
    
    //Check if this PID's address space is empty, if so delete it.
    if(target->map_pointer == NULL) remove_pid(target->map_owner);
    
    return chunk;
}

VirMemMap* VirMemManager::chunk_mapper(const PhyMemMap* phys_chunk, const VirMemFlags flags, VirMapList* target) {
    //We have two goals here :
    // -To map that chunk of physical memory in the virtual memory map of the target.
    // -To put it in its virtual address space, too.
    
    addr_t total_size, offset, tmp;
    VirMemMap *result, *map_parser, *last_item;
    PhyMemMap *chunk_parser;
    
    //Allocate the new memory map item
    if(!free_mapitems) {
        alloc_mapitems();
        if(!free_mapitems) return NULL;
    }
    result = free_mapitems;
    free_mapitems = free_mapitems->next_buddy;
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
    
    //Find virtual chunk location and put it in the map
    if(target->map_pointer == NULL) {
        result->location = PG_SIZE; //First page should not be used : it includes the NULL pointer.
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
    
    //Allocate paging structures
    //If allocation fails, don't forget to restore the map into a clean state before returning NULL.
    tmp = setup_4kpages(result->location, result->size, target->pml4t_location);
    if(!tmp) {
        chunk_liberator(result, target);
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

VirMapList* VirMemManager::find_pid(PID target) {
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
    
    return result;
}

VirMapList* VirMemManager::remove_pid(PID target) {
    VirMapList *result, *previous_item;
    
    //target can't be the first item of the map list, as this item is the kernel.
    //Paging is disabled for the kernel
    previous_item = map_list;
    while(previous_item->next_item) {
        if(previous_item->next_item->map_owner == target) break;
        previous_item = previous_item->next_item;
    }
    result = previous_item->next_item;
    previous_item->next_item = previous_item->next_item->next_item;
    if(!result) return NULL;
    
    //Free all its paging structures and its entry
    remove_all_paging(result);
    phymem->free(result->pml4t_location);
    *result = VirMapList();
    result->next_item = free_listitems;
    free_listitems = result;
    
    return result;
}

VirMemMap* VirMemManager::flag_adjust(VirMemMap* chunk,
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

    return chunk;
}

addr_t VirMemManager::setup_4kpages(addr_t vir_addr, const addr_t length, addr_t pml4t_location) {
    using namespace x86paging;
    
    pml4e* pml4t = (pml4e*) pml4t_location;
    pdp* pdpt;
    pde* pd;
    addr_t tmp;
    int pd_index, pdpt_index, pml4t_index;
    int pd_len, pdpt_len, pml4t_len;
    PhyMemMap* allocd_data;
    
    //Determine where the virtual address is located in the paging structures
    tmp = vir_addr/(0x1000*PTABLE_LENGTH);
    pd_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pdpt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pml4t_index = tmp%PTABLE_LENGTH;
    
    //Determine on how much paging structures the vmem block spreads
    tmp = (addr_t) align_up(length, 0x1000)/0x1000; //In pages
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page tables
    const int first_pd_len = min(tmp, PTABLE_LENGTH-pd_index);
    const int last_pd_len = (tmp-first_pd_len)%(PTABLE_LENGTH+1);
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page directories
    const int first_pdpt_len = min(tmp, PTABLE_LENGTH-pdpt_index);
    const int last_pdpt_len = (tmp-first_pdpt_len)%(PTABLE_LENGTH+1);
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In PDPTs
    pml4t_len = tmp%(PTABLE_LENGTH-pml4t_index+1);
        
    //Allocate paging structures
    for(int pml4t_parser = 0; pml4t_parser < pml4t_len; ++pml4t_parser) { //PML4T level : allocate PDPTs, parse them to allocate PDs
        //Make sure the PDPT exists
        if(!pml4t[pml4t_index+pml4t_parser]) {
            allocd_data = phymem->alloc_page(PID_KERNEL);
            if(!allocd_data) return NULL;
            pml4t[pml4t_index+pml4t_parser] = allocd_data->location + PBIT_PRESENT + PBIT_WRITABLE + PBIT_USERACCESS;
        }
        pdpt = (pdp*) (pml4t[pml4t_index+pml4t_parser] & 0x000ffffffffff000);
        
        //Know which of its entries we're going to parse
        if(pml4t_parser == 0) {
            pdpt_len = first_pdpt_len;
        } else {
            if(pml4t_parser == 1) {
                pdpt_index = 0; //pdpt_index is only valid for the first PDPT we consider.
                pdpt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
            }
            if(pml4t_parser == pml4t_len-1) pdpt_len = last_pdpt_len;
        }
        
        
        //Parse it
        for(int pdpt_parser = 0; pdpt_parser < pdpt_len; ++pdpt_parser) { //PDPT level : allocate PDs, parse them to allocate PTs
            //Make sure the PD exists
            if(!pdpt[pdpt_index+pdpt_parser]) {
                allocd_data = phymem->alloc_page(PID_KERNEL);
                if(!allocd_data) return NULL;
                pdpt[pdpt_index+pdpt_parser] = allocd_data->location + PBIT_PRESENT + PBIT_WRITABLE + PBIT_USERACCESS;
            }
            pd = (pde*) (pdpt[pdpt_index+pdpt_parser] & 0x000ffffffffff000);
            
            //Know which of its entries we're going to parse
            if(pml4t_parser == 0 && pdpt_parser == 0) {
                pd_len = first_pd_len;
            } else {
                if(pml4t_parser == 0 && pdpt_parser == 1) {
                    pd_index = 0; //pd_index is only valid for the very first PD we consider.
                    pd_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
                }
                if(pml4t_parser == pml4t_len-1 && pdpt_parser == pdpt_len-1) pd_len = last_pd_len; //...except for the last one.
            }
            
            //Parse it
            for(int pd_parser = 0; pd_parser < pd_len; ++pd_parser) { //PD level : allocate page tables
                if(!pd[pd_index+pd_parser]) {
                    allocd_data = phymem->alloc_page(PID_KERNEL);
                    if(!allocd_data) return NULL;
                    pd[pd_index+pd_parser] = allocd_data->location + PBIT_PRESENT + PBIT_WRITABLE + PBIT_USERACCESS;
                }
            }
        }
    }
    
    return pml4t_location;
}

addr_t VirMemManager::remove_paging(addr_t vir_addr, const addr_t length, addr_t pml4t_location) {
    using namespace x86paging;
    
    pml4e* pml4t = (pml4e*) pml4t_location;
    pdp* pdpt;
    pde* pd;
    pte* pt;
    addr_t tmp;
    int pt_index, pd_index, pdpt_index, pml4t_index;
    int pt_len, pd_len, pdpt_len, pml4t_len;
    
    //Determine where the virtual address is located in the paging structures
    tmp = vir_addr/0x1000;
    pt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pd_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pdpt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pml4t_index = tmp%PTABLE_LENGTH;
    
    //Determine on how much paging structures the vmem block spreads
    tmp = (addr_t) align_up(length, 0x1000)/0x1000; //In pages
    const int first_pt_len = min(tmp, PTABLE_LENGTH-pt_index);
    const int last_pt_len = (tmp-first_pt_len)%(PTABLE_LENGTH+1);
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page tables
    const int first_pd_len = min(tmp, PTABLE_LENGTH-pd_index);
    const int last_pd_len = (tmp-first_pd_len)%(PTABLE_LENGTH+1);
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page directories
    const int first_pdpt_len = min(tmp, PTABLE_LENGTH-pdpt_index);
    const int last_pdpt_len = (tmp-first_pdpt_len)%(PTABLE_LENGTH+1);
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In PDPTs
    pml4t_len = tmp%(PTABLE_LENGTH-pml4t_index+1);
    
    //Free paging structures
    for(int pml4t_parser = 0; pml4t_parser < pml4t_len; ++pml4t_parser) { //PML4T level : parse PDPTs to free PDs, then free them
        //If this PDPT does not exist, skip it.
        if(!pml4t[pml4t_index+pml4t_parser]) continue;
        pdpt = (pdp*) (pml4t[pml4t_index+pml4t_parser] & 0x000ffffffffff000);
        
        //Know which of its entries we're going to parse
        if(pml4t_parser == 0) {
            pdpt_len = first_pdpt_len;
        } else {
            if(pml4t_parser == 1) {
                pdpt_index = 0; //pdpt_index is only valid for the first PDPT we consider.
                pdpt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
            }
            if(pml4t_parser == pml4t_len-1) pdpt_len = last_pdpt_len; //...except for the last one.
        }
        
        //Parse it
        for(int pdpt_parser = 0; pdpt_parser < pdpt_len; ++pdpt_parser) { //PDPT level : parse PDs to free PTs, then free them
            //If this PD does not exist, skip it
            if(!pdpt[pdpt_index+pdpt_parser]) continue;
            pd = (pde*) (pdpt[pdpt_index+pdpt_parser] & 0x000ffffffffff000);
            
            //Know which of its entries we're going to parse
            if(pdpt_parser == 0 && pml4t_parser == 0) {
                pd_len = first_pd_len;
            } else {
                if(pdpt_parser == 1 && pml4t_parser == 0) {
                    pd_index = 0; //pd_index is only valid for the very first PD we consider.
                    pd_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
                }
                if(pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pd_len = last_pd_len; //...except for the last one.
            }
            
            //Parse it
            for(int pd_parser = 0; pd_parser < pd_len; ++pd_parser) { //PD level : parse PTs to free individual items, or free them
                //If this PT does not exist, skip it
                if(!pd[pd_index+pd_parser]) continue;
                pt = (pte*) (pd[pd_index+pd_parser] & 0x000ffffffffff000);
                
                //Know which of its entries we're going to parse
                if(pd_parser == 0 && pdpt_parser == 0 && pml4t_parser == 0) {
                    pt_len = first_pt_len;
                } else {
                    if(pd_parser == 1 && pdpt_parser == 0 && pml4t_parser == 0) {
                        pt_index = 0; //pt_index is only valid for the very first PT we consider.
                        pt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
                    }
                    if(pd_parser == pd_len-1 && pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pt_len = last_pt_len; //...except for the last one.
                }
                
                //Parse and clear entries of this page table
                for(int pt_parser = 0; pt_parser < pt_len; ++pt_parser) { //PT level : clear individual page table entries
                    pt[pt_index+pt_parser] = 0;
                }
                
                //If this page table is now empty, free it
                bool pt_empty = true;
                for(int pt_parser = 0; pt_parser < PTABLE_LENGTH; ++pt_parser) {
                    if(pt[pt_parser]) pt_empty = false; break;
                }
                if(pt_empty) {
                    phymem->free((addr_t) pt);
                    pd[pd_index+pd_parser] = 0;
                }
            }
            
            //If this page directory is now empty, free it
            bool pd_empty = true;
            for(int pd_parser = 0; pd_parser < PTABLE_LENGTH; ++pd_parser) {
                if(pd[pd_parser]) pd_empty = false; break;
            }
            if(pd_empty) {
                phymem->free((addr_t) pd);
                pdpt[pdpt_index+pdpt_parser] = 0;
            }
        }
        
        //If this PDPT is now empty, free it
        bool pdpt_empty = true;
        for(int pdpt_parser = 0; pdpt_parser < PTABLE_LENGTH; ++pdpt_parser) {
            if(pdpt[pdpt_parser]) pdpt_empty = false; break;
        }
        if(pdpt_empty) {
            phymem->free((addr_t) pdpt);
            pml4t[pml4t_index+pml4t_parser] = 0;
        }
    }
    
    return pml4t_location;
}

addr_t VirMemManager::remove_all_paging(VirMapList* target) {
    using namespace x86paging;

    addr_t total_address_space = PG_SIZE*PTABLE_LENGTH; //Size of a page table
    total_address_space*= PTABLE_LENGTH; //Size of a PD
    total_address_space*= PTABLE_LENGTH; //Size of a PDPT
    total_address_space*= PTABLE_LENGTH; //Size of the whole PML4T
    return remove_paging(0, total_address_space, target->pml4t_location);
}

uint64_t VirMemManager::x86flags(VirMemFlags flags) {
    uint64_t result = PBIT_USERACCESS + PBIT_NOEXECUTE;
    
    if(flags & VMEM_FLAG_P) result+= PBIT_PRESENT;
    if(flags & VMEM_FLAG_W) result+= PBIT_WRITABLE;
    if(flags & VMEM_FLAG_X) result-= PBIT_NOEXECUTE;
    
    return result;
}

VirMemManager::VirMemManager(PhyMemManager& physmem) : phymem(&physmem),
                                                       free_mapitems(NULL),
                                                       free_listitems(NULL) {
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
                if(list_item->map_pointer == NULL) remove_pid(target);
            }
        
        list_item->mutex.release();
        
    maplist_mutex.release();
    
    return result;
}

VirMemMap* VirMemManager::free(VirMemMap* chunk) {
    VirMemMap* result;
        
    if(chunk->owner == map_list) return NULL; //Paging is disabled for the kernel
    
    VirMapList* chunk_owner = chunk->owner; //We won't have access to this once the chunk is
                                            //liberated, so better keep a copy around.
                                            
    maplist_mutex.grab_spin(); //The map list may be modified too, if the process' item is liberated
    
        chunk_owner->mutex.grab_spin();
        
            //Free that chunk
            result = chunk_liberator(chunk, chunk->owner);
        
        if(chunk_owner->pml4t_location) chunk_owner->mutex.release();
        
    maplist_mutex.release();
    
    return result;
}

VirMemMap* VirMemManager::adjust_flags(VirMemMap* chunk,
                                       const VirMemFlags flags,
                                       const VirMemFlags mask) {
    VirMemMap* result;
    
    if(chunk->owner == map_list) return NULL; //Paging is disabled for the kernel
    
    chunk->owner->mutex.grab_spin();
        
        //Adjust these flags
        result = flag_adjust(chunk, flags, mask, chunk->owner);
    
    chunk->owner->mutex.release();
    
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
