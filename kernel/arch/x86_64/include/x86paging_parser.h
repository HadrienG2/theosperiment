 /* Functions for parsing x86 page tables

      Copyright (C) 2011  Hadrien Grasland

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

#ifndef _X86_PAGING_PARSER_H_
#define _X86_PAGING_PARSER_H_

#include <stdint.h>

namespace x86paging {
    //This special integer type is used to describe at which level of the paging hierarchy we are.
    typedef int PagingLevel;
    extern const PagingLevel PML4T_LEVEL;
    extern const PagingLevel PDPT_LEVEL;
    extern const PagingLevel PD_LEVEL;
    extern const PagingLevel PT_LEVEL;
    extern const PagingLevel LVL_DECREMENT; //Substract this from the current level to go to the
                                            //next level

    //To parse a block of virtual memory in x86's multilevel paging structures requires the
    //paging_parser function and an item_handler function. It works as follows :
    //  -paging_parser parses the top-level page table (PML4T) and gives each relevant item of said
    //    table to the provided item_hander function.
    //  -item_handler applies the required modifications at this level of page table (if any), then
    //    calls paging_parser at the lower level.
    //  -Everything is repeated recursively until item_handler is called at the lowest level we
    //    want to reach.
    //  -If item_handler returns 0, paging_parser aborts its operation, since it means something has
    //    failed. If item_handler returns anything else, paging_parser returns the value returned by
    //    the last item_handler.
    //
    //To do anything serious, item handlers requires more parameters than just the page table item.
    //These additional parameters are provided in the form of an array of 64-bit unsigned integers,
    //additional_params, which is passed to the paging_parser function and just transmitted as is to
    //item handers.
    uint64_t paging_parser(uint64_t vaddr,
                           const uint64_t size,
                           const PagingLevel level,
                           uint64_t* page_table,
                           uint64_t (*item_handler)(uint64_t vaddr,
                                                    const uint64_t size,
                                                    const PagingLevel level,
                                                    uint64_t &table_item,
                                                    uint64_t* additional_params),
                           uint64_t* additional_params);

    //Sample item handler : parses the paging structures down to the lowest accessible level,
    //returns a given value if it's the page table level and 0 otherwise.
    //
    //additional_params contents :
    //  0 - The returned value in case of a success
    uint64_t dummy_handler(uint64_t vaddr,
                           const uint64_t size,
                           const PagingLevel level,
                           uint64_t &table_item,
                           uint64_t* additional_params);

    //fill_4kpaging item handler : Maps a block of physical addresses at a designated area of the
    //virtual address space of a process, using 4KB paging.
    //We assume that the required structures are already allocated at the moment, so this function
    //is in fact void, always returning 1.
    //
    //additional_params contents :
    //  0 - Beginning of the block of physical addresses to be mapped
    //  1 - Flags to be used at the page table level when mapping it
    //  2 - "offset" integer, initially set to zero and incremented by the handler as time
    //      passes to keep track of where it is.
    uint64_t fill_4kpaging_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params);

    //set_flags item handler : Sets the flags of a block of virtual addresses to a new value.
    //
    //additional_params contents :
    //  0 - New flags to be set
    uint64_t set_flags_handler(uint64_t vaddr,
                               const uint64_t size,
                               const PagingLevel level,
                               uint64_t &table_item,
                               uint64_t* additional_params);

    //setup_4kpages item handler : Sets up a range of virtual addresses in a process' address space
    //for 4KB paging, so that there's only physical addresses and flags at PT level left to fill.
    //Allocates paging structures when they're not allocated yet.
    //
    //additional_params contents :
    //  0 - Pointer to a PhyMemManager, used to allocate the nonexistent pages
    uint64_t setup_4kpages_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params);

    //remove_paging item handler : Removes all address translations in a range of virtual addresses,
    //freeing paging structures if they're not used anymore.
    //
    //additional_params contents :
    //  0 - Pointer to a PhyMemManager, used to free the useless paging structures
    uint64_t remove_paging_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params);
}

#endif
