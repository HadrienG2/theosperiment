 /* A PhyMemManager- and VirMemManager-based memory allocator

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

#ifndef _KMEM_ALLOCATOR_H_
#define _KMEM_ALLOCATOR_H_

#include <address.h>
#include <physmem.h>
#include <pid.h>
#include <virtmem.h>

//The goal of this class is simple : to implement an architecture-independent malloc/free-style
//functionality on top of the arch-specific [Phy|Vir]MemManager
class MemAllocator {
    private:
        PhyMemManager* phymem;
        VirMemManager* virmem;
        MallocPIDList* map_list;
        MallocMap* free_mapitems; //A collection of ready to use memory map items
        MallocPIDList* free_listitems; //A collection of ready to use map list items
        KernelMutex maplist_mutex; //Hold that mutex when parsing the map list
                                   //or adding/removing maps from it.
        //Support functions
        addr_t alloc_mapitems(); //Get some memory map storage space
        addr_t alloc_listitems(); //Get some map list storage space
        addr_t mallocator(addr_t size, MallocPIDList* target);
        addr_t mallocator_knl(addr_t size); //The kernel has paging disabled, so it has special
                                            //memory allocation routines.
        void liberator(addr_t location, MallocPIDList* target);
        VirMapList* find_pid(PID target); //Find the map list entry associated to this PID,
                                          //return NULL if it does not exist.
        VirMapList* find_or_create_pid(PID target); //Same as above, but try to create the entry
                                                    //if it does not exist yet
        VirMapList* setup_pid(PID target); //Create management structures for a new PID
        VirMapList* remove_pid(PID target); //Discards management structures for this PID
    public:
        MemAllocator(PhyMemManager& phymem, VirMemManager& virmem);
        
        //The functions you have always dreamed of
        addr_t malloc(addr_t size, PID process); //Allocate memory to a process, returns location
        addr_t free(addr_t location, PID process); //Free previously allocated memory. Returns 0
                                                   //if location or process does not exist, location
                                                   //otherwise.
        
        //Shortcuts for use inside of the kernel
        addr_t kalloc(addr_t size) {return malloc(size, PID_KERNEL);}
        addr_t kfree(addr_t location) {return free(location, PID_KERNEL);}
};

#endif