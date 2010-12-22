 /* Memory management testing routines

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
#include <memory_test.h>

#include <dbgstream.h>

namespace MemTest {
    void test_memory(const KernelInformation& kinfo) {
        dbgout << endl << "Beginning tests..." << endl;
        dbgout << "I/ PhyMemManager" << endl;
        PhyMemManager* phymem = test_phymem(kinfo);
        if(!phymem) return;
        dbgout << "II/ VirMemManager" << endl;
        VirMemManager* virmem = test_virmem(*phymem);
        if(!virmem) return;
        dbgout << "III/ MemAllocator" << endl;
        MemAllocator* mallocator = test_mallocator(*phymem, *virmem);
        if(!mallocator) return;
        dbgout << "All tests were successfully completed !" << endl;
    }
    
    PhyMemManager* test_phymem(const KernelInformation& kinfo) {
        dbgout << "  1. Meta (testing the test itself)" << endl;
        if(!phy_test1_meta()) return NULL;
        dbgout << "  2. Initial state" << endl;
        PhyMemManager* phymem = phy_test2_init(kinfo);
        if(!phymem) return NULL;
        return phymem;
    }
    
    bool phy_test1_meta() {
        dbgout << "    * Check PhyMemManager version" << endl;
        if(PHYMEM_TEST_VERSION != PHYMEMMANAGER_VERSION) {
            dbgout << txtcolor(TXT_RED) << "  Error : Test version (" << PHYMEM_TEST_VERSION;
            dbgout << ") and PhyMemManager version (" << PHYMEMMANAGER_VERSION;
            dbgout << ") mismatch." << txtcolor(TXT_LIGHTGRAY) << endl;
            return false;
        }
        return true;
    }
    
    PhyMemManager* phy_test2_init(const KernelInformation& kinfo) {
        dbgout << "    * Initialize PhyMemManager" << endl;
        static PhyMemManager phymem(kinfo);
        
        dbgout << "    * Check availability of mmap_mutex" << endl;
        PhyMemState* phymem_state = (PhyMemState*) &phymem;
        if(!(phymem_state->mmap_mutex.state())) {
            dbgout << txtcolor(TXT_RED) << "  Error : mmap_mutex is not available in a ";
            dbgout << "freshly initialized PhyMemManager" << txtcolor(TXT_LIGHTGRAY) << endl;
            return NULL;
        }
        
        dbgout << "    * Check that the assumptions about phy_mmap are respected" << endl;
        PhyMemMap* map_parser = phymem_state->phy_mmap;
        while(map_parser) {
            if(map_parser->location%PG_SIZE) {
                dbgout << txtcolor(TXT_RED) << "  Error : Map items aren't always on a ";
                dbgout << "page-aligned location" << txtcolor(TXT_LIGHTGRAY) << endl;
                return NULL;
            }
            if(map_parser->size%PG_SIZE) {
                dbgout << txtcolor(TXT_RED) << "  Error : Map items don't always have a ";
                dbgout << "page-aligned size" << txtcolor(TXT_LIGHTGRAY) << endl;
                return NULL;
            }
            if(map_parser->next_mapitem) {
                if(map_parser->location > map_parser->next_mapitem->location) {
                    dbgout << txtcolor(TXT_RED) << "  Error : Map items are not sorted";
                    dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
                }
                if(map_parser->location+map_parser->size > map_parser->next_mapitem->location) {
                    dbgout << txtcolor(TXT_RED) << "  Error : Map items overlap";
                    dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
                }
            }
            map_parser = map_parser->next_mapitem;
        }
        
        dbgout << "    * Check integration of kmmap in phy_mmap" << endl;
        //TODO
        
        return &phymem;
    }
    
    VirMemManager* test_virmem(PhyMemManager& phymem) {
        dbgout << txtcolor(TXT_RED) << "  Not implemented yet. Aborting...";
        dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
        return NULL;
    }
    
    MemAllocator* test_mallocator(PhyMemManager& phymem, VirMemManager& virmem) {
        dbgout << txtcolor(TXT_RED) << "  Not implemented yet. Aborting...";
        dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
        return NULL;
    }
}