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

#include <hack_stdint.h>
#include <x86paging.h>
#include <x86paging_parser.h>
#include <x86asm.h>

namespace x86paging {
    void create_pml4t(uint64_t location) {
        uint64_t* pointer = (uint64_t*) location;
        for(int i=0; i<512; ++i) pointer[i] = 0;
    }

    void fill_4kpaging(const uint64_t phy_addr,
                       uint64_t vir_addr,
                       const uint64_t size,
                       uint64_t flags,
                       uint64_t pml4t_location) {
        uint64_t additional_params[3] = {phy_addr, flags, 0};
        paging_parser(vir_addr,
                      size,
                      PML4T_LEVEL,
                      (uint64_t*) pml4t_location,
                      &fill_4kpaging_handler,
                      additional_params);
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

        //If 1GB pages are on, we have reached the lowest level of paging hierarchy, return the PDP.
        if(pdpt[pdpt_index] & PBIT_LARGEPAGE) return (uint64_t) pdpt[pdpt_index];
        //Else check that PDPT entry exists, if it does go to the next level
        if(!(pdpt[pdpt_index] & PBIT_PRESENT)) return 0;
        pd = (pde*) (pdpt[pdpt_index] & 0x000ffffffffff000);

        //If 2MB pages are on, we have reached the lowest level of paging hierarchy, return the PDE.
        if(pd[pd_index] & PBIT_LARGEPAGE) return (uint64_t) pd[pd_index];
        //Else check that PD entry exists, if it does go to the next level
        if(!(pd[pd_index] & PBIT_PRESENT)) return 0;
        pt = (pte*) (pd[pd_index] & 0x000ffffffffff000);

        //Return the PTE
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

        //If 1GB pages are on, we have reached the lowest level of paging hierarchy
        if(pdpt[pdpt_index] & PBIT_LARGEPAGE) {
            return (uint64_t) ((pdpt[pdpt_index] & 0x000fffffc0000000) + (vaddr & 0x3fffffff));
        }
        //Else check that PDPT entry exists, if it does go to the next level
        if(!(pdpt[pdpt_index] & PBIT_PRESENT)) return 0;
        pd = (pde*) (pdpt[pdpt_index] & 0x000ffffffffff000);

        //If 2MB pages are on, we have reached the lowest level of paging hierarchy
        if(pd[pd_index] & PBIT_LARGEPAGE) {
            return (uint64_t) ((pd[pd_index] & 0x000fffffffe00000) + (vaddr & 0x1fffff));
        }
        //Else check that PD entry exists, if it does go to the next level
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

    uint64_t remove_paging(uint64_t vir_addr,
                           const uint64_t size,
                           uint64_t pml4t_location,
                           PhyMemManager* phymem) {
        uint64_t additional_params[1] = {(uint64_t) phymem};
        return paging_parser(vir_addr,
                            size,
                            PML4T_LEVEL,
                            (uint64_t*) pml4t_location,
                            &remove_paging_handler,
                            additional_params);
    }

    uint64_t setup_4kpages(uint64_t vir_addr,
                           const uint64_t size,
                           uint64_t pml4t_location,
                           PhyMemManager* phymem) {
        uint64_t additional_params[1] = {(uint64_t) phymem};
        return paging_parser(vir_addr,
                            size,
                            PML4T_LEVEL,
                            (uint64_t*) pml4t_location,
                            &setup_4kpages_handler,
                            additional_params);
    }

    void set_flags(uint64_t vaddr, const uint64_t size, uint64_t flags, uint64_t pml4t_location) {
        uint64_t additional_params[1] = {flags};
        paging_parser(vaddr,
                      size,
                      PML4T_LEVEL,
                      (uint64_t*) pml4t_location,
                      &set_flags_handler,
                      additional_params);
    }
}