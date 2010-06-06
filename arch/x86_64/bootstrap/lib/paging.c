/*  Everything needed to setup 4KB-PAE-32b paging with identity mapping

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

#include <paging.h>
#include <kernel_loader.h>
#include <bs_string.h>

pml4e* generate_pagetable(kernel_information* kinfo) {
  /* Page table will be located after the kernel. First, we're going to find the end of the kernel in the memory map. It should
     be called "Kernel RW- segment" */
  
  unsigned int i;
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  
  for(i=0; i<kinfo->kmmap_size; ++i) {
    if(strcmp((char*) (uint32_t) kmmap[i].name, "Kernel RW- segment")==0) break;
  }
  

  return (pml4e*) kmmap;
}
