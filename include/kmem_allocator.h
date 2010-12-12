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
        KernelMutex maplist_mutex; //Hold that mutex when parsing or modifying the map list
        //Kernel malloc management data
        KnlMallocMap* knl_free_map; //A sorted map of ready-to-use chunks of memory for the kernel
        KnlMallocMap* knl_busy_map; //A sorted map of the chunks of memory used by the kernel
        KernelMutex knl_mutex; //Hold that mutex when parsing or modifying the kernel maps
        //Support functions
        addr_t alloc_mapitems(); //Get some memory map storage space
        addr_t alloc_listitems(); //Get some map list storage space
        addr_t allocator(addr_t size, MallocPIDList* target);
        addr_t liberator(addr_t location, MallocPIDList* target);
        addr_t knl_allocator(addr_t size);     //Variants of the previous functions specifically for
        addr_t knl_liberator(addr_t location); //the kernel
        MallocPIDList* find_pid(PID target); //Find the map list entry associated to this PID,
                                             //return NULL if it does not exist.
        MallocPIDList* find_or_create_pid(PID target); //Same as above, but try to create the entry
                                                       //if it does not exist yet
        MallocPIDList* setup_pid(PID target); //Create management structures for a new PID
        MallocPIDList* remove_pid(PID target); //Discards management structures for this PID
    public:
        MemAllocator(PhyMemManager& physmem, VirMemManager& virtmem) : phymem(&physmem),
                                                                       virmem(&virtmem),
                                                                       map_list(NULL),
                                                                       free_mapitems(NULL),
                                                                       free_listitems(NULL),
                                                                       knl_free_map(NULL),
                                                                       knl_busy_map(NULL) {}
        
        //The functions you have always dreamed of
        addr_t malloc(addr_t size, PID target); //Allocate memory to a process, returns location
        addr_t free(addr_t location, PID target); //Free previously allocated memory. Returns 0
                                                  //if location or process does not exist, location
                                                  //otherwise.
        
        //Shortcuts for use inside of the kernel
        addr_t kalloc(addr_t size) {return malloc(size, PID_KERNEL);}
        addr_t kfree(addr_t location) {return free(location, PID_KERNEL);}
};

#endif