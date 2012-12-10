/* Support classes for the memory allocator

        Copyright (C) 2010-2012    Hadrien Grasland

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

#ifndef _MALLOCATOR_SUPPORT_H_
#define _MALLOCATOR_SUPPORT_H_

#include <address.h>
#include <hack_stdint.h>
#include <pid.h>
#include <synchronization.h>
#include <virmem_support.h>


//VirMemManager and PhyMemManager work on a per-page basis. Memory allocation works on a per-byte
//basis. MemAllocator was made to address this fundamental incompatibility without allocating a
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
    PID identifier;
    MemoryChunk* free_map; //A sorted map of available chunks of memory
    MemoryChunk* busy_map; //A sorted map of used chunks of memory
    MallocProcess* next_item;
    OwnerlessMutex mutex;
    uint16_t pool_stack; //Stores the amount of times enter_pool has been consecutively called
    size_t pool_location;
    size_t pool_size;
    uint64_t padding3;
    uint64_t padding4;
    MallocProcess() : identifier(PID_INVALID),
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