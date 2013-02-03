/*    Everything needed to setup 4KB-PAE-64b paging with identity mapping

            Copyright (C) 2010    Hadrien Grasland

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#include <align.h>
#include <bs_string.h>
#include <die.h>
#include <kinfo_handling.h>
#include <paging.h>

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

const char* PAGE_TABLE_NAME = "Kernel page tables";
const char* PAGE_DIR_NAME = "Kernel page directories";
const char* PDPT_NAME = "Kernel page directory pointers";
const char* PML4T_NAME = "Kernel PML4T";

int find_map_region_privileges(const KernelMMapItem* map_region) {
    //Free and reserved segments are considered as RW-
    if(map_region->nature <= NATURE_RES) return 2;
    //Bootstrap segments are considered as R-X
    if(map_region->nature == NATURE_BSK) return 0;
    //Kernel elements are set up according to their specified permission
    if(map_region->nature == NATURE_KNL) {
        if(strcmp((char*) (uint32_t) map_region->name, "Kernel RW- segment")==0) return 2;
        if(strcmp((char*) (uint32_t) map_region->name, "Kernel R-X segment")==0) return 0;
        return 1; //It must be a R kernel segment
    }
    //Modules are always read-only
    if(map_region->nature == NATURE_MOD) return 1;
    //Well, I don't know (control should never reach this point)
    return -1;
}

uint32_t generate_paging(KernelInformation* kinfo) {
    uint64_t pt_location, pd_location, pdpt_location, pml4t_location;
    uint64_t memory_amount, pt_length, pd_length, pdpt_length, pml4t_length;
    uint32_t cr3_value;

    /* We'll do the following :

         Step 1 : Find out how large the paging structures will be and reserve space for them
         Step 2 : Fill in a page table (aligning it on a 2^9 entry boundary with empty entries).
         Step 3 : Make page directories from the page table
         Step 4 : Make page directory pointers table from the page directory
         Step 5 : Make PML4
         Step 6 : Mark used memory as such on the memory map, sort and merge.
         Step 7 : Generate and return CR3 value. */

    memory_amount = kmmap_mem_amount(kinfo);
    //Allocate page table space
    pt_length = align_up((memory_amount/PHYADDR_ALIGN)*PENTRY_SIZE, PT_SIZE*PENTRY_SIZE);
    pt_location = kmmap_alloc_pgalign(kinfo, pt_length, NATURE_KNL, PAGE_TABLE_NAME);
    //Allocate page directory space
    pd_length = align_up(pt_length/PT_SIZE, PD_SIZE*PENTRY_SIZE);
    pd_location = kmmap_alloc_pgalign(kinfo, pd_length, NATURE_KNL, PAGE_DIR_NAME);
    //Allocate PDPT space
    pdpt_length = align_up(pd_length/PD_SIZE, PDPT_SIZE*PENTRY_SIZE);
    pdpt_location = kmmap_alloc_pgalign(kinfo, pdpt_length, NATURE_KNL, PDPT_NAME);
    //Allocate PML4T space
    pml4t_length = PML4T_SIZE*PENTRY_SIZE;
    pml4t_location = kmmap_alloc_pgalign(kinfo, pml4t_length, NATURE_KNL, PML4T_NAME);

    make_identity_page_table(pt_location, kinfo);
    protect_stack(pt_location);

    make_identity_page_directory(pd_location, pt_location, pt_length);

    make_identity_pdpt(pdpt_location, pd_location, pd_length);

    make_identity_pml4t(pml4t_location, pdpt_location, pdpt_length);

    cr3_value = pml4t_location;
    return cr3_value;
}

unsigned int make_identity_page_directory(const unsigned int location,
                                                                                    const unsigned int pt_location,
                                                                                    const unsigned int pt_length) {
    pde* page_directory = (pde*) location;
    pde pde_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pt_location;
    uint64_t current_directory;

    for(current_directory = 0; current_directory<pt_length/(PENTRY_SIZE*PT_SIZE);
        ++current_directory, pde_entry_buffer += PT_SIZE*PENTRY_SIZE)
    {
        page_directory[current_directory] = pde_entry_buffer;
    }

    for(; current_directory%PD_SIZE!=0; ++current_directory) page_directory[current_directory] = 0;

    return PENTRY_SIZE*current_directory;
}

