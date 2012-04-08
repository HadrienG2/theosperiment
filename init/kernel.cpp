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
#include <mem_allocator.h>
#include <phymem_manager.h>
#include <process_manager.h>
#include <virmem_manager.h>

#include <dbgstream.h>

extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << "* Kernel loaded, " << kinfo.cpu_info.core_amount << " CPU core(s) detected" << endl;

    //Some test suites should be run here, before kernel starts.

    dbgout << "* Setting up memory management..." << endl;
    PhyMemManager phymem_manager(kinfo);
    VirMemManager virmem_manager(phymem_manager);
    MemAllocator mem_allocator(phymem_manager, virmem_manager);

    dbgout << "* Setting up process management..." << endl;
    ProcessManager process_manager(mem_allocator);

    dbgout << "* Kernel ready !" << endl;

    //Now, do something useful with that kernel ! :P

    return 0;
}
