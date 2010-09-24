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

VirMemManager::VirMemManager(PhyMemManager& physmem) {
  //This function...
  //  1/Determines the amount of memory necessary to store the management structures
  //  2/Allocate this amount of space.
  //  3/Fill those stuctures using...
  //     -Chunks from the physical memory map
  //     -Flags from the page table
  
  uint64_t pgstruct_entry;
  addr_t mgmt_size, usedmem_current, usedmem_total;
  PhyMemMap *phy_mmap, *current_phyitem, *allocated_chunk;
  VirMemMap *current_item;
  
  //Store a pointer to the physical memory manager, for future access.
  phymem = &physmem;
  
  //See how much storage space we'll need. We'll need one VirMapList plus one VirMemMap per physical memory map item (take account of
  //the physical memory chunk created later)
  mgmt_size = sizeof(VirMapList) + sizeof(VirMemMap);
  phy_mmap = phymem->dump_mmap();
  current_phyitem = phy_mmap;
  while(current_phyitem) {
    mgmt_size+= sizeof(VirMemMap);
    current_phyitem = current_phyitem->next_mapitem;
  }
  
  //Now use the most sensible method to allocate our storage space. Assume allocation is successful,
  //putting a check here would be complicated and it should not fail anyway. Then update phy_mmap.
  if(mgmt_size < PG_SIZE) allocated_chunk = phymem->alloc_page(PID_KERNEL);
  else allocated_chunk = phymem->alloc_chunk(PID_KERNEL, mgmt_size);
  phy_mmap = phymem->dump_mmap();
  
  //Then allocate memory map items and store them in free_mapitems.
  //The non-contiguous nature of physical memory chunks makes it a bit more complex than it could be.
  current_item = (VirMemMap*) (allocated_chunk->location);
  free_mapitems = current_item;
  usedmem_total = 0;
  while(usedmem_total < mgmt_size) {
    for(usedmem_current = sizeof(VirMemMap); usedmem_current < allocated_chunk->size; usedmem_current+= sizeof(VirMemMap)) {
      *current_item = VirMemMap();
      current_item->next_buddy = current_item+1;
      ++current_item;
    }
    //Move to next part of the memory chunk, actualize used memory amount
    allocated_chunk = allocated_chunk->next_buddy;
    usedmem_total+= usedmem_current;
    //Actualize current item
    if(usedmem_total < mgmt_size) {
      --current_item;
      current_item->next_buddy = (VirMemMap*) (allocated_chunk->location);
      current_item = current_item->next_buddy;
    } else {
      *current_item = VirMemMap();
      current_item->next_buddy = NULL;
    }
  }
  
  //Then allocate our map list (one item, the identity-mapped kernel virtual memory map)
  map_list = (VirMapList*) (current_item+1);
  *map_list = VirMapList();
  map_list->map_owner = PID_KERNEL;
  usedmem_total = usedmem_total - sizeof(VirMemMap) + sizeof(VirMapList);
  usedmem_current = usedmem_current - sizeof(VirMemMap) + sizeof(VirMapList);
  dbgout << "Using theoretically " << mgmt_size << " and practically " << usedmem_total << endl;
  
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
    current_item->flags = 0;
    pgstruct_entry = find_lowestpaging(current_phyitem->location);
    if(pgstruct_entry) {
      if(pgstruct_entry & PBIT_PRESENT) current_item->flags|= VMEM_FLAG_P + VMEM_FLAG_R;
      if(pgstruct_entry & PBIT_WRITABLE) current_item->flags|= VMEM_FLAG_R + VMEM_FLAG_W;
      if(!(pgstruct_entry & PBIT_NOEXECUTE)) current_item->flags|= VMEM_FLAG_X;
    }
    //Move to next chunk in both maps
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
  
  dbgout << set_window(screen_win) << *phy_mmap << bp() << *(map_list->map_pointer);
  //TODO : *Manage buddies (will probably require to parse the physical memory map one more time per chunk)
}
