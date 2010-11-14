 /* Virtual memory management, ie managing contiguous chunks of virtual memory
    (allocation, permission management...)

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
 
#include <align.h>
#include <virtmem.h>
#include <x86paging.h>

addr_t VirMemManager::alloc_mapitems() {
  addr_t used_space;
  PhyMemMap* allocated_chunk;
  VirMemMap* current_item;
  
  //Allocate a page of memory
  allocated_chunk = phymem->alloc_page(PID_KERNEL);
  if(!allocated_chunk) return NULL;
  
  //Fill it with initialized map items
  current_item = (VirMemMap*) (allocated_chunk->location);
  free_mapitems = current_item;
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
  
  //Fill it with initialized map items
  current_item = (VirMapList*) (allocated_chunk->location);
  free_listitems = current_item;
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

VirMemMap* VirMemManager::chunk_mapper(const PhyMemMap* phys_chunk, const VirMemFlags flags, VirMapList* target) {
  //We have two goals here :
  // -To map that chunk of physical memory in the virtual memory map of the target.
  // -To put it in its virtual address space, too.
  
  addr_t total_size = 0;
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
  result->points_to = (PhyMemMap*) phys_chunk;
  result->next_buddy = NULL;
  
  //Find virtual chunk size
  chunk_parser = (PhyMemMap*) phys_chunk;
  while(chunk_parser) {
    total_size+= chunk_parser->size;
    chunk_parser = chunk_parser->next_buddy;
  }
  result->size = total_size;
  
  //Find virtual chunk location
  if(target->map_pointer == NULL) {
    result->location = PG_SIZE; //First page should not be used.
  } else {
    if(target->map_pointer->location >= total_size) {
      result->location = target->map_pointer->location-total_size;
      result->next_mapitem = target->map_pointer;
      target->map_pointer = result;
    } else {
      last_item = target->map_pointer;
      map_parser = last_item->next_mapitem;
      while(map_parser) {
        if(map_parser->location-last_item->location >= total_size) {
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
  
  //TODO : Now, commit changes to the paging structures.
  //If allocation fails, don't forget to restore the map into a clean state before returning NULL.
  
  return result;
}

VirMapList* VirMemManager::find_or_create(PID target) {
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

VirMemFlags VirMemManager::get_current_flags(const addr_t location) const {
  uint64_t pgstruct_entry;
  VirMemFlags flags = 0;
  
  pgstruct_entry = x86paging::find_lowestpaging(location);
  if(pgstruct_entry) {
    if(pgstruct_entry & PBIT_PRESENT) flags|= VMEM_FLAG_P + VMEM_FLAG_R;
    if(pgstruct_entry & PBIT_WRITABLE) flags|= VMEM_FLAG_R + VMEM_FLAG_W;
    if(!(pgstruct_entry & PBIT_NOEXECUTE)) flags|= VMEM_FLAG_X;
  }
  return flags;
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
  
  return result;
}

VirMemManager::VirMemManager(PhyMemManager& physmem) {  
  //Store a pointer to the physical memory manager, for future access.
  phymem = &physmem;
  
  //Allocate some management structures.
  alloc_listitems();
  alloc_mapitems();
  
  //Create management structures for the kernel.
  map_list = free_listitems;
  free_listitems = free_listitems->next_item;
  map_list->map_owner = PID_KERNEL;
  map_list->map_pointer = NULL; //Paging is disabled for the kernel (as it turns out it makes things insanely complicated)
  map_list->next_item = NULL;
  map_list->pml4t_location = x86paging::get_pml4t();
}

VirMemMap* VirMemManager::map_chunk(const PhyMemMap* phys_chunk, const VirMemFlags flags, const PID target) {
  VirMapList* list_item;
  VirMemMap* result;
  
  if(target == PID_KERNEL) return NULL; //Paging is disabled for the kernel, so this operation is invalid
  
  maplist_mutex.grab_spin();
  
    list_item = find_or_create(target);
    if(!list_item) return NULL;
    
  list_item->mutex.grab_spin();
  maplist_mutex.release();
    
    //Map that chunk
    result = chunk_mapper(phys_chunk, flags, list_item);
  
  list_item->mutex.release();
  
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
  
    list_item = map_list;
    while(list_item) {
      if(list_item->map_owner == owner) break;
      list_item = list_item->next_item;
    }
    
  list_item->mutex.grab_spin();
  maplist_mutex.release();
  
    dbgout << *(list_item->map_pointer);
    
  list_item->mutex.release();
}
