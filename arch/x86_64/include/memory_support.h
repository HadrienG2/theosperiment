/* Support classes for all code that manages chunks (contiguous or non-contiguous) of memory

        Copyright (C) 2010-2011    Hadrien Grasland

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

#ifndef _MEMORY_SUPPORT_H_
#define _MEMORY_SUPPORT_H_

#include <address.h>
#include <pid.h>
#include <synchronization.h>

typedef struct VirMemProcess VirMemProcess;

//**************************************************
//*********** Physical memory management ***********
//**************************************************

//Represent a list of PIDs
struct PIDs {
    PID current_pid;
    PIDs* next_item;
    uint32_t padding;

    //Constructors and destructors
    PIDs() : current_pid(PID_INVALID),
                         next_item(NULL) {}
    PIDs(const PID& first) : current_pid(first),
                             next_item(NULL) {}

    //Various utility function
    bool has_pid(const PID& the_pid) const;
    unsigned int length() const;
    PIDs& operator=(const PID& param); //Set the contents of a blank PIDs to a single PID

    //Comparison of PIDs
    bool operator==(const PIDs& param) const;
    bool operator!=(const PIDs& param) const {return !(*this==param);}
} __attribute__((packed));


//Represents an item in a map of the physical memory, managed as a chained list at the moment.
//Size should be a divisor of 0x1000 (current size : 0x40) to ease the early allocation process.
struct PhyMemChunk {
    size_t location;
    size_t size;
    PIDs owners;
    uint32_t allocatable; //Boolean. Whether this chunk can be reserved by memory allocation.
                          //It should NOT be the case with memory-mapped I/O, as an example.
    PhyMemChunk* next_buddy;

    //WARNING : PhyMemChunk properties after this point are nonstandard, subject to change without
    //warnings, and should not be read or manipulated by external software.
    PhyMemChunk* next_mapitem;
    uint32_t padding;
    uint64_t padding2;

    PhyMemChunk() : location(0),
                    size(0),
                    owners(PID_INVALID),
                    allocatable(true),
                    next_buddy(NULL),
                    next_mapitem(NULL) {};
    //This mirrors the member functions of "owners"
    bool has_owner(const PID the_owner) const {return owners.has_pid(the_owner);}
    //Algorithms finding things in or about the map
    PhyMemChunk* find_contigchunk(const size_t size) const; //The returned chunk, along with its next
                                                          //neighbours, forms a contiguous chunk
                                                          //of free memory at least "size" large.
    PhyMemChunk* find_thischunk(const size_t location) const;
    unsigned int buddy_length() const;
    unsigned int length() const;
    //Comparing map items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const PhyMemChunk& param) const;
    bool operator!=(const PhyMemChunk& param) const {return !(*this==param);}
} __attribute__((packed));


//**************************************************
//*********** Virtual memory management ************
//**************************************************

//The following flags are available when handling virtual memory access permission
typedef uint32_t VirMemFlags;
const VirMemFlags VIRMEM_FLAG_R = 1; //Region of virtual memory is readable
const VirMemFlags VIRMEM_FLAG_W = (1<<1); //...writable
const VirMemFlags VIRMEM_FLAG_X = (1<<2); //...executable
const VirMemFlags VIRMEM_FLAG_A = (1<<3); //...absent (accessing it will result in a page fault)
const VirMemFlags VIRMEM_FLAG_K = (1<<4); //...Global kernel memory (present in all address spaces and not
                        //accessible by user programs directly, used on kernel pages which are common
                        //to all processes. K pages should not be created after the first non-kernel
                        //process has been created and run, for their presence in that process' address
                        //space cannot be guaranteed)
const VirMemFlags VIRMEM_FLAGS_SAME = (1<<31); //This special virtual memory flag overrides all others,
                        //and is used for sharing. It means that the shared memory region is set up
                        //using the same flags than its "mother" region.
const VirMemFlags VIRMEM_FLAGS_RX = VIRMEM_FLAG_R + VIRMEM_FLAG_X;
const VirMemFlags VIRMEM_FLAGS_RW = VIRMEM_FLAG_R + VIRMEM_FLAG_W;


//Represents an item in a map of the virtual memory, managed as a chained list at the moment.
//Size should be a divisor of 0x1000 (current size : 0x40) to ease the early allocation process.
struct VirMemChunk {
    size_t location;
    size_t size;
    VirMemFlags flags;
    PhyMemChunk* points_to; //Physical memory chunk this virtual memory chunk points to
    VirMemChunk* next_buddy;

    //WARNING : VirMemChunk properties after this point are nonstandard, subject to change without
    //warnings, and should not be read or manipulated by external software.
    VirMemChunk* next_mapitem;
    uint32_t padding;
    uint64_t padding2;
    uint64_t padding3;
    VirMemChunk() : location(0),
                  size(0),
                  flags(VIRMEM_FLAGS_RW),
                  points_to(NULL),
                  next_buddy(NULL),
                  next_mapitem(NULL) {};
    //Algorithms finding things in or about the map
    VirMemChunk* find_thischunk(const size_t location) const;
    unsigned int length() const;
    //Comparing map items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const VirMemChunk& param) const;
    bool operator!=(const VirMemChunk& param) const {return !(*this==param);}
} __attribute__((packed));


//There is one map of virtual memory per process. Since there probably won't ever be more than
//1000 processes running and virtual memory-related requests don't occur that often, a linked
//list sounds like the most sensible option because of its flexibility. We can still change it later.
//As usual, size should be a divisor of 0x1000 (current size : 0x20)
struct VirMemProcess {
    VirMemChunk* map_pointer;
    size_t pml4t_location;

    //WARNING : VirMemProcess properties after this point are nonstandard, subject to change without
    //warnings, and should not be read or manipulated by external software.
    PID owner;
    VirMemProcess* next_item;
    OwnerlessMutex mutex;
    uint16_t may_free_kpages; //Boolean, specifies if the process descriptor can free pages with the K flag
    VirMemProcess() : map_pointer(NULL),
                      pml4t_location(NULL),
                      owner(PID_INVALID),
                      next_item(NULL),
                      may_free_kpages(0) {};
    //Comparing list items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const VirMemProcess& param) const;
    bool operator!=(const VirMemProcess& param) const {return !(*this==param);}
} __attribute__((packed));


//**************************************************
//*************** Memory allocation ****************
//**************************************************

//VirMemManager and PhyMemManager work on a per-page basis. Memory allocation works on a per-byte
//basis. KnlMemAllocator was made to address this fundamental incompatibility without allocating a
//full page when applications only ask for a few bytes. To do that, it maintains two sorted linked
//lists per process : a list of allocated, but not used yet, "parts of page", and a list of
//allocated and used parts of pages. Each list uses the following structure.
//Its size should again be a divisor of 0x1000 (currently : 0x20)
struct MemoryChunk {
    size_t location;
    size_t size;
    VirMemChunk* belongs_to; //The chunk of virtual memory it belongs to.
    MemoryChunk* next_item;
    uint32_t shareable; //Boolean. Indicates that the content of the page has been allocated in a
                        //specific way which makes it suitable for sharing between processes.
    uint32_t share_count; //For the shared copy of a shared chunk, indicates how many times the
                          //chunk has been shared with this process. The process will have to free
                          //its shared copy the same amount of time before it is actually freed.
    uint64_t padding;
    uint64_t padding_2;
    uint64_t padding_3;
    MemoryChunk() : location(NULL),
                  size(NULL),
                  belongs_to(NULL),
                  next_item(NULL),
                  shareable(0),
                  share_count(0) {};
    MemoryChunk* find_contigchunk(const size_t size) const; //Try to find at least "size" contiguous
                                                          //bytes in this map
    MemoryChunk* find_contigchunk(const size_t size, const VirMemFlags flags) const;
    MemoryChunk* find_thischunk(const size_t location) const;
    //Comparing C-style structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const MemoryChunk& param) const;
    bool operator!=(const MemoryChunk& param) const {return !(*this==param);}
} __attribute__((packed));


//There are two maps per process, and we must keep track of each process. The same assumptions as
//before apply. The size of this should also be a divisor of 0x1000 (currently : 0x20);
struct MallocProcess {
    PID owner;
    MemoryChunk* free_map; //A sorted map of ready-to-use chunks of memory
    MemoryChunk* busy_map; //A sorted map of used chunks of memory
    MallocProcess* next_item;
    OwnerlessMutex mutex;
    uint16_t pool_stack; //Stores the amount of times enter_pool has been consecutively called
    size_t pool_location;
    size_t pool_size;
    uint64_t padding3;
    uint64_t padding4;
    MallocProcess() : owner(PID_INVALID),
                      free_map(NULL),
                      busy_map(NULL),
                      next_item(NULL),
                      pool_stack(0),
                      pool_location(NULL),
                      pool_size(NULL) {};
    //Comparing C-style structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const MallocProcess& param) const;
    bool operator!=(const MallocProcess& param) const {return !(*this==param);}
} __attribute__((packed));

#endif
