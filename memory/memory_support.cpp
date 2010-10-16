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

bool compare_virphy(const VirMemMap& vir, const PhyMemMap& phy) {
  if(vir.location != phy.location) return false;
  if(vir.size != phy.size) return false;
  if(vir.owners != phy.owners) return false;
  if(vir.points_to != &phy) return false;
  
  return true;
}

PhyMemMap* PhyMemMap::find_thischunk(const addr_t location) const {
  PhyMemMap* current_item = (PhyMemMap*) this;
  if(current_item->location > location) return NULL;
  
  while(current_item) {
    if(current_item->location+current_item->size > location) break;
    current_item = current_item->next_mapitem;
  }
  return current_item;
}

VirMemMap* VirMemMap::find_thischunk(const addr_t location) const {
  VirMemMap* current_item = (VirMemMap*) this;
  if(current_item->location > location) return NULL;
  
  while(current_item) {
    if(current_item->location+current_item->size > location) break;
    current_item = current_item->next_mapitem;
  }
  return current_item;
}
