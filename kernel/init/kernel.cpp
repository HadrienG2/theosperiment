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
    dbgout << set_window(screen_win);
    dbgout << txtcolor(TXT_WHITE) << "* Kernel loaded, " << kinfo.cpu_info.core_amount << " CPU core(s) detected" << txtcolor(TXT_DEFAULT) << endl;
    dbgout << "* Probing kernel modules..." << endl;
    for(unsigned int i=0; i<kinfo.kmmap_size; ++i) {
        if(kinfo.kmmap[i].nature == NATURE_MOD) dbgout << kinfo.kmmap[i] << endl;
    }

    dbgout << "* Setting up memory management..." << endl;
    PhyMemManager phymem_manager(kinfo);
    VirMemManager virmem_manager(phymem_manager);
    MemAllocator mem_allocator(phymem_manager, virmem_manager);

    dbgout << "* Setting up process management..." << endl;
    ProcessManager process_manager(mem_allocator);

    dbgout << txtcolor(TXT_WHITE) << "* Kernel ready !" << txtcolor(TXT_DEFAULT) << endl;

    //Now, do something useful with that kernel ! :P
    KString test_file("*** Process properties v1 ***\nTest:\n toto=3< >\"\\\"\\\n \"[ ]{{\n }}");
    ProcessPropertiesParser test_parser;
    dbgout << "* Parsing process property file..." << endl << test_file << endl;
    test_parser.open_and_check(test_file);

    return 0;
}
