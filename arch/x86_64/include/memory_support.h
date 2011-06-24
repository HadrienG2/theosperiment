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

typedef struct VirMapList VirMapList;

//**************************************************
//*********** Physical memory management ***********
//**************************************************

//Represents an item in a map of the physical memory, managed as a chained list at the moment.
//Size should be a divisor of 0x1000 (current size : 0x40) to ease the early allocation process.
struct PhyMemMap {
    addr_t location;
    addr_t size;
    PIDs owners;
    uint32_t allocatable; //Boolean. Indicates that this memory chunk can be reserved by memory
                          //allocation algorithms.
                          //It should NOT be the case with memory-mapped I/O, as an example.
    PhyMemMap* next_buddy;
    PhyMemMap* next_mapitem;
    uint64_t padding;
    uint64_t padding2;
    PhyMemMap() : location(0),
                  size(0),
                  owners(PID_NOBODY),
                  allocatable(true),
                  next_buddy(NULL),
                  next_mapitem(NULL) {};
    //This mirrors the member functions of "owners"
    unsigned int add_owner(const PID new_owner) {return owners.add_pid(new_owner);}
    void clear_owners() {owners.clear_pids();}
    void del_owner(const PID old_owner) {owners.del_pid(old_owner);}
    bool has_owner(const PID the_owner) const {return owners.has_pid(the_owner);}
    //Algorithms finding things in or about the map
    PhyMemMap* find_contigchunk(const addr_t size) const; //The returned chunk, along with its next
                                                          //neighbours, forms a contiguous chunk
                                                          //of free memory at least "size" large.
    PhyMemMap* find_thischunk(const addr_t location) const;
    unsigned int buddy_length() const;
    unsigned int length() const;
    //Comparing map items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const PhyMemMap& param) const;
    bool operator!=(const PhyMemMap& param) const {return !(*this==param);}
} __attribute__((packed));

//**************************************************
//*********** Virtual memory management ************
//**************************************************

//The following flags are available when handling virtual memory access permission
typedef uint32_t VirMemFlags;
const VirMemFlags VMEM_FLAG_R = 1; //Region of virtual memory is readable
const VirMemFlags VMEM_FLAG_W = (1<<1); //...writable
const VirMemFlags VMEM_FLAG_X = (1<<2); //...executable
const VirMemFlags VMEM_FLAG_A = (1<<3); //...absent (accessing it will result in a page fault)
const VirMemFlags VMEM_FLAG_K = (1<<4); //...Kernel (not invalidated during a context switch and not
                                        //accessible by user programs directly, used on kernel pages
                                        //which are common to all processes)
const VirMemFlags VMEM_FLAGS_SAME = (1<<31); //This special virtual memory flag overrides all others,
                                             //and is used for sharing. It means that the shared
                                             //memory region is set up using the same flags than its
                                             //"mother" region.
const VirMemFlags VMEM_FLAGS_RX = VMEM_FLAG_R + VMEM_FLAG_X;
const VirMemFlags VMEM_FLAGS_RW = VMEM_FLAG_R + VMEM_FLAG_W;

//Represents an item in a map of the virtual memory, managed as a chained list at the moment.
//Size should be a divisor of 0x1000 (current size : 0x40) to ease the early allocation process.
struct VirMemMap {
    addr_t location;
    addr_t size;
    VirMemFlags flags;
    VirMapList* owner;
    PhyMemMap* points_to; //Physical memory chunk this virtual memory chunk points to
    VirMemMap* next_buddy;
    VirMemMap* next_mapitem;
    uint32_t padding; 
    uint64_t padding2;
    VirMemMap() : location(0),
                  size(0),
                  flags(VMEM_FLAGS_RW),
                  owner(NULL),
                  points_to(NULL),
                  next_buddy(NULL),
                  next_mapitem(NULL) {};
    //Algorithms finding things in or about the map
    VirMemMap* find_thischunk(const addr_t location) const;
    unsigned int length() const;
    //Comparing map items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const VirMemMap& param) const;
    bool operator!=(const VirMemMap& param) const {return !(*this==param);}
} __attribute__((packed));

