 /* Physical memory management, ie managing pages of RAM

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
#include <new.h>
#include <physmem.h>

#include <dbgstream.h>

bool PhyMemManager::alloc_mapitems() {
    size_t remaining_freemem, used_space;
    PhyMemMap *allocated_mem, *current_item, *free_mem = NULL;

    //Find an available chunk of memory
    allocated_mem = free_highmem;
    if(!allocated_mem) return false;

    //We only need one page in this chunk : adjust the properties of the chunk we just found
    remaining_freemem = allocated_mem->size - PG_SIZE;
    allocated_mem->size = PG_SIZE;
    allocated_mem->add_owner(PID_KERNEL);

    //Store our brand new free memory map items in the allocated mem
    current_item = (PhyMemMap*) (allocated_mem->location);
    free_mapitems = current_item;
    for(used_space = sizeof(PhyMemMap); used_space<PG_SIZE; used_space+= sizeof(PhyMemMap)) {
        current_item = new(current_item) PhyMemMap();
        current_item->next_buddy = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) PhyMemMap();
    current_item->next_buddy = NULL;

    //Maybe there is some spare memory after allocating our chunk ?
    //If so, don't forget it.
    if(remaining_freemem) {
        free_mem = free_mapitems;
        free_mapitems = (PhyMemMap*) free_mapitems->next_buddy;
        free_mem = new(free_mem) PhyMemMap();
        free_mem->location = allocated_mem->location+PG_SIZE;
        free_mem->size = remaining_freemem;
        free_mem->next_mapitem = allocated_mem->next_mapitem;
        free_mem->next_buddy = allocated_mem->next_buddy;
        allocated_mem->next_mapitem = free_mem;
    }

    //Update free memory information
    if(free_mem) {
        free_highmem = free_mem;
    } else {
        free_highmem = allocated_mem->next_buddy;
    }
    allocated_mem->next_buddy = NULL;

    return true;
}

PhyMemMap* PhyMemManager::chunk_allocator(const PID initial_owner,
                                          const size_t size,
                                          PhyMemMap* map_used,
                                          bool contiguous) {
    size_t remaining_freemem = 0, to_be_allocd = 0;
    PhyMemMap *previous_free = NULL, *new_free = NULL, *current_item, *previous_item, *result;

    //Make sure there's space for storing a new memory map item
    if(!free_mapitems) {
        if(!alloc_mapitems()) return NULL; //Memory is full
    }

    //Put a suitable chunk of memory in current_item. This is the part which differs between
    //contiguous and non-contiguous chunks : for contiguous chunks, we look for a chunk of free
    //memory of the right size (even if it's fragmented). For non-contiguous chunks size does not
    //matter, as long as the buddies are large enough (hmmm...).
    //
    //If there are free chunks before the one which we're going to use, we also need to keep track
    //of it.
    if(!contiguous) {
        if(map_used == phy_highmmap) {
            current_item = free_highmem;
        } else {
            current_item = free_lowmem;
        }
    } else {
        if(map_used == phy_highmmap) {
            if(!free_highmem) return NULL;
            current_item = free_highmem->find_contigchunk(size);
            if(current_item!= free_highmem) {
                previous_free = free_highmem;
                while(previous_free->next_buddy != current_item) {
                    previous_free = previous_free->next_buddy;
                }
            }
        } else {
            if(!free_lowmem) return NULL;
            current_item = free_lowmem->find_contigchunk(size);
            if(current_item!= free_lowmem) {
                previous_free = free_lowmem;
                while(previous_free->next_buddy != current_item) {
                    previous_free = previous_free->next_buddy;
                }
            }
        }
    }
    if(!current_item) return NULL;
    current_item->add_owner(initial_owner);
    result = current_item;

    //Three situations may occur :
    // 1-We allocated too much memory and must correct this.
    // 2-We allocated the right amount of memory.
    // 3-We allocated too little memory and must allocate more.
    //The to_be_allocd and remaining_freemem vars store more detailed information about this.
    if(current_item->size < size) to_be_allocd = size-current_item->size;
    if(current_item->size > size) remaining_freemem = current_item->size-size;

    //First, if we allocated too little memory...
    while(to_be_allocd) {
        //Let's allocate another chunk of memory
        previous_item = current_item;
        current_item = current_item->next_buddy;
        if(!current_item) {
            chunk_liberator(result);
            return NULL;
        }
        current_item->add_owner(initial_owner);

        //Check if the situation changed
        if(current_item->size < to_be_allocd) {
            to_be_allocd-= current_item->size;
        } else {
            if(current_item->size > to_be_allocd) {
                remaining_freemem = current_item->size-to_be_allocd;
            }
            to_be_allocd = 0;
        }

        //If the newly allocated chunk follows the previous one, merge them together
        if(current_item->location == previous_item->location+previous_item->size) {
            merge_with_next(previous_item);
            current_item = previous_item;
        }
    }

    //Then check if we allocated too much memory, and if so correct this
    if(remaining_freemem) {
        current_item->size-= remaining_freemem;
        new_free = free_mapitems;
        free_mapitems = free_mapitems->next_buddy;
        new_free->location = current_item->location+current_item->size;
        new_free->size = remaining_freemem;
        new_free->next_mapitem = current_item->next_mapitem;
        new_free->next_buddy = current_item->next_buddy;
        current_item->next_mapitem = new_free;
    }

    //Update free memory information
    if(!previous_free) {
        if(map_used == phy_highmmap) {
            if(new_free) {
                free_highmem = new_free;
            } else {
                free_highmem = current_item->next_buddy;
            }
        } else {
            if(new_free) {
                free_lowmem = new_free;
            } else {
                free_lowmem = current_item->next_buddy;
            }
        }
    } else {
        if(new_free) {
            previous_free->next_buddy = new_free;
        } else {
            previous_free->next_buddy = current_item->next_buddy;
        }
    }
    current_item->next_buddy = NULL;

    return result;
}

PhyMemMap* PhyMemManager::resvchunk_allocator(const PID initial_owner,
                                              const size_t location) {
    PhyMemMap* requested_chunk = phy_mmap->find_thischunk(location);

    if(requested_chunk &&
       (requested_chunk->allocatable == 0) &&
       (requested_chunk->owners[0] == PID_NOBODY)) {
        bool result = chunk_owneradd(requested_chunk, initial_owner);
        if(result) {
            return requested_chunk;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

bool PhyMemManager::chunk_liberator(PhyMemMap* chunk) {
    PhyMemMap *current_item, *next_item = chunk, *free_item;

    //Now let's free all memory map items in the chunk
    do {
        //Keep track of the next memory map item to be analyzed
        current_item = next_item;
        next_item = current_item->next_buddy;

        //Free current item from its owners and buddies
        current_item->clear_owners();
        current_item->next_buddy = NULL;

        //If it is allocatable, add current item to the appropriate list of free memory map items.
        //Keep that list sorted while doing that, too...
        if(!(current_item->allocatable)) continue;
        if(current_item->location >= 0x100000) {
            //Case where the current item belongs to high memory
            if(free_highmem == NULL) {
                current_item->next_buddy = NULL;
                free_highmem = current_item;
                continue;
            }
            if(free_highmem->location > current_item->location) {
                current_item->next_buddy = free_highmem;
                free_highmem = current_item;
            } else {
                free_item = free_highmem;
                while(free_item->next_buddy) {
                    if(free_item->next_buddy->location > current_item->location) break;
                    free_item = free_item->next_buddy;
                }
                current_item->next_buddy = free_item->next_buddy;
                free_item->next_buddy = current_item;
            }
        } else {
            //Case where the current item belongs to low memory
            if(free_lowmem == NULL) {
                current_item->next_buddy = NULL;
                free_lowmem = current_item;
                continue;
            }
            if(free_lowmem->location > current_item->location) {
                current_item->next_buddy = free_lowmem;
                free_lowmem = current_item;
            } else {
                free_item = free_lowmem;
                while(free_item->next_buddy) {
                    if(free_item->next_buddy->location > current_item->location) break;
                    free_item = free_item->next_buddy;
                }
                current_item->next_buddy = free_item->next_buddy;
                free_item->next_buddy = current_item;
            }
        }
    } while(next_item);

    return true;
}

bool PhyMemManager::chunk_owneradd(PhyMemMap* chunk, const PID new_owner) {
    unsigned int result;
    PhyMemMap* current_item = chunk;

    do {
        result = current_item->add_owner(new_owner);
        if(!result) break;
        current_item = current_item->next_buddy;
    } while(current_item);

    if(!result) {
        //Revert changes and quit
        PhyMemMap* parser = chunk;
        while(parser!=current_item) {
            parser->del_owner(new_owner);
            parser = parser->next_buddy;
        }
        return false;
    }

    return true;
}

bool PhyMemManager::chunk_ownerdel(PhyMemMap* chunk, const PID former_owner) {
    PhyMemMap* current_item = chunk;
    do {
        current_item->del_owner(former_owner);
        current_item = current_item->next_buddy;
    } while(current_item);
    if(chunk->owners[0] == PID_NOBODY) chunk_liberator(chunk);

    return true;
}

void PhyMemManager::merge_with_next(PhyMemMap* first_item) {
    PhyMemMap* next_item = first_item->next_mapitem;

    //We assume that the first element and his neighbour are really identical.
    first_item->size+= next_item->size;
    first_item->next_mapitem = next_item->next_mapitem;
    first_item->next_buddy = next_item->next_buddy;

    //Now, trash "next_mapitem" in our free_mapitems structures.
    next_item = new(next_item) PhyMemMap();
    next_item->next_buddy = free_mapitems;
    free_mapitems = next_item;
}

void PhyMemManager::killer(PID target) {
    PhyMemMap* parser = phy_mmap;

    while(parser) {
        if(parser->has_owner(target)) chunk_ownerdel(parser, target);
        parser = parser->next_mapitem;
    }
}

PhyMemManager::PhyMemManager(const KernelInformation& kinfo) : phy_mmap(NULL),
                                                               phy_highmmap(NULL),
                                                               free_lowmem(NULL),
                                                               free_highmem(NULL),
                                                               free_mapitems(NULL) {
    //This function...
    //  1/Determines the amount of memory necessary to store the management structures
    //  2/Find this amount of free space in the memory map
    //  3/Fill those stuctures, including with themselves, using the following rules
    //      -Mark reserved memory as non-allocatable
    //      -Pages of nature Bootstrap and Kernel belong to the kernel
    //      -Pages of nature Free and Reserved belong to nobody (PID_NOBODY)

    size_t phymmap_location, phymmap_size, current_location, next_location;
    unsigned int index, storage_index, remaining_space; //Remaining space in the allocated chunk
    const KernelMemoryMap* kmmap = kinfo.kmmap;
    PhyMemMap *current_item, *last_free = NULL;

    //We'll allocate the maximum amount of memory that we can possibly need.
    //More economic options exist, but they're much more complicated too, for a pretty small
    //benefit in the end.
    phymmap_size = align_pgup((kinfo.kmmap_size+2)*sizeof(PhyMemMap));

    //Find an empty chunk of high memory large enough to store our mess... We suppose there's one.
    for(index=0; index<kinfo.kmmap_size; ++index) {
        if(kmmap[index].location < 0x100000) continue;
        if(kmmap[index].nature != NATURE_FRE) continue;
        if(kmmap[index].location+kmmap[index].size-align_pgup(kmmap[index].location) >= phymmap_size) {
            break;
        }
    }
    storage_index = index;
    phymmap_location = align_pgup(kmmap[index].location);

    //Allocate map items in this space
    phy_mmap = (PhyMemMap*) phymmap_location;
    remaining_space = phymmap_size-sizeof(PhyMemMap);
    current_item = phy_mmap;
    for(; remaining_space; remaining_space-= sizeof(PhyMemMap)) {
        current_item = new(current_item) PhyMemMap();
        current_item->next_mapitem = current_item+1;
        ++current_item;
    }
    current_item = new(current_item) PhyMemMap();
    current_item->next_mapitem = NULL;

    //Setup variables for the following initialization steps
    current_location = align_pgdown(kmmap[0].location);
    next_location = align_pgup(kmmap[0].location+kmmap[0].size);

    //Fill first item
    current_item = phy_mmap;
    current_item->location = current_location;
    current_item->size = next_location-current_location;
    switch(kmmap[0].nature) {
        case NATURE_RES:
            current_item->allocatable = false;
            break;
        case NATURE_BSK:
        case NATURE_KNL:
            current_item->add_owner(PID_KERNEL);
    }

    //Fill management structures until we reach the index where we're storing our data
    for(index=1; index<storage_index; ++index) {
        if(kmmap[index].location<next_location) {
            //Update memory map chunk
            switch(kmmap[index].nature) {
                case NATURE_RES:
                    current_item->allocatable = false;
                    break;
                case NATURE_BSK:
                case NATURE_KNL:
                    current_item->add_owner(PID_KERNEL);
            }
        }
        if(kmmap[index].location+kmmap[index].size>next_location) {
            //End of the chunk reached !
            //Prepare the next chunk to be filled, skipping chunks of zero size in the way
            if(kmmap[index].nature == NATURE_FRE) {
                current_location = align_pgup(kmmap[index].location);
                next_location = align_pgdown(kmmap[index].location+kmmap[index].size);
            } else {
                current_location = align_pgdown(kmmap[index].location);
                next_location = align_pgup(kmmap[index].location+kmmap[index].size);
            }
            if(next_location <= current_location) continue;
            //If the previous chunk is free, add it to the free memory pool.
            if(current_item->owners[0] == PID_NOBODY && current_item->allocatable) {
                if(!free_lowmem) {
                    free_lowmem = current_item;
                } else {
                    last_free->next_buddy = current_item;
                }
                last_free = current_item;
            }
            //Fill this new physical mmap item
            current_item = current_item->next_mapitem;
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
    current_item = current_item->next_mapitem;
    current_item->location = phymmap_location;
    current_item->size = phymmap_size;
    current_item->add_owner(PID_KERNEL);
    next_location = align_pgup(kmmap[storage_index].location+kmmap[storage_index].size);
    //Add up what remains of the storage space being used (if any) in the map
    if(phymmap_location+phymmap_size < kmmap[storage_index].location+kmmap[storage_index].size) {
        current_item = current_item->next_mapitem;
        current_item->location = phymmap_location+phymmap_size;
        current_item->size = next_location-phymmap_location-phymmap_size;
    }

    // Continue filling.
    for(index=storage_index+1; index<kinfo.kmmap_size; ++index) {
        if(kmmap[index].location<next_location) {
            //Update memory map chunk
            switch(kmmap[index].nature) {
                case NATURE_RES:
                    current_item->allocatable = false;
                    break;
                case NATURE_BSK:
                case NATURE_KNL:
                    current_item->add_owner(PID_KERNEL);
            }
        }
        if(kmmap[index].location+kmmap[index].size>next_location) {
            //End of the chunk reached !
            //Prepare the next chunk to be filled, skipping chunks of zero size in the way
            if(kmmap[index].nature == NATURE_FRE) {
                current_location = align_pgup(kmmap[index].location);
                next_location = align_pgdown(kmmap[index].location+kmmap[index].size);
            } else {
                current_location = align_pgdown(kmmap[index].location);
                next_location = align_pgup(kmmap[index].location+kmmap[index].size);
            }
            if(next_location <= current_location) continue;
            //If the previous chunk is free, add it to the free memory pool.
            if(current_item->owners[0] == PID_NOBODY && current_item->allocatable) {
                if(!free_lowmem) {
                    free_lowmem = current_item;
                } else {
                    last_free->next_buddy = current_item;
                }
                last_free = current_item;
            }
            //Fill this new physical mmap item
            current_item = current_item->next_mapitem;
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

    //Store remaining memory map items as a chunk in free_mapitems
    free_mapitems = current_item->next_mapitem;
    current_item->next_mapitem = NULL;
    current_item = free_mapitems;
    while(current_item) {
        current_item->next_buddy = current_item->next_mapitem;
        current_item->next_mapitem = NULL;
        current_item = current_item->next_buddy;
    }

    //Locate the beginning of high memory and separate free low memory chunk from
    //free high memory chunk.
    current_item = phy_mmap;
    while(current_item->location < 0x100000) {
        if(current_item->owners[0] == PID_NOBODY && current_item->allocatable) {
            last_free = current_item;
        }
        ++current_item;
    }
    free_highmem = last_free->next_buddy;
    last_free->next_buddy = NULL;
    phy_highmmap = current_item;

    //Allocate extra map items right away so that we don't need some more too soon
    PhyMemMap* free_mapitems_backup = free_mapitems;
    alloc_mapitems();
    if(free_mapitems_backup) {
        current_item = free_mapitems_backup;
        while(current_item->next_buddy) current_item = current_item->next_buddy;
        current_item->next_buddy = free_mapitems;
        free_mapitems = free_mapitems_backup;
    }
}

PhyMemMap* PhyMemManager::alloc_page(const PID initial_owner) {
    PhyMemMap* result;

    mmap_mutex.grab_spin();

        result = chunk_allocator(initial_owner, PG_SIZE, phy_highmmap);

    mmap_mutex.release();
    return result;
}

PhyMemMap* PhyMemManager::alloc_chunk(const PID initial_owner, const size_t size, bool contiguous) {
    PhyMemMap* result;

    mmap_mutex.grab_spin();

        result = chunk_allocator(initial_owner,
                                 align_pgup(size),
                                 phy_highmmap,
                                 contiguous);

    mmap_mutex.release();
    return result;
}

PhyMemMap* PhyMemManager::alloc_resvchunk(const size_t location, const PID initial_owner) {
    PhyMemMap* result;

    mmap_mutex.grab_spin();

        result = resvchunk_allocator(initial_owner, location);

    mmap_mutex.release();
    return result;
}

bool PhyMemManager::free(PhyMemMap* chunk) {
    bool result;

    mmap_mutex.grab_spin();

        result = chunk_liberator(chunk);

    mmap_mutex.release();

    return result;
}

bool PhyMemManager::free(size_t chunk_beginning) {
    bool result;
    PhyMemMap* chunk;

    mmap_mutex.grab_spin();

        chunk = phy_mmap->find_thischunk(chunk_beginning);
        if(!chunk) {
            mmap_mutex.release();
            return false;
        }
        result = chunk_liberator(chunk);

    mmap_mutex.release();

    return true;
}

bool PhyMemManager::owneradd(PhyMemMap* chunk, const PID new_owner) {
    bool result;

    mmap_mutex.grab_spin();

        result = chunk_owneradd(chunk, new_owner);

    mmap_mutex.release();

    return result;
}


bool PhyMemManager::ownerdel(PhyMemMap* chunk, const PID former_owner) {
    if(chunk->has_owner(former_owner) == false) return false;
    bool result;

    mmap_mutex.grab_spin();

        result = chunk_ownerdel(chunk, former_owner);

    mmap_mutex.release();

    return result;
}

void PhyMemManager::kill(PID target) {
    if(target == PID_KERNEL) return; //Find a more constructive way to commit suicide.

    mmap_mutex.grab_spin();

        killer(target);

    mmap_mutex.release();
}

PhyMemMap* PhyMemManager::find_thischunk(size_t location) {
    mmap_mutex.grab_spin();

        PhyMemMap* result = phy_mmap->find_thischunk(location);

    mmap_mutex.release();

    return result;
}

PhyMemMap* PhyMemManager::alloc_lowpage(const PID initial_owner) {
    PhyMemMap *result, *lowmem_end = phy_mmap;

    mmap_mutex.grab_spin();

        //Find the end of low memory
        while(lowmem_end->next_mapitem!=phy_highmmap) lowmem_end = lowmem_end->next_mapitem;
        //Temporarily make high memory disappear
        lowmem_end->next_mapitem = NULL;

        //Do the allocation job
        result = chunk_allocator(initial_owner, PG_SIZE, phy_mmap);

        //Make high memory come back
        lowmem_end->next_mapitem = phy_highmmap;

    mmap_mutex.release();

    return result;
}

PhyMemMap* PhyMemManager::alloc_lowchunk(const PID initial_owner,
                                         const size_t size,
                                         bool contiguous) {
    PhyMemMap *result, *lowmem_end = phy_mmap;

    mmap_mutex.grab_spin();

        //Find the end of low memory
        while(lowmem_end->next_mapitem!=phy_highmmap) lowmem_end = lowmem_end->next_mapitem;
        //Temporarily make high memory disappear
        lowmem_end->next_mapitem = NULL;

        //Do the allocation job
        result = chunk_allocator(initial_owner, align_pgup(size), phy_mmap, contiguous);

        //Make high memory come back
        lowmem_end->next_mapitem = phy_highmmap;

    mmap_mutex.release();

    return result;
}

void PhyMemManager::print_highmmap() {
    mmap_mutex.grab_spin();

        dbgout << *phy_highmmap;

    mmap_mutex.release();
}

void PhyMemManager::print_lowmmap() {
    PhyMemMap *lowmem_end = phy_mmap;

    mmap_mutex.grab_spin();

        //Find the end of low memory
        while(lowmem_end->next_mapitem!=phy_highmmap) lowmem_end = lowmem_end->next_mapitem;
        //Temporarily make high memory disappear
        lowmem_end->next_mapitem = NULL;

        dbgout << *phy_mmap;

        //Make high memory come back
        lowmem_end->next_mapitem = phy_highmmap;

    mmap_mutex.release();
}

void PhyMemManager::print_mmap() {
    mmap_mutex.grab_spin();

        dbgout << *phy_mmap;

    mmap_mutex.release();
}
