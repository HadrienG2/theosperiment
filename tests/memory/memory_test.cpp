 /* Memory management testing routines

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

#include <align.h>
#include <malloc_test.h>
#include <memory_test.h>
#include <phymem_test.h>
#include <test_display.h>
#include <virmem_test.h>

#include <dbgstream.h>

namespace Tests {
    void test_memory(const KernelInformation& kinfo) {
        dbgout << set_window(screen_win);
        dbgout << endl << "Beginning memory management testing..." << endl;

        reset_title();
        test_title("Initialization");

        reset_sub_title();
        subtest_title("Meta (testing the test itself)");
        if(!meta_phymem()) return;
        if(!meta_virmem()) return;
        if(!meta_mallocator()) return;
        subtest_title("PhyMemManager initialization");
        PhyMemManager* phymem_ptr = init_phymem(kinfo);
        if(!phymem_ptr) return;
        PhyMemManager& phymem = *phymem_ptr;
        subtest_title("VirMemManager initialization");
        VirMemManager* virmem_ptr = init_virmem(phymem);
        if(!virmem_ptr) return;
        VirMemManager& virmem = *virmem_ptr;
        subtest_title("MemAllocator initialization");
        MemAllocator* mallocator_ptr = init_mallocator(phymem, virmem);
        if(!mallocator_ptr) return;
        MemAllocator& mallocator = *mallocator_ptr;

        test_title("PhyMemManager");
        phymem_ptr = test_phymem(phymem);
        if(!phymem_ptr) return;

        test_title("VirMemManager");
        virmem_ptr = test_virmem(virmem);
        if(!virmem_ptr) return;

        test_title("MemAllocator");
        mallocator_ptr = test_mallocator(mallocator);
        if(!mallocator_ptr) return;

        dbgout << "All tests were successfully completed !" << endl;
    }
}