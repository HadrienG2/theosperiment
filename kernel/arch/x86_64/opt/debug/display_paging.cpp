 /* Debug functions used to display x86 paging structures on screen

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
#include <display_paging.h>
#include <x86paging.h>
#include <dbgstream.h>

namespace x86paging {
    void dbg_print_pd(const uint64_t location) {
        uint64_t mask; //Mask used to separate physical addresses from control bits
        pde* pd = (pde*) location;  
        pde current_el;
        uint64_t address;
        
        dbgout << pad_status(true);
        dbgout << "#   | Address            | Flags" << endl;
        dbgout << "-------------------------------------------------------------------------------";
        dbgout << endl;
        for(int table_index=0; table_index<PTABLE_LENGTH; ++table_index) {
            //For each nonzero table element, print address and control bits
            current_el = pd[table_index];
            if(current_el == 0) continue;
            dbgout << numberbase(DECIMAL);
            dbgout << pad_size(3);
            dbgout << table_index << " | ";
            if(current_el & PBIT_LARGEPAGE) {
                mask = (1<<21)-1+PBIT_NOEXECUTE;
            } else {
                mask = (1<<12)-1+PBIT_NOEXECUTE;
            }
            address = current_el - (current_el & mask);
            dbgout << numberbase(HEXADECIMAL);
            dbgout << pad_size(18);
            dbgout << address << " | ";
            if(current_el & PBIT_PRESENT) dbgout << "PRST ";
            if(current_el & PBIT_WRITABLE) dbgout << "WRIT ";
            if(current_el & PBIT_USERACCESS) dbgout << "USER ";
            if(current_el & PBIT_NOCACHE) dbgout << "!CAC ";
            if(current_el & PBIT_ACCESSED) dbgout << "ACSD ";
            if(current_el & PBIT_LARGEPAGE) {
                dbgout << "LARG ";
                if(current_el & PBIT_DIRTY) dbgout << "DRTY ";
                if(current_el & PBIT_GLOBALPAGE) dbgout << "GLOB ";
            }
            if(current_el & PBIT_NOEXECUTE) dbgout << "NOEX";
            dbgout << endl;
        }
    }

    void dbg_print_pdpt(const uint64_t location) {
        uint64_t mask; //Mask used to separate physical addresses from control bits
        pdp* pdpt = (pdp*) location;  
        pdp current_el;
        uint64_t address;
      
        dbgout << pad_status(true);
        dbgout << "#   | Address            | Flags" << endl;
        dbgout << "-------------------------------------------------------------------------------";
        dbgout << endl;
        for(int table_index=0; table_index<PTABLE_LENGTH; ++table_index) {
            //For each nonzero table element, print address and control bits
            current_el = pdpt[table_index];
            if(current_el == 0) continue;
            dbgout << numberbase(DECIMAL);
            dbgout << pad_size(3);
            dbgout << table_index << " | ";
            if(current_el & PBIT_LARGEPAGE) {
                mask = (1<<30)-1+PBIT_NOEXECUTE;
            } else {
                mask = (1<<12)-1+PBIT_NOEXECUTE;
            }
            address = current_el - (current_el & mask);
            dbgout << numberbase(HEXADECIMAL);
            dbgout << pad_size(18);
            dbgout << address << " | ";
            if(current_el & PBIT_PRESENT) dbgout << "PRST ";
            if(current_el & PBIT_WRITABLE) dbgout << "WRIT ";
            if(current_el & PBIT_USERACCESS) dbgout << "USER ";
            if(current_el & PBIT_NOCACHE) dbgout << "!CAC ";
            if(current_el & PBIT_ACCESSED) dbgout << "ACSD ";
            if(current_el & PBIT_LARGEPAGE) {
                dbgout << "LARG ";
                if(current_el & PBIT_DIRTY) dbgout << "DRTY ";
                if(current_el & PBIT_GLOBALPAGE) dbgout << "GLOB ";
            }
            if(current_el & PBIT_NOEXECUTE) dbgout << "NOEX";
            dbgout << endl;
        }
    }

    void dbg_print_pml4t(const uint64_t cr3_value) {
        const uint64_t mask = (1<<12)-1+PBIT_NOEXECUTE; //Mask used to eliminate the control bits of CR3
        uint64_t pml4t_location = cr3_value - (cr3_value & mask);
        pml4e* pml4t = (pml4e*) pml4t_location;  
        pml4e current_el;
        uint64_t address;
        
        dbgout << pad_status(true);
        dbgout << "#   | Address            | Flags" << endl;
        dbgout << "-------------------------------------------------------------------------------";
        dbgout << endl;
        for(int table_index=0; table_index<PTABLE_LENGTH; ++table_index) {
            //For each nonzero table element, print address and control bits
            current_el = pml4t[table_index];
            if(current_el == 0) continue;
            dbgout << numberbase(DECIMAL);
            dbgout << pad_size(3);
            dbgout << table_index << " | ";
            address = current_el - (current_el & mask);
            dbgout << numberbase(HEXADECIMAL);
            dbgout << pad_size(18);
            dbgout << address << " | ";
            if(current_el & PBIT_PRESENT) dbgout << "PRST ";
            if(current_el & PBIT_WRITABLE) dbgout << "WRIT ";
            if(current_el & PBIT_USERACCESS) dbgout << "USER ";
            if(current_el & PBIT_NOCACHE) dbgout << "!CAC ";
            if(current_el & PBIT_ACCESSED) dbgout << "ACSD ";
            if(current_el & PBIT_NOEXECUTE) dbgout << "NOEX";
            dbgout << endl;
        }
    }

    void dbg_print_pt(const uint64_t location) {
        const uint64_t mask = (1<<12)-1+PBIT_NOEXECUTE; //Mask used to separate control bits from adresses
        pte* pt = (pte*) location;  
        pte current_el;
        uint64_t address;
        
        dbgout << pad_status(true);
        dbgout << "#   | Address            | Flags" << endl;
        dbgout << "-------------------------------------------------------------------------------";
        dbgout << endl;
        for(int table_index=0; table_index<PTABLE_LENGTH; ++table_index) {
            //For each nonzero table element, print address and control bits
            current_el = pt[table_index];
            if(current_el == 0) continue;
            dbgout << numberbase(DECIMAL);
            dbgout << pad_size(3);
            dbgout << table_index << " | ";
            address = current_el - (current_el & mask);
            dbgout << numberbase(HEXADECIMAL);
            dbgout << pad_size(18);
            dbgout << address << " | ";
            if(current_el & PBIT_PRESENT) dbgout << "PRST ";
            if(current_el & PBIT_WRITABLE) dbgout << "WRIT ";
            if(current_el & PBIT_USERACCESS) dbgout << "USER ";
            if(current_el & PBIT_NOCACHE) dbgout << "!CAC ";
            if(current_el & PBIT_ACCESSED) dbgout << "ACSD ";
            if(current_el & PBIT_DIRTY) dbgout << "DRTY ";
            if(current_el & PBIT_LARGEPAGE) dbgout << "LARG ";
            if(current_el & PBIT_GLOBALPAGE) dbgout << "GLOB ";
            if(current_el & PBIT_NOEXECUTE) dbgout << "NOEX";
            dbgout << endl;
        }
    }
}
