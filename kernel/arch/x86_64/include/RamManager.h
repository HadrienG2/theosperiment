 /* RAM manager, that manages allocation of whole pages of ram

    Copyright (C) 2010-2013   Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#ifndef _RAM_MANAGER_H_
#define _RAM_MANAGER_H_

#include <address.h>
#include <align.h>
#include <KernelInformation.h>
#include <ram_support.h>
#include <pid.h>
#include <process_support.h>
#include <synchronization.h>

const int RAMMANAGER_VERSION = 3; //Increase this when changes require a modification of
                                   //the testing protocol

class ProcessManager;
class RamManager {
    private:
        //Link to other kernel functionality
        ProcessManager* process_manager;

        //Map of Ram (and its mutex)
        OwnerlessMutex mmap_mutex;
        RamChunk* ram_map; //A map of the whole memory
        RamChunk* highmem_map; //A map of high memory (addresses >0x100000)
        RamChunk* free_lowmem; //A noncontiguous chunk representing free low memory
        RamChunk* free_mem; //A noncontiguous chunk representing free high memory

        //Process management
        OwnerlessMutex proclist_mutex;
        RamManagerProcess* process_list;

        //Buffer of management structures
        RamChunk* free_mapitems; //A collection of spare RamChunk objects forming a dummy chunk,
                                 //ready for use in a memory map
        PIDs* free_pids; //A collection of spare PIDs objects forming a dummy PIDs, ready for use
                         //in a memory map
        RamManagerProcess* free_procitems; //A collection of space process descriptors forming a
                                           //dummy list, ready for use in the process list

        //Support methods used by public methods
        bool alloc_mapitems(RamChunk* free_mem_override = NULL);
        bool alloc_pids();
        bool alloc_procitems();
        RamChunk* chunk_allocator(RamManagerProcess* owner,
                                  const size_t size,
                                  RamChunk*& free_mem_used,
                                  bool contiguous);
        bool chunk_liberator(RamChunk* chunk);
        bool chunk_owneradd(RamManagerProcess* new_owner, RamChunk* chunk);
        bool chunk_ownerdel(RamManagerProcess* former_owner, RamChunk* chunk);
        void discard_empty_chunks();
        void fill_mmap(const KernelInformation& kinfo);
        RamManagerProcess* find_process(const PID target);
        void fix_overlap(RamChunk* first_chunk, RamChunk* second_chunk);
        RamChunk* generate_chunk(const KernelInformation& kinfo, size_t& index);
        bool initialize_process_list();
        void killer(RamManagerProcess* target);
        void merge_with_next(RamChunk* first_item); //Merge two consecutive elements of
                                                       //the memory map (in order to save space)
        void pids_liberator(PIDs& target);
        bool split_chunk(RamChunk* chunk, const size_t position);
    public:
        RamManager(const KernelInformation& kinfo);

        //Late feature initialization
        bool init_process(ProcessManager& process_manager); //TODO: Connect the RAM manager with process management facilities

        //Process management functions
        //TODO: PID add_process(PID id, ProcessProperties properties); //Adds a new process to RamManager's database
        void remove_process(PID target); //Removes all traces of a PID in RamManager
        //TODO: PID update_process(PID old_process, PID new_process); //Swaps two PIDs for live updating purposes

        //Page/chunk allocation, sharing and freeing functions
        RamChunk* alloc_chunk(const PID initial_owner,     //Allocates a chunk of memory which is
                              const size_t size = PG_SIZE, //at least "size" large. The
                               bool contiguous = false);    //"contiguous" flag forces it to be
                                                              //physically contiguous
        bool share_chunk(const PID new_owner,  //Add owners to a chunk
                         size_t chunk_beginning);
        bool free_chunk(const PID former_owner,  //Free a chunk from a PID's grasp
                        size_t chunk_beginning); //(liberate it if it no longer has any owner)

        //x86_64-specific methods
        RamChunk* alloc_lowchunk(const PID initial_owner, //Allocate a chunk of low memory
                                    const size_t size = PG_SIZE,
                                    bool contiguous = false);

        //WARNING : Functions after this point are nonstandard, subject to change without
        //warnings, and should not be read or manipulated by external software.

        //Dumps the memory map, used by VirMemManager() to derive the kernel's virtual address space.
        RamChunk* dump_mmap() {return ram_map;}

        //Debug methods
        void print_mmap(); //Print a map of memory
        void print_highmmap(); //Print a map of high memory (>=1MB)
        void print_lowmmap(); //Print a map of low memory (<1MB)
        void print_proclist(); //Print a list of processes along with their properties
};

extern RamManager* ram_manager;

//Global shortcuts to RamManager's process management functions (necessary because C++ does not allow direct linking to a class' method)
//TODO: PID ram_manager_add_process(PID id, ProcessProperties properties);
void ram_manager_remove_process(PID target);
//TODO: PID ram_manager_update_process(PID old_process, PID new_process);

#endif
