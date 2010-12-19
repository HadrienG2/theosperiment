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

/* Sensible bits in page tables (read Intel/AMD manuals for mor details) */
extern const uint64_t PBIT_PRESENT; //Page is present, may be accessed.
extern const uint64_t PBIT_WRITABLE; //User-mode software may write data in this page.
extern const uint64_t PBIT_USERACCESS; //User-mode software has access to this page.
extern const uint64_t PBIT_NOCACHE; //This page cannot be cached by the CPU.
extern const uint64_t PBIT_ACCESSED; //Bit set by the CPU : the paging structure
                                     //has been accessed by software.
extern const uint64_t PBIT_DIRTY; //Only present at the page level of hierarchy.
                                  //Set by the CPU : data has been written in this page.
extern const uint64_t PBIT_LARGEPAGE; //Only present at the PDE/PDPE level of hierarchy.
                                      //Indicates that pages larger than 4KB are being used.
extern const uint64_t PBIT_GLOBALPAGE; //Only present at the page level of hierarchy.
                                       //TLB entry not invalidated on context switch.
extern const uint64_t PBIT_NOEXECUTE; //Prevents execution of data referenced by
                                      //this paging structure.
                                                     
/* Some numeric data... */
#define PML4T_SIZE 512 //Size of a page table/directory/... in entries
#define PDPT_SIZE 512
#define PD_SIZE 512
#define PT_SIZE 512
#define PENTRY_SIZE 8 //Size of a paging structure entry in bytes
#define PHYADDR_ALIGN 4096 //Physical addresses in a PT are aligned on a 4KB basis.
                           //Low-order bits are used for configuration purposes. 

typedef uint64_t pml4e; /* Page-Map Level-4 Entry */
typedef uint64_t pdpe;  /* Page-Directory Pointer Entry */
typedef uint64_t pde;   /* Page-Directory Entry */
typedef uint64_t pte;   /* Page-Table Entry */

// Finds and returns the privileges associated with a memory map region.
//    0 = R-X
//    1 = R--
//    2 = RW-
int find_map_region_privileges(const KernelMemoryMap* map_region);
// Set up paging structures and return CR3 value
uint32_t generate_paging(KernelInformation* kinfo);
// Make a identity-mapped page directory knowing the page table's position and length.
// Return its length in bytes
unsigned int make_identity_page_directory(const unsigned int location,
                                          const unsigned int pt_location,
                                          const unsigned int pt_length);
// Make an identity-mapped page table from a KernelInformation structure and return its length in bytes
unsigned int make_identity_page_table(const unsigned int location, const KernelInformation* kinfo);
// Same for PDPT and PML4T
unsigned int make_identity_pdpt(const unsigned int location, const unsigned int pd_location, const unsigned int pd_length);
unsigned int make_identity_pml4t(const unsigned int location, const unsigned int pdpt_location, const unsigned int pdpt_length);
// Set up stack protection
void protect_stack(const uint32_t cr3_value);
// Set up a page translation (allocating required structures if needed)
void setup_pagetranslation(KernelInformation* kinfo,
                           const uint32_t cr3_value,
                           const uint64_t virt_addr,
                           const uint64_t phys_addr,
                           const uint64_t flags);

#endif