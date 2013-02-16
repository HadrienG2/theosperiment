 /* Support classes for the paging manager

      Copyright (C) 2010-2013  Hadrien Grasland

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

#include <paging_support.h>

PageChunk* PageChunk::find_thischunk(const size_t location) const {
    PageChunk* current_item = (PageChunk*) this;
    if(current_item->location > location) return NULL;

    while(current_item) {
        if(current_item->location == location) break;
        current_item = current_item->next_mapitem;
    }

    return current_item;
}

size_t PageChunk::length() const {
    size_t result = 0;
    PageChunk* current_item = (PageChunk*) this;

    while(current_item) {
        ++result;
        current_item = current_item->next_mapitem;
    }

    return result;
}

bool PageChunk::operator==(const PageChunk& param) const {
    if(location != param.location) return false;
    if(size != param.size) return false;
    if(flags != param.flags) return false;
    if(points_to != param.points_to) return false;
    if(next_buddy != param.next_buddy) return false;
    if(next_mapitem != param.next_mapitem) return false;

    return true;
}

bool PagingManagerProcess::operator==(const PagingManagerProcess& param) const {
    if(identifier != param.identifier) return false;
    if(map_pointer != param.map_pointer) return false;
    if(pml4t_location != param.pml4t_location) return false;
    if(next_item != param.next_item) return false;
    if(mutex != param.mutex) return false;
    if(may_free_kpages != param.may_free_kpages) return false;

    return true;
}
