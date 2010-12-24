 /* Support classes for all code that manages chunks (contiguous or non-contiguous) of memory

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

#include <address.h>
#include <memory_support.h>

PhyMemMap* PhyMemMap::find_contigchunk(const addr_t requested_size) const {
    PhyMemMap* current_item = (PhyMemMap*) this;
    PhyMemMap* result = NULL;
    addr_t current_size = 0, next_location = 0;
    if(current_item->location > location) return NULL;
    
    //Explore the map, looking for contiguous chunks of free memory
    while(current_item) {
        if(current_size) {
            if((current_item->location == next_location) && (current_item->owners[0]==PID_NOBODY)) {
                //We're in the middle of a contiguous chunk of free memory.
                current_size+= current_item->size;
                next_location = current_item->location+current_item->size;
            } else {
                //This is the end of that chunk of free memory and it was not large enough.
                //Let's start over.
                current_size = 0;
                next_location = 0;
            }
        }
        if((!current_size) && (current_item->owners[0]==PID_NOBODY)) {
            //We've found something which might be the beginning of a contiguous chunk
            current_size = current_item->size;
            next_location = current_item->location+current_item->size;
        }
        if(current_size >= requested_size) break; //We found a large enough chunk of memory
        current_item = current_item->next_mapitem;
    }
    return result;
}

PhyMemMap* PhyMemMap::find_thischunk(const addr_t location) const {
    PhyMemMap* current_item = (PhyMemMap*) this;
    if(current_item->location > location) return NULL;
    
    while(current_item) {
        if(current_item->location == location) break;
        current_item = current_item->next_mapitem;
    }
    return current_item;
}

unsigned int PhyMemMap::length() const {
    unsigned int result;
    PhyMemMap* current_item = (PhyMemMap*) this;
    
    while(current_item) {
        ++result;
        current_item = current_item->next_mapitem;
    }
    return result;
}

bool PhyMemMap::operator==(const PhyMemMap& param) const {
    if(location != param.location) return false;
    if(size != param.size) return false;
    if(owners != param.owners) return false;
    if(allocatable != param.allocatable) return false;
    if(next_buddy != param.next_buddy) return false;
    if(next_mapitem != param.next_mapitem) return false;
    return true;
}

VirMemMap* VirMemMap::find_thischunk(const addr_t location) const {
    VirMemMap* current_item = (VirMemMap*) this;
    if(current_item->location > location) return NULL;
    
    while(current_item) {
        if(current_item->location == location) break;
        current_item = current_item->next_mapitem;
    }
    return current_item;
}

unsigned int VirMemMap::length() const {
    unsigned int result;
    VirMemMap* current_item = (VirMemMap*) this;
    
    while(current_item) {
        ++result;
        current_item = current_item->next_mapitem;
    }
    return result;
}

MallocMap* MallocMap::find_contigchunk(const addr_t requested_size) const {
    MallocMap* current_item = (MallocMap*) this;
    
    while(current_item) {
        if(current_item->size >= requested_size) break;
        current_item = current_item->next_item;
    }
    return current_item;
}

MallocMap* MallocMap::find_contigchunk(const addr_t requested_size, const VirMemFlags flags) const {
    MallocMap* current_item = (MallocMap*) this;
    
    while(current_item) {
        if((current_item->size >= requested_size) && (current_item->belongs_to->flags == flags)) {
            break;
        }
        current_item = current_item->next_item;
    }
    return current_item;
}

MallocMap* MallocMap::find_thischunk(const addr_t location) const {
    MallocMap* current_item = (MallocMap*) this;
    
    while(current_item) {
        if(current_item->size == location) break;
        current_item = current_item->next_item;
    }
    return current_item;
}

KnlMallocMap* KnlMallocMap::find_contigchunk(const addr_t requested_size) const {
    KnlMallocMap* current_item = (KnlMallocMap*) this;
    
    while(current_item) {
        if(current_item->size >= requested_size) break;
        current_item = current_item->next_item;
    }
    return current_item;
}

KnlMallocMap* KnlMallocMap::find_thischunk(const addr_t location) const {
    KnlMallocMap* current_item = (KnlMallocMap*) this;
    
    while(current_item) {
        if(current_item->size == location) break;
        current_item = current_item->next_item;
    }
    return current_item;
}
