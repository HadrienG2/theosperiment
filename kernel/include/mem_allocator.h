 /* A RAMManager- and PagingManager-based memory allocator

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

#ifndef _KMEM_ALLOCATOR_H_
#define _KMEM_ALLOCATOR_H_

#include <address.h>
#include <mallocator_support.h>
#include <ram_manager.h>
#include <process_manager.h>
#include <pid.h>
#include <paging_manager.h>

const int MEMALLOCATOR_VERSION = 2; //Increase this when deep changes require a modification of
                                     //the testing protocol

class ProcessManager;
class MemAllocator {
    private:
        //Link to other kernel functionality
        RAMManager* ram_manager;
        ProcessManager* process_manager;
        PagingManager* paging_manager;

        //Internal MemAllocator state
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
                         const PageFlags flags,
                         const bool force);
        size_t allocator_shareable(MallocProcess* target,
                                   size_t size,
                                   const PageFlags flags,
                                   const bool force);
        bool liberator(MallocProcess* target, const size_t location);
        size_t share(MallocProcess* source,
                     const size_t location,
                     MallocProcess* target,
                     const PageFlags flags,
                     const bool force);
        MemoryChunk* shared_already(RAMChunk* to_share, MallocProcess* target_owner);

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
        MemAllocator(RAMManager& ram_manager, PagingManager& paging_manager);

        //Late feature initialization
        bool init_process(ProcessManager& process_manager); //Run once process management is available

        //Process management functions
        //PID add_process(PID id, ProcessProperties properties); //Adds a new process to MemAllocator's database
        void remove_process(PID target); //Removes all traces of a PID in MemAllocator
        //PID update_process(PID old_process, PID new_process); //Swaps noncritical parts of two PIDs for live updating purposes

        //Allocate memory to a process, returns location
        size_t malloc(PID target,
                      const size_t size,
                      const PageFlags flags = PAGE_FLAGS_RW,
                      const bool force = false);

        //Same as above, but the storage space is alone in its chunk, which allows sharing the data
        //inside with other processes without giving them access to other data
        size_t malloc_shareable(PID target,
                                const size_t size,
                                const PageFlags flags = PAGE_FLAGS_RW,
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
                     const PageFlags flags = PAGE_FLAGS_SAME,
                     const bool force = false);

        //Pooled memory allocation. The idea is to allocate a large enough block of memory with
        //init_pool(), then let malloc automatically allocate from that pool with very fast
        //performance. Once you're done, call leave_pool() to go back to normal memory allocation.
        //If, for some reason, you want to temporarily leave the pool and go back to it later, store
        //the value returned by leave_pool(), which is the previous pool state, and call
        //reinstate_pool() using it as a parameter later.
        //
        //Some things to keep in mind :
        // * You can only free the whole pool, not individual objects (that's the idea), so
        //   keep a pointer to said pool.
        // * You should make sure that pooled allocation is atomic as far as memory management is
        //   concerned, to prevent unrelated allocation requests from going to the pool and possibly
        //   causing pool overflow.
        // * Don't forget to call leave_pool() when you're done.
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
             const PageFlags flags = PAGE_FLAGS_RW,
             const bool force = false);
void* kalloc_shareable(PID target,
                       size_t size,
                       const PageFlags flags = PAGE_FLAGS_RW,
                       const bool force = false);
bool kfree(PID target, void* location);
void* kshare(const PID source,
             const void* location,
             PID target,
             const PageFlags flags = PAGE_FLAGS_SAME,
             const bool force = false);

//Pooled allocation
bool kenter_pool(PID target, void* pool);
void* kleave_pool(PID target);

//Global shortcuts to MemAllocator's process management functions
//PID mem_allocator_add_process(PID id, ProcessProperties properties);
void mem_allocator_remove_process(PID target);
//PID mem_allocator_update_process(PID old_process, PID new_process);

#endif
