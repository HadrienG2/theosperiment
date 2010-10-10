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

VirMemFlags VirMemManager::get_current_flags(addr_t location) {
  uint64_t pgstruct_entry;
  VirMemFlags flags = 0;
  
  pgstruct_entry = find_lowestpaging(location);
  if(pgstruct_entry) {
    if(pgstruct_entry & PBIT_PRESENT) flags|= VMEM_FLAG_P + VMEM_FLAG_R;
    if(pgstruct_entry & PBIT_WRITABLE) flags|= VMEM_FLAG_R + VMEM_FLAG_W;
    if(!(pgstruct_entry & PBIT_NOEXECUTE)) flags|= VMEM_FLAG_X;
  }
  return flags;
}

void VirMemManager::map_kernel(PhyMemMap* phy_mmap) {
  PhyMemMap *current_phyitem;
  VirMemMap *free_mem, *kernel_item, *previous_item, *new_freemem, *initial_freemem, *trashed_item;
  extern char knl_rx_start, knl_rx_end;
  extern char knl_r_start, knl_r_end;
  extern char knl_rw_start, knl_rw_end;
  addr_t segments[6] = {(addr_t) &knl_rx_start, (addr_t) &knl_rx_end, (addr_t) &knl_r_start, (addr_t) &knl_r_end,
    (addr_t) &knl_rw_start, (addr_t) &knl_rw_end};
  addr_t phy_addr;
  
  //Kernel segments are mapped around 1GB in virtual memory, above all data allocated at this stage of the kernel
  //initialization process. At this place, in physical memory, we can only have a contiguous region of free physical memory,
  //which ends somewhere and is followed by... well... void, and some memory-mapped hardware high above around 4GB which is
  //of no interest for us.
  //Kernel segments can be mapped either in the middle of the big free memory region, in a void space, or at the frontier of
  //both. We'll first locate that free memory item, then check where each segment has landed and map it.
  free_mem = map_list->map_pointer;
  while(free_mem->location+free_mem->size <= segments[0]) {
    if(free_mem->next_mapitem == NULL) break;
    if(free_mem->next_mapitem->location > segments[0]) break;
    free_mem = free_mem->next_mapitem;
  }
  previous_item = free_mem;
  initial_freemem = free_mem;
  
  //We successively explore each kernel segment, in growing address order.
  for(int current_sgt = 0; current_sgt<6; current_sgt+=2) {
    //Allocate and fill a map item corresponding to the kernel segment, except of for the next_mapitem and
    //next_buddy fields which require further informations in order to be filled.
    kernel_item = free_mapitems;
    free_mapitems = free_mapitems->next_buddy;
    kernel_item->next_buddy = NULL;
    
    //Fill kernel segment data
    kernel_item->location = segments[current_sgt];
    kernel_item->size = segments[current_sgt+1] - segments[current_sgt];
    kernel_item->add_owner(PID_KERNEL);
    kernel_item->flags = get_current_flags(segments[current_sgt]);
    
    //Locate the segment in physical memory map
    phy_addr = get_target(segments[current_sgt]);
    current_phyitem = phy_mmap;
    while(current_phyitem) {
      if(current_phyitem->location == phy_addr) break;
      current_phyitem = current_phyitem->next_mapitem;
    }
    kernel_item->points_to = current_phyitem;
    
    //Now, manage each of the three possible situations mentioned above
    new_freemem = NULL;
    if(segments[current_sgt] < free_mem->location + free_mem->size) {
      if(segments[current_sgt+1] < free_mem->location + free_mem->size - 1) {
        //Kernel segment is in the beginning or middle of the free memory region.
        new_freemem = free_mapitems;
        free_mapitems = free_mapitems->next_buddy;
        new_freemem->next_buddy = NULL;
        
        //Content : same as free_memory, except for location, size, and chaining
        *new_freemem = *free_mem;
        new_freemem->location = segments[current_sgt+1];
        new_freemem->size = free_mem->location + free_mem->size - segments[current_sgt+1];
        free_mem->next_mapitem = kernel_item;
        kernel_item->next_mapitem = new_freemem;
      } else {
        //Segment overlaps both void and free memory
        kernel_item->next_mapitem = free_mem->next_mapitem;
        free_mem->next_mapitem = kernel_item;
      }
      //Anyway, the kernel segment overlaps with the end of the free memory region, so its size must be adjusted
      free_mem->size -= free_mem->location + free_mem->size - segments[current_sgt];
      
      if(new_freemem) {
        //Update the definition of free_mem : the place where next kernel segments can be is now new_freemem
        free_mem = new_freemem;
      }
    } else {
      //Kernel segment is in the middle of physically void space and can just be mapped at the right place
      //(to keep mmap sorted).
      //All we know about that "right place" is that it's after previous_item.
      while(previous_item->next_mapitem) {
        if(previous_item->next_mapitem->location >= segments[current_sgt+1]) break;
        previous_item = previous_item->next_mapitem;
      }
      kernel_item->next_mapitem = previous_item->next_mapitem;
      previous_item->next_mapitem = kernel_item;
    }
  }
  
  //Cleanup : erase all empty memory map items we may have created (starting from initial_freemem)
  while(initial_freemem->next_mapitem) {
    if(initial_freemem->next_mapitem->size == 0) {
      trashed_item = initial_freemem->next_mapitem;
      initial_freemem->next_mapitem = initial_freemem->next_mapitem->next_mapitem;
      *trashed_item = VirMemMap();
      trashed_item->next_buddy = free_mapitems;
      free_mapitems = trashed_item;
    } else {
      initial_freemem = initial_freemem->next_mapitem;
    }
  }
}

