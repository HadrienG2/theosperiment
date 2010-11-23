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

#include <align.h>
#include <hack_stdint.h>
#include <kmaths.h>
#include <x86paging.h>
#include <x86asm.h>

namespace x86paging {
  void create_pml4t(uint64_t location) {
    uint64_t* pointer = (uint64_t*) location;
    for(int i=0; i<512; ++i) pointer[i] = 0;
  }
  
  void fill_4kpaging(const uint64_t phy_addr,
                       uint64_t vir_addr,
                       const uint64_t length,
                       uint64_t flags,
                       const uint64_t pml4t_location) {
    pml4e* pml4t = (pml4e*) pml4t_location;
    pdp* pdpt;
    pde* pd;
    pte* pt;
    uint64_t tmp, offset;
    int pt_index, pd_index, pdpt_index, pml4t_index;
    int pt_len, pd_len, pdpt_len, pml4t_len;
    
    //Determine where the virtual address is located in the paging structures
    tmp = vir_addr/0x1000;
    pt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pd_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pdpt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pml4t_index = tmp%PTABLE_LENGTH;
    
    //Determine on how much paging structures the vmem block spreads
    tmp = (uint64_t) align_up(length, 0x1000)/0x1000; //In pages
    const int first_pt_len = min(tmp, PTABLE_LENGTH-pt_index);
    const int last_pt_len = (tmp-first_pt_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page tables
    const int first_pd_len = min(tmp, PTABLE_LENGTH-pd_index);
    const int last_pd_len = (tmp-first_pd_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page directories
    const int first_pdpt_len = min(tmp, PTABLE_LENGTH-pdpt_index);
    const int last_pdpt_len = (tmp-first_pdpt_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In PDPTs
    pml4t_len = tmp%(PTABLE_LENGTH-pml4t_index);
    
    //Fill paging structures
    for(int pml4t_parser = 0; pml4t_parser < pml4t_len; ++pml4t_parser) { //PML4T parsing
      pdpt = (pdp*) (pml4t[pml4t_index+pml4t_parser] & 0x000ffffffffff000);
      
      //Know which PDPTs we're going to parse
      if(pml4t_parser == 0) pdpt_len = first_pdpt_len;
      if(pml4t_parser == 1) {
        pdpt_index = 0; //pdpt_index is only valid for the first PDPT we consider.
        pdpt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
      }
      if(pml4t_parser == pml4t_len-1) pdpt_len = last_pdpt_len; //...except for the last one.
      
      //Parse them
      for(int pdpt_parser = 0; pdpt_parser < pdpt_len; ++pdpt_parser) { //PDPT parsing
        pd = (pde*) (pdpt[pdpt_index+pdpt_parser] & 0x000ffffffffff000);
        
        //Know which PDs we're going to parse
        if(pdpt_parser == 0 && pml4t_parser == 0) pd_len = first_pd_len;
        if(pdpt_parser == 1 && pml4t_parser == 0) {
          pd_index = 0; //pd_index is only valid for the very first PD we consider.
          pd_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
        }
        if(pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pd_len = last_pd_len; //...except for the last one.
        
        //Parse them
        for(int pd_parser = 0; pd_parser < pd_len; ++pd_parser) { //PD parsing
          pt = (pte*) (pd[pd_index+pd_parser] & 0x000ffffffffff000);
          
          //Know which PT entries we're going to parse
          if(pd_parser == 0 && pdpt_parser == 0 && pml4t_parser == 0) pt_len = first_pt_len;
          if(pd_parser == 1 && pdpt_parser == 0 && pml4t_parser == 0) {
            pt_index = 0; //pt_index is only valid for the very first PT we consider.
            pt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
          }
          if(pd_parser == pd_len-1 && pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pt_len = last_pt_len; //...except for the last one.
          
          //Fill this page table
          for(int pt_parser = 0; pt_parser < pt_len; ++pt_parser) { //PT level : filling page table
            pt[pt_index+pt_parser] = phy_addr+offset+flags;
            offset+=0x1000;
          }
        }
      }
    }
  }

  uint64_t find_lowestpaging(const uint64_t vaddr, const uint64_t pml4t_location) {
    uint64_t tmp, pt_index, pd_index, pdpt_index, pml4_index;
    pml4e* pml4;
    pdp* pdpt;
    pde* pd;
    pte* pt;

    pml4 = (pml4e*) pml4t_location;
    
    //We assume 4KB paging first. Things will be then adjusted as needed.
    tmp = vaddr/0x1000;
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

  uint64_t get_target(const uint64_t vaddr, const uint64_t pml4t_location) {
    uint64_t tmp, pt_index, pd_index, pdpt_index, pml4_index;
    pml4e* pml4;
    pdp* pdpt;
    pde* pd;
    pte* pt;
    
    pml4 = (pml4e*) pml4t_location;
    
    //We assume 4KB paging first. Things will be then adjusted as needed.
    tmp = vaddr/0x1000;
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
    
    //If 1GB pages are on, we have reached the lowest level of paging hierarchy, return the physical address
    if(pdpt[pdpt_index] & PBIT_LARGEPAGE) return (uint64_t) ((pdpt[pdpt_index] & 0x000fffffc0000000) + (vaddr & 0x3fffffff));
    //Else check that PDPT entry exists, if not the address is invalid, if it does go to the next level
    if(!(pdpt[pdpt_index] & PBIT_PRESENT)) return 0;
    pd = (pde*) (pdpt[pdpt_index] & 0x000ffffffffff000);
    
    //If 2MB pages are on, we have reached the lowest level of paging hierarchy, return the physical address
    if(pd[pd_index] & PBIT_LARGEPAGE) return (uint64_t) ((pd[pd_index] & 0x000fffffffe00000) + (vaddr & 0x1fffff));
    //Else check that PD entry exists, if not the address is invalid, if it does go to the next level
    if(!(pd[pd_index] & PBIT_PRESENT)) return 0;
    pt = (pte*) (pd[pd_index] & 0x000ffffffffff000);
    
    //Return the physical address
    return (uint64_t) ((pt[pt_index] & 0x000ffffffffff000) + (vaddr & 0xfff));
  }

  uint64_t get_pml4t() {
    uint64_t cr3;
    rdcr3(cr3);
    return cr3 & 0x000ffffffffff000;
  }
  
  void set_flags(uint64_t vaddr, const uint64_t length, uint64_t flags, uint64_t pml4t_location) {
    pml4e* pml4t = (pml4e*) pml4t_location;
    pdp* pdpt;
    pde* pd;
    pte* pt;
    uint64_t tmp;
    int pt_index, pd_index, pdpt_index, pml4t_index;
    int pt_len, pd_len, pdpt_len, pml4t_len;
    
    //Determine where the virtual address is located in the paging structures
    tmp = vaddr/0x1000;
    pt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pd_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pdpt_index = tmp%PTABLE_LENGTH;
    tmp/= PTABLE_LENGTH;
    pml4t_index = tmp%PTABLE_LENGTH;
    
    //Determine on how much paging structures the vmem block spreads
    tmp = (uint64_t) align_up(length, 0x1000)/0x1000; //In pages
    const int first_pt_len = min(tmp, PTABLE_LENGTH-pt_index);
    const int last_pt_len = (tmp-first_pt_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page tables
    const int first_pd_len = min(tmp, PTABLE_LENGTH-pd_index);
    const int last_pd_len = (tmp-first_pd_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In page directories
    const int first_pdpt_len = min(tmp, PTABLE_LENGTH-pdpt_index);
    const int last_pdpt_len = (tmp-first_pdpt_len)%PTABLE_LENGTH;
    tmp = align_up(tmp, PTABLE_LENGTH)/PTABLE_LENGTH; //In PDPTs
    pml4t_len = tmp%(PTABLE_LENGTH-pml4t_index);
    
    //Fill paging structures
    for(int pml4t_parser = 0; pml4t_parser < pml4t_len; ++pml4t_parser) { //PML4T parsing
      pdpt = (pdp*) (pml4t[pml4t_index+pml4t_parser] & 0x000ffffffffff000);
      
      //Know which PDPTs we're going to parse
      if(pml4t_parser == 0) pdpt_len = first_pdpt_len;
      if(pml4t_parser == 1) {
        pdpt_index = 0; //pdpt_index is only valid for the first PDPT we consider.
        pdpt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
      }
      if(pml4t_parser == pml4t_len-1) pdpt_len = last_pdpt_len; //...except for the last one.
      
      //Parse them
      for(int pdpt_parser = 0; pdpt_parser < pdpt_len; ++pdpt_parser) { //PDPT parsing
        pd = (pde*) (pdpt[pdpt_index+pdpt_parser] & 0x000ffffffffff000);
        
        //Know which PDs we're going to parse
        if(pdpt_parser == 0 && pml4t_parser == 0) pd_len = first_pd_len;
        if(pdpt_parser == 1 && pml4t_parser == 0) {
          pd_index = 0; //pd_index is only valid for the very first PD we consider.
          pd_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
        }
        if(pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pd_len = last_pd_len; //...except for the last one.
        
        //Parse them
        for(int pd_parser = 0; pd_parser < pd_len; ++pd_parser) { //PD parsing
          pt = (pte*) (pd[pd_index+pd_parser] & 0x000ffffffffff000);
          
          //Know which PT entries we're going to parse
          if(pd_parser == 0 && pdpt_parser == 0 && pml4t_parser == 0) pt_len = first_pt_len;
          if(pd_parser == 1 && pdpt_parser == 0 && pml4t_parser == 0) {
            pt_index = 0; //pt_index is only valid for the very first PT we consider.
            pt_len = PTABLE_LENGTH; //The others are browsed from the beginning to the end...
          }
          if(pd_parser == pd_len-1 && pdpt_parser == pdpt_len-1 && pml4t_parser == pml4t_len-1) pt_len = last_pt_len; //...except for the last one.
          
          //Fill this page table
          for(int pt_parser = 0; pt_parser < pt_len; ++pt_parser) { //PT level : switching flags
            pt[pt_index+pt_parser] = (pt[pt_index+pt_parser] & 0x000ffffffffff000)+flags;
          }
        }
      }
    }
  }
}