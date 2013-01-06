/*  Kernel's main function

      Copyright (C) 2010-2013  Hadrien Grasland

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
#include <ram_manager.h>
#include <process_manager.h>
#include <paging_manager.h>

#include <dbgstream.h>


extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << txtcolor(TXT_WHITE) << "* Kernel loaded, " << kinfo.cpu_info.core_amount << " CPU core(s) detected" << txtcolor(TXT_DEFAULT) << endl;

    dbgout << "* Setting up memory management components: ram page malloc..." << move_rel(-18,0);
    RAMManager ram_manager(kinfo);
    dbgout << "RAM ";
    PagingManager paging_manager(ram_manager);
    dbgout << "PAGE ";
    MemAllocator mem_allocator(ram_manager, paging_manager);
    dbgout << "MALLOC" << endl;

    dbgout << "* Setting up process management..." << endl;
    ProcessManager process_manager(mem_allocator);
    
    //Silly birthday text :P
    dbgout << txtcolor(TXT_WHITE) << "* " << txtcolor(TXT_LIGHTRED) << "Ha" << txtcolor(TXT_YELLOW) << "pp" << txtcolor(TXT_LIGHTGREEN) << "y ";
    dbgout << txtcolor(TXT_LIGHTCOBALT) << "bi" << txtcolor(TXT_LIGHTBLUE) << "rt" << txtcolor(TXT_LIGHTRED) << "hd" << txtcolor(TXT_YELLOW) << "ay";
    dbgout << txtcolor(TXT_LIGHTGREEN) << ", " << txtcolor(TXT_LIGHTCOBALT) << "OS" << txtcolor(TXT_LIGHTBLUE) << "|p" << txtcolor(TXT_LIGHTPURPLE) <<"er";
    dbgout << txtcolor(TXT_LIGHTRED) << "im" << txtcolor (TXT_YELLOW) << "en" << txtcolor(TXT_LIGHTGREEN) << "t " << txtcolor(TXT_LIGHTBLUE) << "!";
    dbgout << txtcolor(TXT_DEFAULT) << bp() << endl;
    
    //dbgout << txtcolor(TXT_WHITE) << "* Kernel ready !" << txtcolor(TXT_DEFAULT) << bp() << endl;

    //Now, do something useful with that kernel ! :P
    KString test_file("*** Process properties v1 ***\nTest:\n toto=3< >\"\\\"\\\n \"[ ]{{\n }}");
    ProcessPropertiesParser test_parser;
    dbgout << "* Parsing process property file..." << endl << test_file << endl;
    test_parser.open_and_check(test_file);

    return 0;
}
