/*  Everything needed to setup 4KB-PAE-32b paging with identity mapping

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

#ifndef _PAGING_H_
#define _PAGING_H_

#include <kernel_information.h>

/* Page-table entries bits */
#define PBIT_PRESENT         1
#define PBIT_WRITABLE        (1<<1)
#define PBIT_USERACCESS      (1<<2)
#define PBIT_WRITETHROUGH    (1<<3)  //Slower than writeback, and not useful on a non-distributed system. Avoid it.
#define PBIT_NOCACHE         (1<<4)
#define PBIT_ACCESSED        (1<<5)
#define PBIT_DIRTY           (1<<6)  //Only present at the PTE level of hierarchy
#define PBIT_LARGEPAGE       (1<<7)  //Only present at the PDE/PDPE level of hierarchy
#define PBIT_GLOBALPAGE      (1<<8)  //Only present at PTE level, TLB entry not invalidated on context switch by MOV CRn
#define PBIT_PAGEATTRIBTABLE (1<<7)  //Only present at the PTE level
#define PBIT_NOEXECUTE       (1<<63) //Prevents execution of data. Not present at PML4E level
/* Some numeric data... */
#define PADD_BITSHIFT   12      //How many bits to the left the address must go
#define PML4T_SIZE      (1<<9)  //Size of the various element of the page table
#define PDPT_SIZE       (1<<9)
#define PDT_SIZE        (1<<9)
#define PT_SIZE         (1<<9)

typedef uint64_t pml4e; /* Page-Map Level-4 Entry */
typedef uint64_t pdpe;  /* Page-Directory Pointer Entry */
typedef uint64_t pde;   /* Page-Directory Entry */
typedef uint64_t pte;   /* Page-Table Entry */

/* Setup a page table for identity mapping */
pml4e* generate_pagetable(kernel_information* kinfo);

#endif