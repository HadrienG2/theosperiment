 /* Support classes for the physical memory manager

      Copyright (C) 2010-2012  Hadrien Grasland

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

#ifndef _PHYMEM_SUPPORT_H_
#define _PHYMEM_SUPPORT_H_

#include <address.h>
#include <hack_stdint.h>
#include <pid.h>
#include <synchronization.h>

//A list of PIDs
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

//This structure is used to manage high-level functionality of PhyMemManager after ProcessManager
//has been initialized. Contrary to other memory management structures, it is allocated using kalloc()
//and as such does not need to follow alignment constraints
struct PhyMemProcess {
    OwnerlessMutex mutex;
    PID identifier;
    size_t memory_usage;
    size_t memory_cap;
    PhyMemProcess* next_item;

    PhyMemProcess() : identifier(PID_INVALID),
                      memory_usage(0),
                      memory_cap(0xffffffffffffffff),
                      next_item(NULL) {}
};

#endif
