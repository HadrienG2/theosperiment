/*  Kernel's main function

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

#include <kernel_information.h>
#include <kmem_allocator.h>
#include <physmem.h>
#include <virtmem.h>

#include <dbgstream.h>
#include <memory_test.h>

extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << txtcolor(TXT_LIGHTGREEN) << "Kernel loaded: " << kinfo.cpu_info.core_amount;
    dbgout << " CPU core(s) around."  << txtcolor(TXT_LIGHTGRAY) << endl;

    //Put test suites here, as otherwise they might smash the work of already initialized components
    dbgout << set_window(screen_win);
    Tests::test_memory(kinfo);
    dbgout << bp();

    dbgout << "* Setting up physical memory management..." << endl;
    PhyMemManager phymem(kinfo);
    dbgout << "* Setting up virtual memory management..." << endl;
    VirMemManager virmem(phymem);
    dbgout << "* Setting up memory allocator..." << endl;
    MemAllocator mallocator(phymem, virmem);
    setup_kalloc(mallocator);
    dbgout << "All done !";

    return 0;
}
