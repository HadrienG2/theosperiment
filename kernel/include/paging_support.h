 /* Support classes for the paging manager

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

#ifndef _PAGING_SUPPORT_H_
#define _PAGING_SUPPORT_H_

#include <address.h>
#include <hack_stdint.h>
#include <ram_support.h>
#include <pid.h>
#include <synchronization.h>

//The following flags are available when handling page access permission
typedef uint32_t PageFlags;
const PageFlags PAGE_FLAG_R = 1; //Region of memory is readable
const PageFlags PAGE_FLAG_W = (1<<1); //...writable
const PageFlags PAGE_FLAG_X = (1<<2); //...executable
const PageFlags PAGE_FLAG_A = (1<<3); //...absent (accessing it will result in a page fault)
const PageFlags PAGE_FLAG_K = (1<<4); //...Global kernel memory (present in all address spaces and not
                        //accessible by user programs directly, used on kernel pages which are common
                        //to all processes. K pages should not be created after the first non-kernel
                        //process has been created and run, for their presence in that process' address
                        //space cannot be guaranteed)
const PageFlags PAGE_FLAGS_SAME = (1<<31); //This special paging flag overrides all others,
                        //and is used for sharing. It means that the shared memory region is set up
                        //using the same flags than its "mother" region.
const PageFlags PAGE_FLAGS_RX = PAGE_FLAG_R + PAGE_FLAG_X;
const PageFlags PAGE_FLAGS_RW = PAGE_FLAG_R + PAGE_FLAG_W;


//Represents an item in a map of page translations, managed as a linked list at the moment.
struct PageChunk {
    size_t location;
    size_t size;
    PageFlags flags;
    RAMChunk* points_to; //RAM chunk this page chunk points to
    PageChunk* next_buddy;

    //WARNING : PageChunk properties after this point are nonstandard, subject to change without
    //warnings, and should not be read or manipulated by external software.
    PageChunk* next_mapitem;
    PageChunk() : location(0),
                  size(0),
                  flags(PAGE_FLAGS_RW),
                  points_to(NULL),
                  next_buddy(NULL),
                  next_mapitem(NULL) {};
    //Algorithms finding things in or about the map
    PageChunk* find_thischunk(const size_t location) const;
    size_t length() const;
    //Comparing map items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const PageChunk& param) const;
    bool operator!=(const PageChunk& param) const {return !(*this==param);}
};


//There is one map of page translations per process. Since there probably won't ever be more than
//1000 processes running and paging-related requests don't occur that often, a linked
//list sounds like the most sensible option because of its flexibility.
struct PagingManagerProcess {
    PageChunk* map_pointer;
    size_t pml4t_location;

    //WARNING : PagingManagerProcess properties after this point are nonstandard, subject to change without
    //warnings, and should not be read or manipulated by external software.
    PID identifier;
    PagingManagerProcess* next_item;
    OwnerlessMutex mutex;
    bool may_free_kpages; //Specifies if the process descriptor can free pages with the K flag
    PagingManagerProcess() : map_pointer(NULL),
                             pml4t_location(NULL),
                      	     identifier(PID_INVALID),
                             next_item(NULL),
                             may_free_kpages(false) {};
    //Comparing list items is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const PagingManagerProcess& param) const;
    bool operator!=(const PagingManagerProcess& param) const {return !(*this==param);}
};

#endif
