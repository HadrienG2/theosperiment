 /* Support classes for the memory allocator

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

#include <mallocator_support.h>

MemoryChunk* MemoryChunk::find_contigchunk(const size_t requested_size) const {
    MemoryChunk* current_item = (MemoryChunk*) this;

    while(current_item) {
        if(current_item->size >= requested_size) break;
        current_item = current_item->next_item;
    }

    return current_item;
}

MemoryChunk* MemoryChunk::find_contigchunk(const size_t requested_size, const PageFlags flags) const {
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
    if(identifier != param.identifier) return false;
    if(free_map != param.free_map) return false;
    if(busy_map != param.busy_map) return false;
    if(next_item != param.next_item) return false;
    if(mutex != param.mutex) return false;
    if(pool_stack != param.pool_stack) return false;
    if(pool_location != param.pool_location) return false;
    if(pool_size != param.pool_size) return false;

    return true;
}
