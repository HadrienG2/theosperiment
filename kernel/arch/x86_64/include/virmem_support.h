 /* Support classes for the virtual memory manager

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

#ifndef _VIRMEM_SUPPORT_H_
#define _VIRMEM_SUPPORT_H_

#include <address.h>
#include <hack_stdint.h>
#include <phymem_support.h>
#include <pid.h>
#include <synchronization.h>

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
    PID identifier;
    VirMemProcess* next_item;
    OwnerlessMutex mutex;
    uint16_t may_free_kpages; //Boolean, specifies if the process descriptor can free pages with the K flag
    VirMemProcess() : map_pointer(NULL),
                      pml4t_location(NULL),
                      identifier(PID_INVALID),
                      next_item(NULL),
                      may_free_kpages(0) {};
    //Comparing list items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const VirMemProcess& param) const;
    bool operator!=(const VirMemProcess& param) const {return !(*this==param);}
} __attribute__((packed));

#endif
