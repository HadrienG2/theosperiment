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

#include <bs_string.h>
#include <die.h>
#include <gen_kernel_info.h>
#include <paging.h>

int find_map_region_privileges(kernel_memory_map* map_region) {
  //Free and reserved segments are considered as RW
  if(map_region->nature <= 1) return 2;
  //Bootstrap segments are considered as R
  if(map_region->nature == 2) return 1;
  //Kernel elements are set up according to their specified permission
  if(map_region->nature == 3) {
    if(strcmp((char*) (uint32_t) map_region->name, "Kernel RW- segment")==0) return 2;
    if(strcmp((char*) (uint32_t) map_region->name, "Kernel R-X segment")==0) return 0;
    //It's either a R kernel segment or a module
    return 1;
  }
  //Well, I don't know (control should never reach this point)
  return -1;
}

uint64_t generate_paging(kernel_information* kinfo) {
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  uint32_t kmmap_size = kinfo->kmmap_size;
  uint32_t pt_length, pd_length, pdpt_length, pml4t_length;
  uint64_t cr3_value;

  /* We'll do the following :
  
     Step 1 : Locate the place where the page table will be stored (after the RW- segment of the kernel)
     Step 2 : Fill in a page table (aligning it on a 2^9 entry boundary with empty entries).
     Step 3 : Make page directories from the page table
     Step 4 : Make page directory pointers table from the page directory
     Step 5 : Make PML4
     Step 6 : Mark used memory as such on the memory map, sort and merge.
     Step 7 : Generate and return CR3 value. */
  
  uint32_t first_blank = locate_first_blank(kmmap, kmmap_size);

  pt_length = make_page_table(kmmap[first_blank].location, kinfo);
  
  pd_length = make_page_directory(kmmap[first_blank].location, pt_length);
  
  pdpt_length = make_pdpt(kmmap[first_blank].location+pt_length, pd_length);
  
  pml4t_length = make_pml4t(kmmap[first_blank].location+pt_length+pd_length, pdpt_length);
  
  if(kmmap_size+3 >= MAX_KMMAP_SIZE) die(MMAP_TOO_SMALL);
  kmmap[kmmap_size].location = kmmap[first_blank].location;
  kmmap[kmmap_size].size = pt_length;
  kmmap[kmmap_size].nature = 3;
  kmmap[kmmap_size].name = (uint32_t) "Kernel page tables";
  kmmap[kmmap_size+1].location = kmmap[first_blank].location+pt_length;
  kmmap[kmmap_size+1].size = pd_length;
  kmmap[kmmap_size+1].nature = 3;
  kmmap[kmmap_size+1].name = (uint32_t) "Kernel page directories";
  kmmap[kmmap_size+2].location = kmmap[first_blank].location+pt_length+pd_length;
  kmmap[kmmap_size+2].size = pdpt_length;
  kmmap[kmmap_size+2].nature = 3;
  kmmap[kmmap_size+2].name = (uint32_t) "Kernel page directory pointers";
  kmmap[kmmap_size+3].location = kmmap[first_blank].location+pt_length+pd_length+pdpt_length;
  kmmap[kmmap_size+3].size = pml4t_length;
  kmmap[kmmap_size+3].nature = 3;
  kmmap[kmmap_size+3].name = (uint32_t) "Kernel PML4T";
  kinfo->kmmap_size+=4;
  sort_memory_map(kinfo);
  merge_memory_map(kinfo);
  
  cr3_value = kmmap[first_blank].location+pt_length+pd_length+pdpt_length;
  return cr3_value;
}

uint32_t locate_first_blank(kernel_memory_map* kmmap, uint32_t kmmap_size) {
  uint32_t first_blank;
  
  for(first_blank=0; first_blank<kmmap_size; ++first_blank) {
    if(strcmp((char*) (uint32_t) kmmap[first_blank].name, "Kernel RW- segment")==0) return first_blank+1;
  }
  return 0;
}

uint32_t make_page_directory(uint32_t pt_location, uint32_t pt_length) {
  pde* page_directory = (pde*) pt_location+pt_length;
  pde pde_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pt_location;
  uint64_t current_directory;
  
  for(current_directory = 0; current_directory<pt_length/(4*512); ++current_directory) {
    pde_entry_buffer += PT_SIZE*0x1000;
    page_directory[current_directory] = pde_entry_buffer;
  }
  
  for(; current_directory%PD_SIZE!=0; ++current_directory) page_directory[current_directory] = 0;
  
  return 4*current_directory;
}

