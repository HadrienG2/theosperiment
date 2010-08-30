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

#ifndef _VIRTMEM_H_
#define _VIRTMEM_H_

#include <physmem.h>

//This class takes care of
//  -Knowing the amount of allocatable virtual memory
//  -Mapping chunks of virtual memory which are in use by processes, and the place in
//    physical memory which they point to.
//  -...which PID (Process IDentifier) owns each used page of memory
//  -Mapping of a non-contiguous chunk of physical memory as a contiguous chunk of virtual memory,
//    adding up RWX-style permission management.
//
//Later: -Sharing of physical memory pages/chunks.
//       -Maybe swapping, in a very distant future.
class VirMemManager {
  private:
    PhyMemManager* phymem;
  public:
    VirMemManager(PhyMemManager& physmem, KernelInformation& kinfo);
    //Virtual page management function
    addr_t alloc_chunk(PID initial_owner, PhyMemMap* phys_chunk, perm_flags flags);
    addr_t alloc_page(PID initial_owner, addr_t phys_page, perm_flags flags);
    addr_t set_pageflags(addr_t virt_page, perm_flags flags);
    addr_t free(addr_t virt_location);
    
    //Debug methods (to be deleted)
    void print_lowmmap();
    void print_highmmap();
};

#endif