unsigned int make_identity_page_table(const unsigned int location, const KernelInformation* kinfo) {
    unsigned int current_mmap_index = 0;
    const KernelMMapItem* kmmap = (const KernelMMapItem*) (uint32_t) kinfo->kmmap;
    uint64_t current_page, current_region_end = kmmap[0].location + kmmap[0].size;
    uint64_t memory_amount = kmmap_mem_amount(kinfo);

    pte pte_mask = PBIT_PRESENT+PBIT_NOEXECUTE+PBIT_WRITABLE; //"mask" being added to page table entries.
    pte pte_entry_buffer = pte_mask;
    pte* page_table = (pte*) location;

    int mode = 0; // Refers to the currently examined memory region.
                  //   0 = In the middle of a normal mmap region
                  //   1 = Non-mapped region (no memory)
                  //   2 = Region at the frontier between two memory map entries
    int privileges = 2; // 0 = R-X
                        // 1 = R--
                        // 2 = RW-

    //Paging all known memory regions
    for(current_page = 0; current_page*PG_SIZE<memory_amount; ++current_page, pte_entry_buffer += PG_SIZE) {
        //Writing page table entry in memory
        page_table[current_page] = pte_entry_buffer;


        // Checking if next page still is in the same memory map entry.
        if((current_page+2)*PG_SIZE > current_region_end) {
            //No. Check what's happening then.
            if((((current_page+2)*PG_SIZE > kmmap[current_mmap_index+1].location) &&
                    ((current_page+1)*PG_SIZE < kmmap[current_mmap_index].location+kmmap[current_mmap_index].size) &&
                    (current_mmap_index<kinfo->kmmap_size-1))
                || (((current_page+2)*PG_SIZE > kmmap[current_mmap_index+2].location) &&
                    (current_mmap_index<kinfo->kmmap_size-2)))
            {
                //Our page overlaps with two distinct memory map entries.
                //This means that we're in the region where GRUB puts its stuff and requires specific care
                    while((current_page+2)*PG_SIZE >= kmmap[current_mmap_index].location + kmmap[current_mmap_index].size) {
                        ++current_mmap_index;
                    }
                    --current_mmap_index;
                    mode=2;
                    current_region_end = kmmap[current_mmap_index].location + kmmap[current_mmap_index].size;
            } else {
                if((current_page+2)*PG_SIZE < kmmap[current_mmap_index+1].location) {
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
            pte_mask = 0;
            switch(mode) {
                case 0:
                    switch(privileges) {
                        case 2: pte_mask+= PBIT_WRITABLE;
                        case 1: pte_mask+= PBIT_NOEXECUTE;
                        case 0: pte_mask+= PBIT_PRESENT;
                    }
                    break;
                case 1:
                    //Memory region is not present, hence page table entry is empty
                    break;
                case 2:
                    /* In overlapping cases, we encountered a grub segment or a module, because the rest is page aligned...
                       Privilege is hence R-- */
                    pte_mask = PBIT_PRESENT+PBIT_NOEXECUTE;
                    break;
            }
            //Refreshing the page table entry buffer
            pte_entry_buffer = current_page;
            pte_entry_buffer *= PG_SIZE;
            pte_entry_buffer += pte_mask;
        }
    }

    //Padding page table with zeroes till it has proper alignment
    for(; current_page%PT_SIZE!=0; ++current_page) page_table[current_page] = 0;

    return current_page*PENTRY_SIZE;
}

unsigned int make_identity_pdpt(const unsigned int location, const unsigned int pd_location, const unsigned int pd_length) {
    pdpe* pdpt = (pdpe*) location;
    pdpe pdpe_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pd_location;
    uint64_t current_dp;

    for(current_dp = 0; current_dp<pd_length/(PENTRY_SIZE*PD_SIZE); ++current_dp, pdpe_entry_buffer += PD_SIZE*PENTRY_SIZE) {
        pdpt[current_dp] = pdpe_entry_buffer;
    }

    for(; current_dp%PDPT_SIZE!=0; ++current_dp) pdpt[current_dp] = 0;

    return PENTRY_SIZE*current_dp;
}

unsigned int make_identity_pml4t(const unsigned int location, const unsigned int pdpt_location, const unsigned int pdpt_length) {
    pml4e* pml4t = (pml4e*) location;
    pml4e pml4e_entry_buffer = PBIT_PRESENT+PBIT_WRITABLE+pdpt_location;
    uint64_t current_ml4e;

    for(current_ml4e = 0; current_ml4e<pdpt_length/(PENTRY_SIZE*PDPT_SIZE); ++current_ml4e, pml4e_entry_buffer += PDPT_SIZE*PENTRY_SIZE) {
        pml4t[current_ml4e] = pml4e_entry_buffer;
    }

    for(; current_ml4e<PML4T_SIZE; ++current_ml4e) pml4t[current_ml4e] = 0;

    return PENTRY_SIZE*current_ml4e;
}

void protect_stack(const uint32_t pt_location) {
    pte* page_table = (pte*) pt_location;
    extern char begin_stack;
    extern char end_stack;
    uint32_t start_pg = ((uint32_t) (&begin_stack))/PG_SIZE;
    uint32_t end_pg = ((uint32_t) (&end_stack))/PG_SIZE - 1;

    if(page_table[start_pg]&PBIT_PRESENT) page_table[start_pg]-=PBIT_PRESENT;
    if(page_table[end_pg]&PBIT_PRESENT) page_table[end_pg]-=PBIT_PRESENT;
}

void setup_pagetranslation(KernelInformation* kinfo,
                           const uint32_t cr3_value,
                           const uint64_t virt_addr,
                           const uint64_t phys_addr,
                           const uint64_t flags) {
    pml4e* pml4t;
    pdpe* pdpt;
    pde* page_dir;
    pte* page_table;
    unsigned int i;
    uint64_t pml4t_index, pdpt_index, pd_index, pt_index, virt_address = virt_addr;
    uint64_t pdpt_location, pdpt_length, pd_location, pd_length, pt_location, pt_length;

    // What does this function do ?
    //     1. Find related PML4T element
    //     2. If PDPT does not exist, create it
    //     3. Find related PDPT element
    //     4. If PD does not exist, create it
    //     5. Find related PD element
    //     6. If PT does not exist, create it
    //     7. Find related PT element
    //     8. Change it to make it point to the physical address, and add flags.
    //
    // We do not need to re-set paging permissions for the PDPT, PD, and PT we create
    // because they require RW- permission, which is the same permission as free memory

    pml4t = (pml4e*) (cr3_value & 0xfffff000);
    pml4t_index = virt_address/PHYADDR_ALIGN; //Remove non-aligned part
    pml4t_index /= PT_SIZE*PD_SIZE*PDPT_SIZE; //Remove PT, PD, and PDPT index parts

    if(!(pml4t[pml4t_index] & PBIT_PRESENT)) {
        //PDPT does not exist. Create it
        pdpt_length = PDPT_SIZE*PENTRY_SIZE;
        pdpt_location = kmmap_alloc_pgalign(kinfo, pdpt_length, NATURE_KNL, PDPT_NAME);

        pdpt = (pdpe*) (uint32_t) pdpt_location;
        for(i=0; i<PDPT_SIZE; ++i) pdpt[i]=0;

        //Put PDPT address in PML4T
        pml4t[pml4t_index] = PBIT_PRESENT+PBIT_WRITABLE+pdpt_location;
    }

    pdpt = (pdpe*) (uint32_t) (pml4t[pml4t_index] & 0xfffff000);
    //Removing the PML4T part of the virtual address
    pml4t_index *= PDPT_SIZE*PD_SIZE*PT_SIZE; //Add empty PDPT, PD, and PT parts
    pml4t_index *= PHYADDR_ALIGN; //Add empty non-aligned part
    virt_address -= pml4t_index;
    //Getting the PDPT index part
    pdpt_index = virt_address/PHYADDR_ALIGN; //Remove non-aligned part
    pdpt_index /= PT_SIZE*PD_SIZE; //Remove PT and PD index part

    if(!(pdpt[pdpt_index] & PBIT_PRESENT)) {
        //Page directory does not exist. Create it
        pd_length = PD_SIZE*PENTRY_SIZE;
        pd_location = kmmap_alloc_pgalign(kinfo, pd_length, NATURE_KNL, PAGE_DIR_NAME);

        page_dir = (pde*) (uint32_t) pd_location;
        for(i=0; i<PD_SIZE; ++i) page_dir[i]=0;

        //Put PD address in PDPT
        pdpt[pdpt_index] = PBIT_PRESENT+PBIT_WRITABLE+pd_location;
    }

    page_dir = (pde*) (uint32_t) (pdpt[pdpt_index] & 0xfffff000);
    //Removing the PDPT part of the virtual address
    pdpt_index *= PD_SIZE*PT_SIZE; //Add empty PD and PT parts
    pdpt_index *= PHYADDR_ALIGN; //Add empty non-aligned part
    virt_address -= pdpt_index;
    //Getting the PD index part
    pd_index = virt_address/PHYADDR_ALIGN; //Remove non-aligned part
    pd_index /= PT_SIZE; //Remove PT part

    if(!(page_dir[pd_index] & PBIT_PRESENT)) {
        //Page table does not exist. Create it
        pt_length = PT_SIZE*PENTRY_SIZE;
        pt_location = kmmap_alloc_pgalign(kinfo, pt_length, NATURE_KNL, PAGE_TABLE_NAME);

        page_table = (pte*) (uint32_t) pt_location;
        for(i=0; i<PT_SIZE; ++i) page_table[i]=0;

        //Put PT address in PD
        page_dir[pd_index] = PBIT_PRESENT+PBIT_WRITABLE+pt_location;
    }

    page_table = (pte*) (uint32_t) (page_dir[pd_index] & 0xfffff000);
    //Removing the PD part of the virtual address
    pd_index *= PT_SIZE; //Add empty PT part
    pd_index *= PHYADDR_ALIGN; //Remove non-aligned part
    virt_address -= pd_index;
    //Getting the PT index part
    pt_index = virt_address/PHYADDR_ALIGN;

    page_table[pt_index] = PBIT_PRESENT + flags + (phys_addr & 0xfffff000);
}