uint32_t make_page_table(uint32_t location, kernel_information* kinfo) {
  uint32_t current_mmap_index = 0;
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  uint64_t current_page, current_region_end = kmmap[0].location + kmmap[0].size;
  pte pte_mask = PBIT_PRESENT+PBIT_NOEXECUTE+PBIT_WRITABLE; //"mask" being added to page table entries.
  pte pte_entry_buffer = pte_mask;
  pte* page_table = (pte*) location;
  
  int mode = 0; // Refers to the currently examined memory region.
                //    0 = In the middle of a normal mmap region
                //    1 = Non-mapped region
                //    2 = Region at the frontier between two memory map entries
  int privileges = 2; // 0 = R-X
                              // 1 = R--
                              // 2 = RW-
  
  //Paging all known memory regions
  for(current_page = 0; current_page*0x1000<kmmap[kinfo->kmmap_size-1].location; ++current_page) {
    pte_entry_buffer += 0x1000;
    //Writing page table entry in memory
    page_table[current_page] = pte_entry_buffer;
    
    
    // Checking if next page still is in the same memory map entry.
    if((current_page+2)*0x1000 > current_region_end) {
      //No. Check what's happening then.
      if((((current_page+2)*0x1000 > kmmap[current_mmap_index+1].location) &&
          ((current_page+1)*0x1000 < kmmap[current_mmap_index].location+kmmap[current_mmap_index].size) &&
          (current_mmap_index<kinfo->kmmap_size-1))
        || (((current_page+2)*0x1000 > kmmap[current_mmap_index+2].location) &&
          (current_mmap_index<kinfo->kmmap_size-2)))
      {
        //Our page overlaps with two distinct memory map entries.
        //This means that we're in the region where GRUB puts its stuff and requires specific care
          while((current_page+2)*0x1000 >= kmmap[current_mmap_index].location + kmmap[current_mmap_index].size) {
            ++current_mmap_index;
          }
          --current_mmap_index;
          mode=2;
          current_region_end = kmmap[current_mmap_index].location + kmmap[current_mmap_index].size;
      } else {
        if((current_page+2)*0x1000 < kmmap[current_mmap_index+1].location) {
          //Memory region is not mapped
          mode = 1;
          current_region_end = kmmap[current_mmap_index+1].location;
        } else {
          //We just reached a new region in memory
          ++current_mmap_index;
          mode=0;
          privileges = find_map_region_privileges(&(kmmap[current_mmap_index]));
          current_region_end = kmmap[current_mmap_index].location + kmmap[current_mmap_index].size;
        }
      }
      
      //Refreshing the page table entry mask
      pte_mask = PBIT_PRESENT;
      switch(mode) {
        case 0:
          switch(privileges) {
            case 0:
              break;
            case 1:
              pte_mask += PBIT_NOEXECUTE;
              break;
            case 2:
              pte_mask += PBIT_NOEXECUTE + PBIT_WRITABLE;
              break;
          }
        case 1:
          pte_mask += PBIT_WRITABLE + PBIT_NOEXECUTE;
          break;
        case 2:
          /* In overlapping cases, we encountered a grub segment or a module, because the rest is page aligned...
              Privilege is hence R-- */
          pte_mask += PBIT_NOEXECUTE;
          break;
      }
      //Refreshing the page table entry buffer
      pte_entry_buffer = current_page;
      pte_entry_buffer <<= PADD_BITSHIFT;
      pte_entry_buffer += pte_mask;
    }
  }
  
  //Padding page table with zeroes till it has proper alignment
  for(; current_page%PT_SIZE!=0; ++current_page) page_table[current_page] = 0;
  
  return current_page*4;
}

uint32_t make_pdpt(uint32_t pd_location, uint32_t pd_length) {
  pdpe* pdpt = (pdpe*) pd_location+pd_length;
  pdpe pdpe_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pd_location;
  uint64_t current_dp;
  
  for(current_dp = 0; current_dp<pd_length/(4*512); ++current_dp) {
    pdpe_entry_buffer += PD_SIZE*0x1000;
    pdpt[current_dp] = pdpe_entry_buffer;
  }

  for(; current_dp%PDPT_SIZE!=0; ++current_dp) pdpt[current_dp] = 0;

  return 4*current_dp;
}

uint32_t make_pml4t(uint32_t pdpt_location, uint32_t pdpt_length) {
  pml4e* pml4t = (pml4e*) pdpt_location+pdpt_length;
  pml4e pml4e_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pdpt_location;
  uint64_t current_ml4e;
  
  for(current_ml4e = 0; current_ml4e<pdpt_length/(4*512); ++current_ml4e) {
    pml4e_entry_buffer += PDPT_SIZE*0x1000;
    pml4t[current_ml4e] = pml4e_entry_buffer;
  }
  
  for(; current_ml4e<PML4T_SIZE; ++current_ml4e) pml4t[current_ml4e] = 0;
  
  return 4*PML4T_SIZE;
}