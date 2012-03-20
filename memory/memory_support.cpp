 /* Support classes for all code that manages chunks (contiguous or non-contiguous) of memory

    Copyright (C) 2010-2011 Hadrien Grasland

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

#include <address.h>
#include <memory_support.h>


bool PIDs::has_pid(const PID& the_pid) const {
    PIDs* parser = (PIDs*) this;

    while(parser) {
        if(parser->current_pid == the_pid) return true;
        parser = parser->next_item;
    }
    return false;
}

unsigned int PIDs::length() const {
    PIDs* parser = (PIDs*) this;
    unsigned int result;

    for(result = 0; parser; ++result, parser = parser->next_item);
    return result;
}

PIDs& PIDs::operator=(const PID& param) {
    current_pid = param;

    return *this;
}


bool PIDs::operator==(const PIDs& param) const {
    PIDs *source, *dest;
    //This function compares two lists of PIDs, checking if each element of one is in the other.
    //(The algorithm is O(N^2), but sharing should not often occur between more than ~10 processes)

    source = (PIDs*) this;
    while(source) {
        dest = (PIDs*) &param;
        while(dest) {
            if(dest->current_pid == source->current_pid) break;
            dest = dest->next_item;
        }
        if(!dest) return false;
        source = source->next_item;
    }

    source = (PIDs*) &param;
    while(source) {
        dest = (PIDs*) this;
        while(dest) {
            if(dest->current_pid == source->current_pid) break;
            dest = dest->next_item;
        }
        if(!dest) return false;
        source = source->next_item;
    }

    return true;
}

PhyMemChunk* PhyMemChunk::find_contigchunk(const size_t requested_size) const {
    PhyMemChunk* current_item = (PhyMemChunk*) this;
    PhyMemChunk* result = NULL;
    size_t current_size = 0, next_location = 0;

    //Explore the map, looking for contiguous chunks of free memory
    while(current_item) {
        if(current_size) {
            if((current_item->location == next_location) && (current_item->has_owner(PID_INVALID))) {
                //We're in the middle of a contiguous chunk of free memory.
                current_size+= current_item->size;
                next_location = current_item->location+current_item->size;
            } else {
                //This is the end of that chunk of free memory and it was not large enough.
                //Let's start over.
                result = NULL;
                current_size = 0;
                next_location = 0;
            }
        }
        if((!current_size) && (current_item->has_owner(PID_INVALID))) {
            //We've found something which might be the beginning of a contiguous chunk
            result = current_item;
            current_size = current_item->size;
            next_location = current_item->location+current_item->size;
        }
        if(current_size >= requested_size) break; //We found a large enough chunk of memory
        current_item = current_item->next_mapitem;
    }
    if(current_size < requested_size) result = NULL;

    return result;
}

PhyMemChunk* PhyMemChunk::find_thischunk(const size_t location) const {
    PhyMemChunk* current_item = (PhyMemChunk*) this;
    if(current_item->location > location) return NULL;

    while(current_item) {
        if(current_item->location == location) break;
        current_item = current_item->next_mapitem;
    }

    return current_item;
}

unsigned int PhyMemChunk::buddy_length() const {
    unsigned int result = 0;
    PhyMemChunk* current_item = (PhyMemChunk*) this;

    while(current_item) {
        ++result;
        current_item = current_item->next_buddy;
    }

    return result;
}

unsigned int PhyMemChunk::length() const {
    unsigned int result = 0;
    PhyMemChunk* current_item = (PhyMemChunk*) this;

    while(current_item) {
        ++result;
        current_item = current_item->next_mapitem;
    }

    return result;
}

bool PhyMemChunk::operator==(const PhyMemChunk& param) const {
    if(location != param.location) return false;
    if(size != param.size) return false;
    if(owners != param.owners) return false;
    if(allocatable != param.allocatable) return false;
    if(next_buddy != param.next_buddy) return false;
    if(next_mapitem != param.next_mapitem) return false;

    return true;
}

VirMemChunk* VirMemChunk::find_thischunk(const size_t location) const {
    VirMemChunk* current_item = (VirMemChunk*) this;
    if(current_item->location > location) return NULL;

    while(current_item) {
        if(current_item->location == location) break;
        current_item = current_item->next_mapitem;
    }

    return current_item;
}

unsigned int VirMemChunk::length() const {
    unsigned int result;
    VirMemChunk* current_item = (VirMemChunk*) this;

    while(current_item) {
        ++result;
        current_item = current_item->next_mapitem;
    }

    return result;
}

bool VirMemChunk::operator==(const VirMemChunk& param) const {
    if(location != param.location) return false;
    if(size != param.size) return false;
    if(flags != param.flags) return false;
    if(points_to != param.points_to) return false;
    if(next_buddy != param.next_buddy) return false;
    if(next_mapitem != param.next_mapitem) return false;

    return true;
}

bool VirMemProcess::operator==(const VirMemProcess& param) const {
    if(owner != param.owner) return false;
    if(map_pointer != param.map_pointer) return false;
    if(pml4t_location != param.pml4t_location) return false;
    if(next_item != param.next_item) return false;
    if(mutex != param.mutex) return false;
    if(may_free_kpages != param.may_free_kpages) return false;

    return true;
}

MemoryChunk* MemoryChunk::find_contigchunk(const size_t requested_size) const {
    MemoryChunk* current_item = (MemoryChunk*) this;

    while(current_item) {
        if(current_item->size >= requested_size) break;
        current_item = current_item->next_item;
    }

    return current_item;
}

MemoryChunk* MemoryChunk::find_contigchunk(const size_t requested_size, const VirMemFlags flags) const {
    MemoryChunk* current_item = (MemoryChunk*) this;

    while(current_item) {
        if((current_item->size >= requested_size) && (current_item->belongs_to->flags == flags)) {
            break;
        }
        current_item = current_item->next_item;
    }

    return current_item;
}

MemoryChunk* MemoryChunk::find_thischunk(const size_t location) const {
    MemoryChunk* current_item = (MemoryChunk*) this;

    while(current_item) {
        if((current_item->location + current_item->size > location) && (current_item->location <= location)) break;
        current_item = current_item->next_item;
    }

    return current_item;
}

bool MemoryChunk::operator==(const MemoryChunk& param) const {
    if(location != param.location) return false;
    if(size != param.size) return false;
    if(belongs_to != param.belongs_to) return false;
    if(next_item != param.next_item) return false;
    if(shareable != param.shareable) return false;
    if(share_count != param.share_count) return false;

    return true;
}

bool MallocProcess::operator==(const MallocProcess& param) const {
    if(owner != param.owner) return false;
    if(free_map != param.free_map) return false;
    if(busy_map != param.busy_map) return false;
    if(next_item != param.next_item) return false;
    if(mutex != param.mutex) return false;
    if(pool_stack != param.pool_stack) return false;
    if(pool_location != param.pool_location) return false;
    if(pool_size != param.pool_size) return false;

    return true;
}
