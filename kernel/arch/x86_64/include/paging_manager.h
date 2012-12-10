 /* Paging management classes

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

#ifndef _PAGING_MANAGER_H_
#define _PAGING_MANAGER_H_

#include <address.h>
#include <ram_manager.h>
#include <process_support.h>
#include <pid.h>
#include <synchronization.h>
#include <paging_support.h>


const int PAGINGMANAGER_VERSION = 3; //Increase this when deep changes require a modification of
                                      //the testing protocol

class ProcessManager;
class PagingManager {
    private:
        //Link to other kernel functionality
        RAMManager* ram_manager;
        ProcessManager* process_manager;

        //PagingManager's internal state
        OwnerlessMutex proclist_mutex; //Hold that mutex when parsing the process list or altering it
        PagingManagerProcess* process_list;
        PageChunk* free_mapitems; //A collection of ready to use paging memory map items
                                  //(chained using next_buddy)
        PagingManagerProcess* free_process_descs; //A collection of ready to use process descriptors
        bool malloc_active; //Tells if kernel-wide allocation through kalloc is available

        //Support methods
        bool alloc_mapitems(); //Get some memory map storage space
        bool alloc_process_descs(); //Get some map list storage space
        PageChunk* alloc_virtual_address_space(PagingManagerProcess* target,
                                               size_t size,
                                               size_t location = NULL);
        PageChunk* chunk_mapper(PagingManagerProcess* target,
                                const RAMChunk* ram_chunk,
                                const PageFlags flags,
                                size_t location = NULL);
        PageChunk* chunk_mapper_contig(PagingManagerProcess* target,
                                       const RAMChunk* ram_chunk,
                                       const PageFlags flags);
        PageChunk* chunk_mapper_identity(PagingManagerProcess* target,
                                         const RAMChunk* ram_chunk,
                                         const PageFlags flags,
                                         size_t offset);
        bool chunk_liberator(PagingManagerProcess* target,
                             PageChunk* chunk);
        PagingManagerProcess* find_pid(const PID target); //Find the map list entry associated to this PID,
                                                //return NULL if it does not exist.
        PagingManagerProcess* find_or_create_pid(PID target); //Same as above, but try to create the entry
                                                       //if it does not exist yet
        PageChunk* flag_adjust(PagingManagerProcess* target, //Adjust the paging flags associated with a chunk
                                 PageChunk* chunk,
                                 const PageFlags flags,
                                 const PageFlags mask);
        bool map_k_chunks(PagingManagerProcess* target); //Maps K chunks in a newly created address space
        bool map_kernel(); //Maps the kernel's initial address space during initialization
        bool remove_all_paging(PagingManagerProcess* target);
        bool remove_pid(PID target); //Discards management structures for this PID
        PagingManagerProcess* setup_pid(PID target); //Create management structures for a new PID
        void unmap_k_chunk(PageChunk* chunk); //Removes K pages from the address space of non-kernel processes.
        uint64_t x86flags(PageFlags flags); //Converts PageFlags to x86 paging flags
    public:
        PagingManager(RAMManager& ram_manager);

        //Late feature initialization
        bool init_malloc(); //Run once memory allocation is available
        bool init_process(ProcessManager& process_manager); //Run once process management is available

        //Process management functions
        //TODO : PID add_process(PID id, ProcessProperties properties); //Adds a new process to PagingManager's database
        void remove_process(PID target); //Removes all traces of a PID in PagingManager
        //TODO : PID update_process(PID old_process, PID new_process); //Swaps two PIDs for live updating purposes

        //Chunk manipulation functions
        PageChunk* map_chunk(const PID target,               //Map a non-contiguous RAM chunk as a
                             const RAMChunk* ram_chunk,  //contiguous page chunk in the target address space
                             const PageFlags flags = PAGE_FLAGS_RW);
        bool free_chunk(const PID target, //Unmaps a page chunk
                        size_t chunk_beginning);
        PageChunk* adjust_chunk_flags(const PID target, //Change a page chunk's properties
                                        size_t chunk_beginning,
                                        const PageFlags flags,
                                        const PageFlags mask);

        //x86_64 specific.
        //Prepare for a context switch by giving the CR3 value to load before jumping
        uint64_t cr3_value(const PID target);

        //Debug methods. Will go out in final release.
        void print_maplist();
        void print_mmap(PID owner);
        void print_pml4t(PID owner);
};

//Global shortcuts to PagingManager's process management functions
//TODO : PID paging_manager_add_process(PID id, ProcessProperties properties);
void paging_manager_remove_process(PID target);
//TODO : PID paging_manager_update_process(PID old_process, PID new_process);

#endif
