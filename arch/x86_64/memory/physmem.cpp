 /* Physical memory management, ie managing pages of RAM

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
#include <physmem.h>

addr_t PhyMemManager::alloc_storage_space() {
  addr_t chunksize_left, location, remaining_space;
  MemMap *current_item, *free_region, *next_region;
  
  //Find an available chunk of memory
  free_region = first_highhole;
  if(!free_region) return NULL;
  
  //We only need one page in this chunk : adjust the properties of the chunk we just found
  location = free_region->location;
  chunksize_left = free_region->size - PG_SIZE;
  free_region->size = PG_SIZE;
  free_region->add_owner(PID_KERNEL);
  
  //Store our brand new free memory map items in free_mapitems
  current_item = (MemMap*) location;
  free_mapitems = current_item;
  for(remaining_space=PG_SIZE-sizeof(MemMap); remaining_space; remaining_space-= sizeof(MemMap)) {
    current_item->next_item = current_item+1;
    ++current_item;
  }
  current_item->next_item = NULL;
  
  //Store what remains unallocated in the initial chunk in our memory map
  if(chunksize_left) {
    next_region = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    *next_region = MemMap();
    next_region->location = location+PG_SIZE;
    next_region->size = chunksize_left;
    next_region->next_item = free_region->next_item;
    free_region->next_item = next_region;
  }
  
  //Update memory hole information
  first_highhole = first_highhole->find_freechunk();
  
  return location;
}


MemMap* PhyMemManager::chunk_allocator(MemMap* map_used, PID initial_owner, addr_t size) {
  addr_t chunksize_left, remaining_allocsize;
  MemMap *current_item, *new_item, *previous_item, *result;
  
  //Grab some memory map storage space if needed
  if(!free_mapitems) {
    if(!alloc_storage_space()) return NULL; //Memory is full
  }
  
  //Allocate a free region of memory
  if(map_used == phy_highmmap) {
    current_item = first_highhole;
  } else {
    current_item = first_hole;
  }
  if(!current_item) return NULL;
  result = current_item;
  current_item->add_owner(initial_owner);
  
  //Three situations may occur : there is either too much, enough, or too little space
  //in the memory chunk we found
  if(current_item->size > size) chunksize_left = current_item->size-size;
  else chunksize_left = 0;
  if(current_item->size < size) remaining_allocsize = size-current_item->size;
  else remaining_allocsize = 0;
  
  //If there's too little space, continue allocation until there is enough allocated space.
  while(remaining_allocsize) {
    previous_item = current_item;
    current_item = current_item->find_freechunk();
    if(!current_item) {
      free(result->location);
      return NULL;
    }
    //Same test as before, but with remaining size to be allocated
    if(current_item->size > remaining_allocsize) chunksize_left = current_item->size-remaining_allocsize;
    else chunksize_left = 0;
    if(current_item->size < remaining_allocsize) remaining_allocsize-= current_item->size;
    else remaining_allocsize = 0;
    //Connect the new chunk with the rest of the allocated memory.
    if(current_item == previous_item->next_item) {
      current_item = merge_with_next(previous_item);
    } else {
      previous_item->next_buddy = current_item;
      current_item->add_owner(initial_owner);
    }
  }
  current_item->next_buddy = NULL;
  
  //Now, manage what happens if there's too much allocated space in the last allocated chunk.
  if(chunksize_left) {
    current_item->size-= chunksize_left;
    new_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    *new_item = MemMap();
    new_item->location = current_item->location+current_item->size;
    new_item->size = chunksize_left;
    new_item->next_item = current_item->next_item;
    current_item->next_item = new_item;
  }
  
  //Update first hole information
  if(map_used == phy_highmmap) {
    first_highhole = current_item->find_freechunk();
  } else {
    first_hole = current_item->find_freechunk();
  }
  return result;
}


MemMap* PhyMemManager::merge_with_next(MemMap* first_item) {
  MemMap* next_item = first_item->next_item;
  
  //We assume that the first element and his neighbour are really identical.
  first_item->size+= next_item->size;
  first_item->next_item = next_item->next_item;
  
  //Now, trash "next_item" in our free_mapitems structures.
  next_item->next_item = free_mapitems;
  free_mapitems = next_item;
  return first_item;
}


addr_t PhyMemManager::page_allocator(MemMap* map_used, PID initial_owner) {
  addr_t location, chunksize_left;
  MemMap *free_region, *new_item;

  //Grab some management structure storage space if needed
  if(!free_mapitems) {
    if(!alloc_storage_space()) return NULL; //Memory is full
  }
  
  //Find a free region of memory of at least one page (means any free region of memory since
  //our map of memory has its locations and sizes page-aligned)
  if(map_used == phy_highmmap) {
    free_region = first_highhole;
  } else {
    free_region = first_hole;
  }
  if(!free_region) return NULL;
  
  //Adjust memory chunks properties and allocate new memory map item if needed
  location = free_region->location;
  chunksize_left = free_region->size-PG_SIZE;
  free_region->size = PG_SIZE;
  free_region->add_owner(initial_owner);
  free_region->next_buddy = NULL;
  if(chunksize_left) {
    new_item = free_mapitems;
    free_mapitems = free_mapitems->next_item;
    *new_item = MemMap();
    new_item->location = location+PG_SIZE;
    new_item->size = chunksize_left;
    new_item->next_item = free_region->next_item;
    free_region->next_item = new_item;
  }
  
  //Update first hole information
  if(map_used == phy_highmmap) {
    first_highhole = first_highhole->find_freechunk();
  } else {
    first_hole = first_hole->find_freechunk();
  }
  if(location) return location; else return 1;
}


PhyMemManager::PhyMemManager(KernelInformation& kinfo) {
  //This function...
  //  1/Determines the amount of memory necessary to store the management structures
  //  2/Find this amount of free space in the memory map
  //  3/Fill those stuctures, including with themselves, using the following rules in priority order
  //       -Mark reserved memory as non-allocatable
  //       -Pages of nature Bootstrap and Kernel belong to the kernel
  //       -Pages of nature Free and Reserved belong to nobody (PID_NOBODY)
  
  addr_t phymmap_location, phymmap_size, current_location, next_location;
  unsigned int index, storage_index, remaining_space; //Remaining space in the allocated chunk
  KernelMemoryMap* kmmap = kinfo.kmmap;
  MemMap* current_item;
  
  //We'll allocate the maximum amount of memory that we can possibly need.
  //More economic options exist, but they're much more complicated too, for a pretty small
  //benefit in the end.
  phymmap_size = align_pgup((kinfo.kmmap_size+2)*sizeof(MemMap));
  
  //Find an empty chunk of high memory large enough to store our mess...
  for(index=0; index<kinfo.kmmap_size; ++index) {
    if(kmmap[index].location < 0x100000) continue;
    if(kmmap[index].nature != NATURE_FRE) continue;
    if((kmmap[index].location+kmmap[index].size)-align_pgup(kmmap[index].location) >= phymmap_size) break;
  }
  //We suppose there's enough space in main memory. At this stade of the boot process,
  //it's a reasonable assumption.
  storage_index = index;
  phymmap_location = align_pgup(kmmap[index].location);
  
  //Setup management structures
  current_location = align_pgup(kmmap[0].location);
  next_location = align_pgup(kmmap[0].location+kmmap[0].size);
  phy_mmap = (MemMap*) phymmap_location;
  remaining_space = phymmap_size-sizeof(MemMap);
  current_item = phy_mmap;
  *current_item = MemMap();
  //Fill first item
  current_item->location = current_location;
  current_item->size = next_location-current_location;
  current_item->allocatable = !(kmmap[0].nature == NATURE_RES);
  switch(kmmap[0].nature) {
    case NATURE_BSK:
    case NATURE_KNL:
      current_item->add_owner(PID_KERNEL);
  }
  
  //Fill management structures until we reach the index where we're storing our data
  for(index=1; index<storage_index; ++index) {
    if(kmmap[index].location<next_location) {
      //Update memory map chunk
      if(kmmap[index].nature == NATURE_RES) current_item->allocatable = false;
      switch(kmmap[index].nature) {
        case NATURE_BSK:
        case NATURE_KNL:
          current_item->add_owner(PID_KERNEL);
      }
    }
    if(kmmap[index].location+kmmap[index].size>next_location) {
      //Onto the next memory map chunk
      current_location = align_pgup(kmmap[index].location);
      next_location = align_pgup(kmmap[index].location+kmmap[index].size);
      if(next_location == current_location) continue;
      current_item->next_item = current_item+1;
      ++current_item;
      remaining_space-=sizeof(MemMap);
      *current_item = MemMap();
      //Fill this new physical mmap item
      current_item->location = current_location;
      current_item->size = next_location-current_location;
      current_item->allocatable = !(kmmap[index].nature == NATURE_RES);
      switch(kmmap[index].nature) {
        case NATURE_BSK:
        case NATURE_KNL:
          current_item->add_owner(PID_KERNEL);
      }
    }
  }
  
  // Insert our freshly allocated map of physical memory
  current_item->next_item = current_item+1;
  ++current_item;
  remaining_space-=sizeof(MemMap);
  *current_item = MemMap();
  current_item->location = phymmap_location;
  current_item->size = phymmap_size;
  current_item->add_owner(PID_KERNEL);
  next_location = align_pgup(kmmap[storage_index].location+kmmap[storage_index].size);
  //Add up what remains of the storage space being used
  if(phymmap_location+phymmap_size < kmmap[storage_index].location+kmmap[storage_index].size) {
    //Onto the next memory map chunk
    current_item->next_item = current_item+1;
    ++current_item;
    remaining_space-=sizeof(MemMap);
    *current_item = MemMap();
    current_item->location = phymmap_location+phymmap_size;
    current_item->size = next_location-phymmap_location-phymmap_size;
  }
  
  // Continue filling.
  for(index=storage_index+1; index<kinfo.kmmap_size; ++index) {
    if(kmmap[index].location<next_location) {
      //Update memory map chunk
      if(kmmap[index].nature == NATURE_RES) current_item->allocatable = false;
      switch(kmmap[index].nature) {
        case NATURE_BSK:
        case NATURE_KNL:
          current_item->add_owner(PID_KERNEL);
      }
    }
    if(kmmap[index].location+kmmap[index].size>next_location) {
      //Onto the next memory map chunk
      current_location = align_pgup(kmmap[index].location);
      next_location = align_pgup(kmmap[index].location+kmmap[index].size);
      if(next_location == current_location) continue;
      current_item->next_item = current_item+1;
      ++current_item;
      remaining_space-=sizeof(MemMap);
      *current_item = MemMap();
      //Fill this new physical mmap item
      current_item->location = current_location;
      current_item->size = next_location-current_location;
      current_item->allocatable = !(kmmap[index].nature == NATURE_RES);
      switch(kmmap[index].nature) {
        case NATURE_BSK:
        case NATURE_KNL:
          current_item->add_owner(PID_KERNEL);
      }
    }
  }
  
  //Store remaining memory map items in free_mapitems
  if(remaining_space) {
    ++current_item;
    remaining_space-= sizeof(MemMap);
    free_mapitems = current_item;
    for(; remaining_space; remaining_space-= sizeof(MemMap)) {
      current_item->next_item = current_item+1;
      ++current_item;
    }
    current_item->next_item = NULL;
  }
  
  //Locate the beginning of high memory
  current_item = phy_mmap;
  while(current_item->location < 0x100000) ++current_item;
  phy_highmmap = current_item;
  //Locate first holes in low and high memory
  first_hole = phy_mmap->find_freechunk();
  first_highhole = phy_highmmap->find_freechunk();
}


MemMap* PhyMemManager::alloc_chunk(PID initial_owner, addr_t size) {
  return chunk_allocator(phy_highmmap, initial_owner, align_pgup(size));
}


addr_t PhyMemManager::alloc_page(PID initial_owner) {
  return page_allocator(phy_highmmap, initial_owner);
}


addr_t PhyMemManager::free(addr_t location) {
  addr_t page_location;
  MemMap *current_item, *previous_item;
  
  //All operations on the memory map must be done under mutual exclusion, for
  //obvious consistency reasons.
  mmap_mutex.grab_spin();
  
  //Find the region of memory map where this location belongs
  page_location = align_pgdown(location);
  current_item = phy_mmap->find_thischunk(page_location);
  if(!current_item) {
    mmap_mutex.release();
    return NULL;
  }
  //Clear the owner list of this chunk, along with its buddies.
  do {
    current_item->clear_owners();
    previous_item = current_item;
    current_item = current_item->next_buddy;
    previous_item->next_buddy = NULL;
  } while(current_item);
  //Freeing memory is generally less urgent than allocating memory,
  //so that the use of a slow hole-finding algorithm does not come with
  //a very high performance hit
  if(location>0x100000) {
    first_highhole = phy_highmmap->find_freechunk();
  } else {
    first_hole = phy_mmap->find_freechunk();
  }
  mmap_mutex.release();
  return location;
}


MemMap* PhyMemManager::alloc_lowchunk(PID initial_owner, addr_t size) {
  MemMap *result, *lowmem_end = phy_mmap;
  
  //All operations on the memory map must be done under mutual exclusion, for
  //obvious consistency reasons.
  mmap_mutex.grab_spin();
  
  //Find the end of low memory
  while(lowmem_end->next_item!=phy_highmmap) lowmem_end = lowmem_end->next_item;
  //Temporarily make high memory disappear
  lowmem_end->next_item = NULL;
  
  //Do the allocation job
  result = chunk_allocator(phy_mmap, initial_owner, align_pgup(size));
  
  //Make high memory come back
  lowmem_end->next_item = phy_highmmap;
  mmap_mutex.release();
  return result;
}


addr_t PhyMemManager::alloc_lowpage(PID initial_owner) {
  addr_t result;
  MemMap* lowmem_end = phy_mmap;
  
  //All operations on the memory map must be done under mutual exclusion, for
  //obvious consistency reasons.
  mmap_mutex.grab_spin();
  
  //Find the end of low memory
  while(lowmem_end->next_item!=phy_highmmap) lowmem_end = lowmem_end->next_item;
  //Temporarily make high memory disappear
  lowmem_end->next_item = NULL;
  
  //Do the allocation job
  result = page_allocator(phy_mmap, initial_owner);
  
  //Make high memory come back
  lowmem_end->next_item = phy_highmmap;
  mmap_mutex.release();
  return result;
}


void PhyMemManager::print_lowmmap() {
  MemMap* lowmem_end = phy_mmap;
  
  //All operations on the memory map must be done under mutual exclusion, for
  //obvious consistency reasons.
  mmap_mutex.grab_spin();
  
  //Find the end of low memory
  while(lowmem_end->next_item!=phy_highmmap) lowmem_end = lowmem_end->next_item;
  //Temporarily make high memory disappear
  lowmem_end->next_item = NULL;
  
  //Print memory map
  dbgout << *phy_mmap;
  
  //Make high memory come back
  lowmem_end->next_item = phy_highmmap;
  mmap_mutex.release();
}


void PhyMemManager::print_highmmap() {
  //All operations on the memory map must be done under mutual exclusion, for
  //obvious consistency reasons.
  mmap_mutex.grab_spin();
  dbgout << *phy_highmmap;
  mmap_mutex.release();
}
