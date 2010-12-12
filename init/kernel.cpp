#include <interrupts.h>
#include <kernel_information.h>
#include <physmem.h>
#include <virtmem.h>

#include <dbgstream.h>
#include <display_paging.h>


extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << txtcolor(TXT_LIGHTRED) << "Kernel loaded !";
    dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
    dbgout << kinfo.cpu_info.core_amount << " CPU core(s) around." << endl;
    dbgout << "* Setting up physical memory management..." << endl;
    PhyMemManager phymem(kinfo);
    dbgout << "* Setting up virtual memory management..." << endl;
    VirMemManager virmem(phymem);
    dbgout << set_window(screen_win);
    phymem.print_mmap();
    
    return 0;
}
