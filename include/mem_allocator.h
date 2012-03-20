 /* A PhyMemManager- and VirMemManager-based memory allocator

      Copyright (C) 2010-2011  Hadrien Grasland

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
#include <memory_support.h>
#include <phymem_manager.h>
#include <pid.h>
#include <virmem_manager.h>

const int MEMALLOCATOR_VERSION = 1; //Increase this when deep changes require a modification of
                                    //the testing protocol

//The goal of this class is simple : to implement an architecture-independent malloc/free-style
//functionality on top of the arch-specific [Phy|Vir]MemManager
//To do later : -Switching address spaces
class MemAllocator {
    private:
        PhyMemManager* phymem_manager;
        VirMemManager* virmem_manager;
        MallocProcess* process_list;
        MemoryChunk* free_mapitems; //A collection of ready to use memory map items
        MallocProcess* free_process_descs; //A collection of ready to use process descriptors
        OwnerlessMutex proclist_mutex; //Hold that mutex when parsing or modifying the process list

        //Internal allocator
        bool alloc_mapitems(); //Get some memory map storage space
        bool alloc_process_descs(); //Get some process descriptors

        //Support functions

        //Allocation, liberation and sharing functions -- normal processes
        size_t allocator(MallocProcess* target,
                         const size_t size,
                         const VirMemFlags flags,
                         const bool force);
        size_t allocator_shareable(MallocProcess* target,
                                   size_t size,
                                   const VirMemFlags flags,
                                   const bool force);
        bool liberator(MallocProcess* target, const size_t location);
        size_t share(MallocProcess* source,
                     const size_t location,
                     MallocProcess* target,
                     const VirMemFlags flags,
                     const bool force);
        MemoryChunk* shared_already(PhyMemChunk* to_share, MallocProcess* target_owner);

        //PID setup
        MallocProcess* find_pid(const PID target); //Find the map list entry associated to this PID,
                                                   //return NULL if it does not exist.
        MallocProcess* find_or_create_pid(PID target,  //Same as above, but try to create the entry
                                          bool force); //if it does not exist yet
        MallocProcess* setup_pid(PID target); //Create management structures for a new PID
        bool remove_pid(PID target); //Discards management structures for this PID

        //Pooled memory allocation
        bool set_pool(MallocProcess* target, size_t pool_location);

        //Auxiliary functions
        void liberate_memory();
    public:
        MemAllocator(PhyMemManager& physmem, VirMemManager& virtmem);

        //Kill a process, more exactly remove all traces of it from MemAllocator
        void remove_process(PID target);

        //Allocate memory to a process, returns location
        size_t malloc(PID target,
                      const size_t size,
                      const VirMemFlags flags = VIRMEM_FLAGS_RW,
                      const bool force = false);

        //Same as above, but the storage space is alone in its chunk, which allows sharing the data
        //inside with other processes without giving them access to other data
        size_t malloc_shareable(PID target,
                                const size_t size,
                                const VirMemFlags flags = VIRMEM_FLAGS_RW,
                                const bool force = false);

        //Free previously allocated memory. Returns false if location or process does not exist,
        //true otherwise
        bool free(PID target, const size_t location);

        //Give another process access to that data under the limits of "flags".
        //Note that by doing so, the current owner loses property of that data : free will only
        //remove his right to access the data, and not the data itself.
        //Also, always allocate data used for this with malloc_shareable.
        size_t share(PID source,
                     const size_t location,
                     PID target,
                     const VirMemFlags flags = VIRMEM_FLAGS_SAME,
                     const bool force = false);

        //Pooled memory allocation. The idea is to allocate a large enough block of memory with
        //init_pool(), then let malloc automatically allocate from that pool with very fast
        //performance. Once you're done, call leave_pool() to go back to normal memory allocation.
        //If, for some reason, you want to temporarily leave the pool and go back to it later, store
        //the value returned by leave_pool(), which is the previous pool state, and call
        //reinstate_pool() using it as a parameter later.
        //
        //Some things to keep in mind :
        // * You can only free the whole pool, not individual objects (that's the principle), so
        //   keep a pointer to it.
        // * You should make sure that pooled allocation is atomic as far as memory management is
        //   concerned, to prevent unrelated allocation requests from going to the pool and possibly
        //   causing pool overflow.
        // * To ensure better performance, pooled allocation comes with zero safety checks. This
        //   means in particular no protection against pool overflow, so don't forget to call
        //   leave_pool() when you're done.
        bool enter_pool(PID target, size_t pool_location);
        size_t leave_pool(PID target);

        //Debug methods. Will go out in final release.
        void print_maplist();
        void print_busymap(const PID owner);
        void print_freemap(const PID owner);
};

//Alocation, freeing, sharing
void* kalloc(PID target,
             const size_t size,
             const VirMemFlags flags = VIRMEM_FLAGS_RW,
             const bool force = false);
void* kalloc_shareable(PID target,
                       size_t size,
                       const VirMemFlags flags = VIRMEM_FLAGS_RW,
                       const bool force = false);
bool kfree(PID target, void* location);
void* kshare(const PID source,
             const void* location,
             PID target,
             const VirMemFlags flags = VIRMEM_FLAGS_SAME,
             const bool force = false);

//Pooled allocation
bool kenter_pool(PID target, void* pool);
void* kleave_pool(PID target);

#endif
