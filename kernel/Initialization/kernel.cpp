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

#include <KernelInformation.h>
#include <KUtf32String.h>
#include <MemAllocator.h>
#include <RamManager.h>
#include <PagingManager.h>
#include <ProcessManager.h>

#include <dbgstream.h>


extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << txtcolor(TXT_WHITE) << "* Kernel loaded, " << kinfo.cpu_info.core_amount << " CPU core(s) detected" << txtcolor(TXT_DEFAULT) << endl;

    //Initialize memory management
    dbgout << txtcolor(TXT_WHITE) << "* Setting up memory management components: ram ";
    RamManager ram_manager(kinfo);
    dbgout << move_rel(-4,0) << "RAM page ";
    PagingManager paging_manager(ram_manager);
    dbgout << move_rel(-5,0) << "PAGE malloc ";
    MemAllocator mem_allocator(ram_manager, paging_manager);
    dbgout << move_rel(-7,0) << "MALLOC" << txtcolor(TXT_DEFAULT) << endl;
    
    //Initialize kernel module managment
    dbgout << txtcolor(TXT_WHITE) << "* Initializing kernel module management..." << txtcolor(TXT_LIGHTRED) << " /!\\ TO BE DONE /!\\" << txtcolor(TXT_DEFAULT) << endl;
    dbgout << bp();
    
    //Test KUtf32String
    dbgout << txtcolor(TXT_WHITE) << "* Playing with Unicode strings..." << txtcolor(TXT_DEFAULT) << endl;
    KUtf32String test("This is a test");
    dbgout << bp();
    
    //Start process manager
    dbgout << txtcolor(TXT_WHITE) << "* Setting up process management..." << txtcolor(TXT_YELLOW) << " /!\\ WORK IN PROGRESS /!\\" << txtcolor(TXT_DEFAULT) << endl;
    ProcessManager process_manager(mem_allocator);
    dbgout << bp();

    //Now, do something useful with that kernel ! :P
    dbgout << txtcolor(TXT_WHITE) << "* Playing with process properties..."  << txtcolor(TXT_DEFAULT) << endl;
    KAsciiString test_file("*** Process properties v1 ***\nTest:\n toto=3< >\"\\\"\\\n \"[ ]{{\n }}");
    ProcessPropertiesParser test_parser;
    dbgout << "* Parsing process property file..." << endl << test_file << endl;
    test_parser.open_and_check(test_file);
    dbgout << bp();
    
    //Initialize remaining kernel components
    dbgout << txtcolor(TXT_WHITE) << "* Initializing remaining kernel components..." << txtcolor(TXT_LIGHTRED) << " /!\\ TO BE DONE /!\\" << txtcolor(TXT_DEFAULT) << endl;
    
    //Once everything is done, get rid of the now-useless bootstrap component, and associated data
    dbgout << txtcolor(TXT_WHITE) << "* Freeing up bootstrap data structures..." << txtcolor(TXT_LIGHTRED) << " /!\\ TO BE DONE /!\\" << txtcolor(TXT_DEFAULT) << endl;
    
    dbgout << txtcolor(TXT_WHITE) << "* Ready to roll out !" << txtcolor(TXT_DEFAULT);

    return 0;
}
