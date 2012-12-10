 /* Function parsing x86's multilevel page tables

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

#include <align.h>
#include <kmaths.h>
#include <phymem_manager.h>
#include <x86paging.h>
#include <x86paging_parser.h>

namespace x86paging {
    //As these constants can have the value we want, we give them the value of the bitshift
    //which must be applied to a virtual address or length in order to get the index in the relevant
    //table.
    const PagingLevel PML4T_LEVEL = 39;
    const PagingLevel PDPT_LEVEL = 30;
    const PagingLevel PD_LEVEL = 21;
    const PagingLevel PT_LEVEL = 12;
    const PagingLevel LVL_DECREMENT = 9; //Substract this from the current level to go to the next
                                         //level

    uint64_t paging_parser(uint64_t vaddr,
                           const uint64_t size,
                           const PagingLevel level,
                           uint64_t* page_table,
                           uint64_t (*item_handler)(uint64_t vaddr,
                                                    const uint64_t size,
                                                    const PagingLevel level,
                                                    uint64_t &table_item,
                                                    uint64_t* additional_params),
                           uint64_t* additional_params) {
        //The size of the virtual address space covered by each item of our table.
        const uint64_t ITEM_SIZE = 1 << level;

        //The first item which we're going to parse in the table
        const int first_index = (vaddr >> level)%PTABLE_LENGTH;

        //The number of items which we're going to parse
        int parsed_length = size >> level;
        if(size % ITEM_SIZE) {
            ++parsed_length;
        }
        //Check that the requested size does not imply overflowing the table. If it does, abort.
        if(parsed_length > PTABLE_LENGTH-first_index) return 0;

        //To make the job of el_handler easier, we will adjust the values of vaddr and the size it
        //receives, so that they are aligned on a table item boundary. Here, we define some
        //constants used for that
        const uint64_t vaddr_base = align_down(vaddr, ITEM_SIZE);
        const uint64_t size_first = min(size, align_up(vaddr, ITEM_SIZE)-vaddr);
        const uint64_t size_middle = ITEM_SIZE;
        uint64_t size_last = (size-size_first)%ITEM_SIZE;
        if(!size_last) size_last = ITEM_SIZE;

        //Parse the table. Make sure result is nonzero for a success even if size is zero.
        uint64_t result = 1;
        for(int table_parser = 0; table_parser < parsed_length; ++table_parser) {
            //Adjust values of vaddr and size sent to el_handler
            uint64_t eff_vaddr;
            uint64_t eff_size;
            if(table_parser == 0) {
                eff_vaddr = vaddr;
                eff_size = size_first;
            } else {
                eff_vaddr = vaddr_base+table_parser*ITEM_SIZE;
                if(table_parser == 1) eff_size = size_middle;
                if(table_parser == parsed_length-1) eff_size = size_last;
            }

            //Call item_handler on each table item
            result = item_handler(eff_vaddr,
                                  eff_size,
                                  level,
                                  page_table[first_index+table_parser],
                                  additional_params);
            if(!result) return 0; //item_handler has failed ! Abort.
        }

        return result;
    }

    uint64_t dummy_handler(uint64_t vaddr,
                           const uint64_t size,
                           const PagingLevel level,
                           uint64_t &table_item,
                           uint64_t* additional_params) {
        //On which level are we ?
        switch(level) {
            case PT_LEVEL:
                //We have reached the page table level, return first additional parameter
                return additional_params[0];
            default:
                //Extract where the next table should be located (if there's one)
                uint64_t* next_table = (uint64_t*) (table_item & 0x000ffffffffff000);

                //Check if we've reached the lowest accessible level, return 0 in that case
                if((table_item & PBIT_PRESENT) == 0) return 0;
                if(!next_table) return 0;

                //Otherwise, parse to the next level
                return paging_parser(vaddr,
                                     size,
                                     level-LVL_DECREMENT,
                                     next_table,
                                     &dummy_handler,
                                     additional_params);
        }
    }

    uint64_t fill_4kpaging_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params) {
        //Have we reached the page table level ? If so, setup a page translation.
        if(level == PT_LEVEL) {
            table_item = (additional_params[0]&0x000ffffffffff000)      //Physical base address
                          + additional_params[2]                        //Offset
                          + additional_params[1];                       //Flags
            additional_params[2]+= 0x1000; //Make "offset" move one 4KB page forward
            return 1;
        }

        //Otherwise, move to the next level of paging structures
        uint64_t* next_table = (uint64_t*) (table_item & 0x000ffffffffff000);
        return paging_parser(vaddr,
                                size,
                                level-LVL_DECREMENT,
                                next_table,
                                &fill_4kpaging_handler,
                                additional_params);
    }

    uint64_t set_flags_handler(uint64_t vaddr,
                               const uint64_t size,
                               const PagingLevel level,
                               uint64_t &table_item,
                               uint64_t* additional_params) {
        //Are we at the lowest level of paging structures ?
        //If so, overwrite flags and quit.
        if((level == PT_LEVEL) || (table_item & PBIT_LARGEPAGE)) {
            table_item = (table_item & 0x000ffffffffff000) + additional_params[0];
            return 1;
        }

        //Otherwise, move to the next level of paging structures
        uint64_t* next_table = (uint64_t*) (table_item & 0x000ffffffffff000);
        return paging_parser(vaddr,
                             size,
                             level-LVL_DECREMENT,
                             next_table,
                             &set_flags_handler,
                             additional_params);
    }

    uint64_t setup_4kpages_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params) {
        //Check if next level of paging structures is available.
        uint64_t* next_table = (uint64_t*) (table_item & 0x000ffffffffff000);
        if(!next_table) {
            //If not, allocate paging structures
            PhyMemManager* phymem_manager = (PhyMemManager*) additional_params[0];
            PhyMemChunk* allocd_page = phymem_manager->alloc_chunk(PID_KERNEL);
            if(!allocd_page) return 0;
            next_table = (uint64_t*) allocd_page->location;

            //These should be zeroed out before use, to prevent errors and security exploits in
            //case they aren't initialized properly later.
            for(int index = 0; index < PTABLE_LENGTH; ++index) next_table[index] = 0;

            //Now we can use them
            table_item = allocd_page->location + PBIT_PRESENT       //All paging protections
                                               + PBIT_WRITABLE      //are disabled at this
                                               + PBIT_USERACCESS;   //level of paging structs.
        }

        //If we're at the PD level, we're done with our allocation job. Otherwise, move to the next
        //level of paging structures.
        if(level == PD_LEVEL) return 1;
        return paging_parser(vaddr,
                             size,
                             level-LVL_DECREMENT,
                             next_table,
                             &setup_4kpages_handler,
                             additional_params);
    }

    uint64_t remove_paging_handler(uint64_t vaddr,
                                   const uint64_t size,
                                   const PagingLevel level,
                                   uint64_t &table_item,
                                   uint64_t* additional_params) {
        //Have we reached the lowest level of paging structures ?
        //If so, clear any existing page translation.
        if((level == PT_LEVEL) || (table_item & PBIT_LARGEPAGE)) {
            table_item = 0;
            return 1;
        }

        //Otherwise, if the next level of paging structures is already nonexistent, skip it
        if(!table_item) return 1;

        //Parse the next level of paging structures
        uint64_t* next_table = (uint64_t*) (table_item & 0x000ffffffffff000);
        uint64_t result = paging_parser(vaddr,
                                        size,
                                        level-LVL_DECREMENT,
                                        next_table,
                                        &remove_paging_handler,
                                        additional_params);
        if(!result) return 0;

        //If the previously parsed table is now empty, free it
        bool is_empty = true;
        for(int index = 0; index < PTABLE_LENGTH; ++index) {
            if(next_table[index]) {
                is_empty = false;
                break;
            }
        }
        if(is_empty) {
            PhyMemManager* phymem_manager = (PhyMemManager*) additional_params[0];
            table_item = 0;
            phymem_manager->free_chunk(PID_KERNEL, (uint64_t) next_table);
        }

        return result;
    }
}
