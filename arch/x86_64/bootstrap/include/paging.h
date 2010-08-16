/*  Everything needed to setup 4KB-PAE-64b paging with identity mapping

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

#include <align.h>
#include <bs_kernel_information.h>
#include <stdint.h>

/* Page-table entries bits */
#define PBIT_PRESENT         1
#define PBIT_WRITABLE        (1<<1)
#define PBIT_USERACCESS      (1<<2)
#define PBIT_WRITETHROUGH    (1<<3)  //Slower than writeback, and not useful on a non-distributed system. Avoid it.
#define PBIT_NOCACHE         (1<<4)
#define PBIT_ACCESSED        (1<<5)
#define PBIT_DIRTY           (1<<6)  //Only present at the PTE level of hierarchy
#define PBIT_LARGEPAGE       (1<<7)  //Only present at the PDE/PDPE level of hierarchy
#define PBIT_PAGEATTRIBTABLE (1<<7)  //Only present at the PTE level
#define PBIT_GLOBALPAGE      (1<<8)  //Only present at PTE level, TLB entry not invalidated on context switch by MOV CRn
#define PBIT_NOEXECUTE       0x8000000000000000 //Prevents execution (can't be written in bitshift form because we still handle 32b data)
/* Some numeric data... */
#define PML4T_SIZE      512     //Size of a page table/directory/... in entries
#define PDPT_SIZE       512
#define PD_SIZE         512
#define PT_SIZE         512
#define PENTRY_SIZE     8       //Size of a paging structure entry in bytes
#define PHYADDR_ALIGN   4096    //All physical addresses in a PT are aligned on a 4KB basis.
                                //This allows using low-order bits for configuration purposes. 

typedef uint64_t pml4e; /* Page-Map Level-4 Entry */
typedef uint64_t pdpe;  /* Page-Directory Pointer Entry */
typedef uint64_t pde;   /* Page-Directory Entry */
typedef uint64_t pte;   /* Page-Table Entry */

// Finds and returns the privileges associated with a memory map region.
//    0 = R-X
//    1 = R--
//    2 = RW-
int find_map_region_privileges(KernelMemoryMap* map_region);
// Set up paging structures and return CR3 value
uint32_t generate_paging(KernelInformation* kinfo);
// Make a identity-mapped page directory knowing the page table's position and length.
// Return its length in bytes
unsigned int make_identity_page_directory(unsigned int location, unsigned int pt_location, unsigned int pt_length);
// Make an identity-mapped page table from a KernelInformation structure and return its length in bytes
unsigned int make_identity_page_table(unsigned int location, KernelInformation* kinfo);
// Same for PDPT and PML4T
unsigned int make_identity_pdpt(unsigned int location, unsigned int pd_location, unsigned int pd_length);
unsigned int make_identity_pml4t(unsigned int location, unsigned int pdpt_location, unsigned int pdpt_length);
// Set up stack protection
void protect_stack(uint32_t cr3_value);
// Set up a page translation (allocating required structures if needed)
void setup_pagetranslation(KernelInformation* kinfo, uint32_t cr3_value, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

#endif