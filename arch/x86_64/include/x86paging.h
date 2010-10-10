 /* Paging-related helper functions and defines

      Copyright (C) 2010  Hadrien Grasland

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

typedef uint64_t pte; //Page table entry
typedef uint64_t pde; //Page directory entry
typedef uint64_t pdp; //Page directory pointer
typedef uint64_t pml4e; //PML4 entry

/* Sensible bits in page tables */
#define PBIT_PRESENT              1
#define PBIT_WRITABLE             (1<<1)
#define PBIT_USERACCESS           (1<<2)
#define PBIT_WRITETHROUGH         (1<<3)  //Slower than writeback, and not useful on a non-distributed system. Avoid it.
#define PBIT_NOCACHE              (1<<4)
#define PBIT_ACCESSED             (1<<5)
#define PBIT_DIRTY                (1<<6)  //Only present at the PTE level of hierarchy
#define PBIT_LARGEPAGE            (1<<7)  //Only present at the PDE/PDPE level of hierarchy
#define PBIT_PAGEATTRIBTAB_4KB    (1<<7)  //Only present at the PTE level
#define PBIT_PAGEATTRIBTAB_LARGE  (1<<7)  //Only present at the lowest level
#define PBIT_GLOBALPAGE           (1<<8)  //Only present at PTE level, TLB entry not invalidated on context switch by MOV CRn
#define PBIT_NOEXECUTE            0x8000000000000000 //Prevents execution (can't be written in bitshift form because we still handle 32b data)
/* Other useful data... */
#define PTABLE_LENGTH   512     //Size of a table/directory/... in entries
#define PENTRY_SIZE     8       //Size of a paging structure entry in bytes

uint64_t find_lowestpaging(uint64_t vaddr); //Find the lowest level of paging structures associated with a linear address, if it exists.
uint64_t get_target(uint64_t vaddr); //Get the physical memory address associated with a virtual address (if it does exist).
uint64_t get_pml4t(); //Return address of the current PML4T

#endif