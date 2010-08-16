 /* Support classes for all code that manages chunks (contiguous or non-contiguous) of memory

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
 
#include <memory_support.h>

MemMap* MemMap::find_freechunk() {
  MemMap* current_item = this;

  while(current_item) {
    if(current_item->has_owner(PID_NOBODY) && current_item->allocatable) break;
    current_item = current_item->next_item;
  }
  return current_item;
}

MemMap* MemMap::find_thischunk(addr_t location) {
  MemMap* current_item = this;
  if(current_item->location > location) return NULL;
  
  while(current_item) {
    if(current_item->location+current_item->size > location) break;
    current_item = current_item->next_item;
  }
  return current_item;
}