void update_kernel_mmap() {
  //How it works :
  //
  //1-Check if there is a difference between the current physical and virtual memory maps (apart from kernel segments).
  //  If so, calculate the amount of new memory map items required.
  //2-Determine if allocation is necessary. If so, allocate the required amount of items + n where n is the amount of physical
  //  pages potentially created by the allocation process.
  //3-Apply changes.
  //
  //TODO : Code a variant of alloc_mapitems() where the user can specify the amount of memory map items requested.
}

VirMemManager::VirMemManager(PhyMemManager& physmem) {
  //This function...
  //  1/Determines the amount of memory necessary to store the management structures
  //  2/Allocate this amount of space.
  //  3/Fill those stuctures using...
  //     -Chunks from the physical memory map
  //     -Flags from the page table
  
  addr_t map_size, usedmem_current, usedmem_total;
  PhyMemMap *phy_mmap, *current_phyitem, *allocated_mapchunk, *allocated_listchunk;
  VirMemMap *current_item, *buddy_lookup;
  VirMapList *current_lsitem;
  
  //Store a pointer to the physical memory manager, for future access.
  phymem = &physmem;
  
  //See how much storage space we'll need. We'll need one VirMapList plus one VirMemMap per physical memory map item.
  //Up to three physical memory map items can be created : one because of the VirMemMap() items created, one because of
  //the VirMapList() items created, and one in case PhyMemManager itself has to allocate some memory.
  //We also need up to 6 other VirMemMap items, one per kernel segment, plus one extra one which might be needed in case of overlap.
  map_size = 9*sizeof(VirMemMap);
  phy_mmap = phymem->dump_mmap();
  current_phyitem = phy_mmap;
  while(current_phyitem) {
    map_size+= sizeof(VirMemMap);
    current_phyitem = current_phyitem->next_mapitem;
  }
  
  //Now use the most sensible method to allocate our storage space. Assume allocation is successful,
  //putting a check here would be complicated and it should not fail anyway. Then update phy_mmap.
  if(map_size < PG_SIZE) allocated_mapchunk = phymem->alloc_page(PID_KERNEL);
  else allocated_mapchunk = phymem->alloc_chunk(PID_KERNEL, map_size);
  allocated_listchunk = phymem->alloc_page(PID_KERNEL);
  phy_mmap = phymem->dump_mmap();
  
  //Then allocate memory map items and store them in free_mapitems.
  //The non-contiguous nature of physical memory chunks makes it a bit more complex than it could be.
  current_item = (VirMemMap*) (allocated_mapchunk->location);
  free_mapitems = current_item;
  usedmem_total = 0;
  while(allocated_mapchunk) {
    for(usedmem_current = sizeof(VirMemMap); usedmem_current < allocated_mapchunk->size; usedmem_current+= sizeof(VirMemMap)) {
      *current_item = VirMemMap();
      current_item->next_buddy = current_item+1;
      ++current_item;
    }
    //Move to next part of the memory chunk, actualize used memory amount
    allocated_mapchunk = allocated_mapchunk->next_buddy;
    usedmem_total+= usedmem_current;
    //Actualize current item
    if(allocated_mapchunk) {
      --current_item;
      current_item->next_buddy = (VirMemMap*) (allocated_mapchunk->location);
      current_item = current_item->next_buddy;
    } else {
      *current_item = VirMemMap();
      current_item->next_buddy = NULL;
    }
  }
  
  //Then allocate our map list items and store them in free_listitems.
  current_lsitem = (VirMapList*) (allocated_listchunk->location);
  free_listitems = current_lsitem;
  for(usedmem_current = sizeof(VirMapList); usedmem_current < allocated_listchunk->size; usedmem_current+= sizeof(VirMapList)) {
    *current_lsitem = VirMapList();
    current_lsitem->next_item = current_lsitem+1;
    ++current_lsitem;
  }
  *current_lsitem = VirMapList();
  current_lsitem->next_item = NULL;
  
  //Among those, we'll need one item, the identity-mapped kernel virtual memory map
  map_list = free_listitems;
  free_listitems = free_listitems->next_item;
  map_list->map_owner = PID_KERNEL;
  map_list->next_item = NULL;
  map_list->pml4t_location = get_pml4t();
  
  //Then fill the kernel's identity-paged virtual memory map
  current_phyitem = phy_mmap;
  map_list->map_pointer = free_mapitems;
  current_item = free_mapitems; //free_mapitems will be updated later
  while(current_phyitem) {
    //Fill data from the physical memory chunk
    current_item->location = current_phyitem->location;
    current_item->size = current_phyitem->size;
    current_item->owners = current_phyitem->owners;
    current_item->points_to = current_phyitem;
    //Fill flags from the page table data
    current_item->flags = get_current_flags(current_phyitem->location);
    //Move to next chunk in both maps or update free_mapitems
    current_phyitem = current_phyitem->next_mapitem;
    if(current_phyitem) {
      current_item->next_mapitem = current_item->next_buddy;
      current_item->next_buddy = NULL;
      current_item = current_item->next_mapitem;
    } else {
      free_mapitems = current_item->next_buddy;
      current_item->next_buddy = NULL;
    }
  }
  
  //Though the kernel's virtual address space is mostly identity-mapped, an exception is the kernel itself, mapped in the higher-half
  //of memory. Let's put it in our map.
  map_kernel(phy_mmap);
  
  //Manage buddies
  for(current_item = map_list->map_pointer; current_item; current_item = current_item->next_mapitem) {
    if(current_item->points_to->next_buddy == NULL) continue;
    current_phyitem = current_item->points_to->next_buddy;
    for(buddy_lookup = map_list->map_pointer; buddy_lookup; buddy_lookup = buddy_lookup->next_mapitem) {
      if(buddy_lookup->points_to == current_phyitem) break;
    }
    current_item->next_buddy = buddy_lookup;
  }
}

VirMemMap* VirMemManager::map_chunk(PhyMemMap* phys_chunk, VirMemFlags flags, PID target) {
  VirMapList* list_item = map_list;
  
  if(target == PID_KERNEL) update_kernel_mmap();
  
  maplist_mutex.grab_spin();
    while(list_item) {
      if(list_item->map_owner == target) break;
      list_item = list_item->next_item;
    }
    if(!list_item) {
      //Create a list item corresponding to the target PID
    }
  maplist_mutex.release();
  
  return 0;
}
