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

#ifndef _MEMORY_SUPPORT_H_
#define _MEMORY_SUPPORT_H_

#include <address.h>
#include <pid.h>

//Represents an item in a map of the physical memory, managed as a chained list at the moment.
//Size must be a divisor of 4096 (current size : 0x40)
//
//The provided algorithms use the following assumptions :
//  -Memory regions are sorted in location order
//  -No overlap exists between memory regions
//  -Memory regions are at least one page large and locations are page-aligned
struct PhyMemMap {
  addr_t location;
  addr_t size;
  uint32_t allocatable; //Boolean. Indicates that this memory chunk can be reserved by memory allocation
                        //algorithms. It should NOT be the case with memory-mapped I/O, as an example.
  PIDs owners;
  PhyMemMap* next_buddy; //This second chaining allows non-contiguous memory chunk
  PhyMemMap* next_mapitem;
  uint64_t padding;
  uint64_t padding2;
  PhyMemMap() : location(0),
                 size(0),
                 allocatable(true),
                 owners(PID_NOBODY),
                 next_buddy(NULL),
                 next_mapitem(NULL) {};
  //This mirrors the member functions of "owners"
  int add_owner(PID new_owner) {return owners.add_pid(new_owner);}
  void clear_owners() {owners.clear_pids();}
  void del_owner(PID old_owner) {owners.del_pid(old_owner);}
  bool has_owner(PID the_owner) {return owners.has_pid(the_owner);}
  //Algorithms finding things in the map
  PhyMemMap* find_freechunk();
  PhyMemMap* find_thischunk(addr_t location);
} __attribute__((packed));

//Permission flags for virtual memory management
typedef uint32_t perm_flags;
#define PERM_PRESENT 1
#define PERM_READ 2
#define PERM_WRITE 4
#define PERM_EXECUTE 8

//A variant of the class above which is more virtual memory oriented. Apart from that, its management
//is absolutely identical...
struct VirMemMap {
  addr_t vir_location;
  addr_t phys_location;
  addr_t size;
  perm_flags flags;
  PIDs owners;
  VirMemMap* next_buddy; //This second chaining allows non-contiguous memory chunk
  VirMemMap* next_mapitem;
  uint64_t padding;
  VirMemMap() : vir_location(0),
             phys_location(0),
             size(0),
             flags(7),
             owners(PID_NOBODY),
             next_buddy(NULL),
             next_mapitem(NULL) {};
  //This mirrors the member functions of "owners"
  int add_owner(PID new_owner) {return owners.add_pid(new_owner);}
  void clear_owners() {owners.clear_pids();}
  void del_owner(PID old_owner) {owners.del_pid(old_owner);}
  bool has_owner(PID the_owner) {return owners.has_pid(the_owner);}
  //Algorithms finding things in the map
  VirMemMap* find_freechunk();
  VirMemMap* find_thischunk(addr_t location);
} __attribute__((packed));

#endif