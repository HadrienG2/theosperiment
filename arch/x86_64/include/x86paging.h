 /* Paging-related helper functions and defines

      Copyright (C) 2010-2011  Hadrien Grasland

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

#ifndef _X86PAGING_H_
#define _X86PAGING_H_

#include <physmem.h>

namespace x86paging {
    typedef uint64_t pte; //Page table entry
    typedef uint64_t pde; //Page directory entry
    typedef uint64_t pdp; //Page directory pointer
    typedef uint64_t pml4e; //PML4 entry

    /* Sensible bits in page tables (read Intel/AMD manuals for mor details) */
    const uint64_t PBIT_PRESENT = 1; //Page is present, may be accessed.
    const uint64_t PBIT_WRITABLE = (1<<1); //User-mode software may write data in this page.
    const uint64_t PBIT_USERACCESS = (1<<2); //User-mode software has access to this page.
    const uint64_t PBIT_NOCACHE = (1<<4); //This page cannot be cached by the CPU.
    const uint64_t PBIT_ACCESSED = (1<<5); //Bit set by the CPU : the paging structure
                                           //has been accessed by software.
    const uint64_t PBIT_DIRTY = (1<<6); //Only present at the page level of hierarchy.
                                        //Set by the CPU : data has been written in this page.
    const uint64_t PBIT_LARGEPAGE = (1<<7); //Only present at the PDE/PDPE level of hierarchy.
                                            //Indicates that pages larger than 4KB are being used.
    const uint64_t PBIT_GLOBALPAGE = (1<<8); //Only present at the page level of hierarchy.
                                             //TLB entry not invalidated on context switch.
    const uint64_t PBIT_NOEXECUTE = 0x8000000000000000; //Prevents execution of data referenced by
                                                        //this paging structure.

    /* Other useful data... */
    const int PTABLE_LENGTH = 512; //Size of a table/directory/... in entries
    const int PENTRY_SIZE = 8; //Size of a paging structure entry in bytes

    void create_pml4t(uint64_t location); //Create an empty PML4T at that location

    void fill_4kpaging(const uint64_t phy_addr,     //Have "length" bytes of physical memory,
                       uint64_t vir_addr,           //starting at phy_addr, be mapped in the
                       const uint64_t size,         //virtual address space of a process,
                       uint64_t flags,              //starting at vir_addr.
                       uint64_t pml4t_location);    //(This function assumes that paging
                                                    //structures are already allocated and
                                                    //set up for 4k paging.)

    uint64_t find_lowestpaging(const uint64_t vaddr,           //Find the lowest level of paging
                               const uint64_t pml4t_location); //structures associated with a linear
                                                               //address, return 0 if that address
                                                               //is invalid.

    uint64_t get_target(const uint64_t vaddr,         //Get the physical memory address associated
                        const uint64_t pml4t_location); //with a linear address (if it does exist).

    uint64_t get_pml4t(); //Return address of the current PML4T

    bool remove_paging(uint64_t vir_addr,  //Remove page translations in a virtual address range
                       const uint64_t size,
                       uint64_t pml4t_location,
                       PhyMemManager* phymem);

    uint64_t setup_4kpages(uint64_t vir_addr,          //Setup paging structures for 4KB paging in
                           const uint64_t size,        //a virtual address range.
                           uint64_t pml4t_location,
                           PhyMemManager* phymem);

    void set_flags(uint64_t vaddr,         //Sets a whole linear address block's paging flags to
                   const uint64_t size,    //"flags"
                   uint64_t flags,
                   uint64_t pml4t_location);
}

#endif