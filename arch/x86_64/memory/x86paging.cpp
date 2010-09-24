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
 
#include <hack_stdint.h>
#include <x86paging.h>
#include <x86asm.h>

uint64_t find_lowestpaging(uint64_t addr) {
  uint64_t cr3, tmp, pt_index, pd_index, pdpt_index, pml4_index;
  pml4e* pml4;
  pdp* pdpt;
  pde* pd;
  pte* pt;
  
  rdcr3(cr3);
  pml4 = (pml4e*) (cr3 & 0x000ffffffffff000);
  
  //We assume 4KB paging first. Things will be then adjusted as needed.
  tmp = addr/0x1000;
  pt_index = tmp%PTABLE_LENGTH;
  tmp/= PTABLE_LENGTH;
  pd_index = tmp%PTABLE_LENGTH;
  tmp/= PTABLE_LENGTH;
  pdpt_index = tmp%PTABLE_LENGTH;
  tmp/= PTABLE_LENGTH;
  pml4_index = tmp%PTABLE_LENGTH;
  
  //Check that PML4 entry exists, if not the address is invalid.
  if(!(pml4[pml4_index] & PBIT_PRESENT)) return 0;
  pdpt = (pdp*) (pml4[pml4_index] & 0x000ffffffffff000);
  
  //If 1GB pages are on, we have reached the lowest level of paging hierarchy, return the appropriate PDP.
  if(pdpt[pdpt_index] & PBIT_LARGEPAGE) return (uint64_t) pdpt[pdpt_index];
  //Else check that PDPT entry exists, if not the address is invalid, if it does go to the next level
  if(!(pdpt[pdpt_index] & PBIT_PRESENT)) return 0;
  pd = (pde*) (pdpt[pdpt_index] & 0x000ffffffffff000);
  
  //If 2MB pages are on, we have reached the lowest level of paging hierarchy, return the appropriate PDE.
  if(pd[pd_index] & PBIT_LARGEPAGE) return (uint64_t) pd[pd_index];
  //Else check that PD entry exists, if not the address is invalid, if it does go to the next level
  if(!(pd[pd_index] & PBIT_PRESENT)) return 0;
  pt = (pte*) (pd[pd_index] & 0x000ffffffffff000);
  
  //Return the appropriate PTE
  return (uint64_t) pt[pt_index];
}