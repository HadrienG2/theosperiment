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
 
#include <virtmem.h>

VirMemManager::VirMemManager(PhyMemManager& physmem, KernelInformation& kinfo) {
  //This function...
  //  1/Determines the amount of memory necessary to store the management structures
  //  2/Allocates this amount of memory using the physical memory manager
  //  3/Fill those stuctures, including with themselves, using the following rules in priority order
  //       -Pages of nature Bootstrap and Kernel belong to the kernel
  //       -Pages of nature Free and Reserved belong to nobody (PID_NOBODY)
  //       -Free and reserved memory has permission RW
  //       -Bootstrap memory has permission R
  //       -Kernel memory has permission RW unless declared otherwise
  //
  //A small issue comes from the fact that kinfo is not valid anymore at this stage.
  //That's because the physical memory manager may already be eating up some of that memory which
  //was still marked as free by the time this information was generated.
  //A function of PhyMemManager allows one to address this problem by giving one access to the internal
  //memory map being used. We'll parse its data at the same time as we parse kinfo. Any undocumented
  //memory region will in fact be kernel memory (in use by the physical memory manager).
  
  phymem = &physmem;
  
}