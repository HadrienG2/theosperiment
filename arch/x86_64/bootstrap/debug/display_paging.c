 /* Display paging structures on screen for debugging purposes

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
#include "display_paging.h"
#include <paging.h>
#include <txt_videomem.h>

void dbg_print_pd(const uint32_t location) {
  int table_index;
  const uint64_t mask = PG_SIZE-1+PBIT_NOEXECUTE; //Mask eliminating the low-order bits of CR3 and page table entries
  pde* pd = (pde*) location;  
  pde current_el;
  uint64_t address;
  
  //Print PD
  print_str("Address            | Flags\n");
  print_str("-------------------------------------------------------------------------------\n");
  for(table_index=0; table_index<PD_SIZE; ++table_index) {
    //For each table element, print address, isolated as before
    current_el = pd[table_index];
    if(current_el == 0) continue;
    address = current_el - (current_el & mask);
    print_hex64(address);
    print_str(" | ");
    if(current_el & PBIT_PRESENT) {
      print_str("PRST ");
    }
    if(current_el & PBIT_WRITABLE) {
      print_str("WRIT ");
    }
    if(current_el & PBIT_USERACCESS) {
      print_str("USER ");
    }
    if(current_el & PBIT_WRITETHROUGH) {
      print_str("WRTH ");
    }
    if(current_el & PBIT_NOCACHE) {
      print_str("!CAC ");
    }
    if(current_el & PBIT_ACCESSED) {
      print_str("ACSD ");
    }
    if(current_el & PBIT_LARGEPAGE) {
      print_str("LARG ");
    }
    if(current_el & PBIT_NOEXECUTE) {
      print_str("NOEX");
    }
    print_chr('\n');
  }
  print_chr('\n');
}

void dbg_print_pdpt(const uint32_t location) {
  int table_index;
  const uint64_t mask = PG_SIZE-1 + PBIT_NOEXECUTE; //Mask eliminating the low-order bits of CR3 and page table entries
  pdpe* pdpt = (pdpe*) location;  
  pdpe current_el;
  uint64_t address;
  
  //Print PDPT
  print_str("Address            | Flags\n");
  print_str("-------------------------------------------------------------------------------\n");
  for(table_index=0; table_index<PDPT_SIZE; ++table_index) {
    //For each table element, print address, isolated as before
    current_el = pdpt[table_index];
    if(current_el == 0) continue;
    address = current_el - (current_el & mask);
    print_hex64(address);
    print_str(" | ");
    if(current_el & PBIT_PRESENT) {
      print_str("PRST ");
    }
    if(current_el & PBIT_WRITABLE) {
      print_str("WRIT ");
    }
    if(current_el & PBIT_USERACCESS) {
      print_str("USER ");
    }
    if(current_el & PBIT_WRITETHROUGH) {
      print_str("WRTH ");
    }
    if(current_el & PBIT_NOCACHE) {
      print_str("!CAC ");
    }
    if(current_el & PBIT_ACCESSED) {
      print_str("ACSD ");
    }
    if(current_el & PBIT_LARGEPAGE) {
      print_str("LARG ");
    }
    if(current_el & PBIT_NOEXECUTE) {
      print_str("NOEX");
    }
    print_chr('\n');
  }
  print_chr('\n');
}

void dbg_print_pml4t(const uint32_t cr3_value) {
  int table_index;
  const uint64_t mask = PG_SIZE-1+PBIT_NOEXECUTE; //Mask eliminating the low-order bits of CR3 and page table entries
  uint32_t pml4t_location = cr3_value - (cr3_value & mask);
  pml4e* pml4t = (pml4e*) pml4t_location;  
  pml4e current_el;
  uint64_t address;
  
  //Print PML4T
  print_str("Address            | Flags\n");
  print_str("-------------------------------------------------------------------------------\n");
  for(table_index=0; table_index<PML4T_SIZE; ++table_index) {
    //For each table element, print address, isolated as before
    current_el = pml4t[table_index];
    if(current_el == 0) continue;
    address = current_el - (current_el & mask);
    print_hex64(address);
    print_str(" | ");
    if(current_el & PBIT_PRESENT) {
      print_str("PRST ");
    }
    if(current_el & PBIT_WRITABLE) {
      print_str("WRIT ");
    }
    if(current_el & PBIT_USERACCESS) {
      print_str("USER ");
    }
    if(current_el & PBIT_WRITETHROUGH) {
      print_str("WRTH ");
    }
    if(current_el & PBIT_NOCACHE) {
      print_str("!CAC ");
    }
    if(current_el & PBIT_ACCESSED) {
      print_str("ACSD ");
    }
    if(current_el & PBIT_NOEXECUTE) {
      print_str("NOEX");
    }
    print_chr('\n');
  }
  print_chr('\n');
}

void dbg_print_pt(const uint32_t location) {
  int table_index;
  const uint64_t mask = PG_SIZE-1+PBIT_NOEXECUTE; //Mask eliminating the low-order bits of CR3 and page table entries
  pte* pt = (pte*) location;  
  pte current_el;
  uint64_t address;
  
  //Print PT
  print_str("Address            | Flags\n");
  print_str("-------------------------------------------------------------------------------\n");
  for(table_index=0; table_index<PT_SIZE; ++table_index) {
    //For each table element, print address, isolated as before
    current_el = pt[table_index];
    if(current_el == 0) continue;
    address = current_el - (current_el & mask);
    print_hex64(address);
    print_str(" | ");
    if(current_el & PBIT_PRESENT) {
      print_str("PRST ");
    }
    if(current_el & PBIT_WRITABLE) {
      print_str("WRIT ");
    }
    if(current_el & PBIT_USERACCESS) {
      print_str("USER ");
    }
    if(current_el & PBIT_WRITETHROUGH) {
      print_str("WRTH ");
    }
    if(current_el & PBIT_NOCACHE) {
      print_str("!CAC ");
    }
    if(current_el & PBIT_ACCESSED) {
      print_str("ACSD ");
    }
    if(current_el & PBIT_PAGEATTRIBTABLE) {
      print_str("PATT ");
    }
    if(current_el & PBIT_GLOBALPAGE) {
      print_str("GLOB ");
    }
    if(current_el & PBIT_NOEXECUTE) {
      print_str("NOEX");
    }
    print_chr('\n');
  }
  print_chr('\n');
}