//There is one map of virtual memory per process. Since there probably won't ever be more than
//1000 processes running and virtual memory-related requests don't occur that often, a linked
//list sounds like the most sensible option because of its flexibility. We can still change it later.
//As usual, size should be a divisor of 0x1000 (current size : 0x20)
struct VirMapList {
    PID map_owner;
    VirMemMap* map_pointer;
    addr_t pml4t_location;
    VirMapList* next_item;
    OwnerlessMutex mutex;
    uint16_t padding2;
    VirMapList() : map_owner(PID_NOBODY),
                   map_pointer(NULL),
                   pml4t_location(NULL),
                   next_item(NULL) {};
    //Comparing list items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const VirMapList& param) const;
    bool operator!=(const VirMapList& param) const {return !(*this==param);}
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
struct MallocMap {
    addr_t location;
    addr_t size;
    VirMemMap* belongs_to; //The chunk of virtual memory it belongs to.
    MallocMap* next_item;
    uint32_t shareable; //Boolean. Indicates that the content of the page has been allocated in a
                        //specific way which makes it suitable for sharing between processes.
    uint32_t share_count; //For the shared copy of a shared chunk, indicates how many times the
                          //chunk has been shared with this process. The process will have to free
                          //its shared copy the same amount of time before it is actually freed.
    uint64_t padding;
    uint64_t padding_2;
    uint64_t padding_3;
    MallocMap() : location(NULL),
                  size(NULL),
                  belongs_to(NULL),
                  next_item(NULL),
                  shareable(0),
                  share_count(0) {};
    MallocMap* find_contigchunk(const addr_t size) const; //Try to find at least "size" contiguous
                                                          //bytes in this map
    MallocMap* find_contigchunk(const addr_t size, const VirMemFlags flags) const;
    MallocMap* find_thischunk(const addr_t location) const;
    //Comparing C-style structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const MallocMap& param) const;
    bool operator!=(const MallocMap& param) const {return !(*this==param);}
} __attribute__((packed));

//There is a derivative of the previous structure for the kernel, as it has virtual memory disabled.
struct KnlMallocMap {
    addr_t location;
    addr_t size;
    PhyMemMap* belongs_to; //The chunk of *physical* memory it belongs to.
    KnlMallocMap* next_item;
    uint32_t shareable; //Boolean. Indicates that the content of the page has been allocated in a
                        //specific way which makes it suitable for sharing between processes.
    uint32_t share_count; //For the shared copy of a shared chunk, indicates how many times the
                          //chunk has been shared with this process. The process will have to free
                          //its shared copy the same amount of time before it is actually freed.
    uint64_t padding;
    uint64_t padding_2;
    uint64_t padding_3;
    KnlMallocMap() : location(NULL),
                     size(NULL),
                     belongs_to(NULL),
                     next_item(NULL),
                     shareable(0),
                     share_count(0) {};
    KnlMallocMap* find_contigchunk(const addr_t size) const; //Try to find at least "size"
                                                             //contiguous bytes in this map
    KnlMallocMap* find_thischunk(const addr_t location) const;
    //Comparing C-style structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const KnlMallocMap& param) const;
    bool operator!=(const KnlMallocMap& param) const {return !(*this==param);}
} __attribute__((packed));

//There are two maps per process, and we must keep track of each process. The same assumptions as
//before apply. The size of this should also be a divisor of 0x1000 (currently : 0x20);
struct MallocPIDList {
    PID map_owner;
    MallocMap* free_map; //A sorted map of ready-to-use chunks of memory
    MallocMap* busy_map; //A sorted map of used chunks of memory
    MallocPIDList* next_item;
    OwnerlessMutex mutex;
    uint16_t padding;
    addr_t pool; //For pooled memory allocation purposes
    uint64_t padding2;
    uint64_t padding3;
    uint64_t padding4;
    MallocPIDList() : map_owner(PID_NOBODY),
                      free_map(NULL),
                      busy_map(NULL),
                      next_item(NULL),
                      pool(NULL) {};
    //Comparing C-style structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const MallocPIDList& param) const;
    bool operator!=(const MallocPIDList& param) const {return !(*this==param);}
} __attribute__((packed));

#endif