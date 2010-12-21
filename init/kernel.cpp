#include <interrupts.h>
#include <kernel_information.h>
#include <kmem_allocator.h>
#include <physmem.h>
#include <virtmem.h>

#include <dbgstream.h>
#include <display_paging.h>


extern "C" int kmain(const KernelInformation& kinfo) {
    dbgout << txtcolor(TXT_LIGHTRED) << "Kernel loaded : " << kinfo.cpu_info.core_amount;
    dbgout << " CPU core(s) around."  << txtcolor(TXT_LIGHTGRAY) << endl;
    dbgout << "* Setting up physical memory management..." << endl;
    PhyMemManager phymem(kinfo);
    dbgout << "* Setting up virtual memory management..." << endl;
    VirMemManager virmem(phymem);
    dbgout << "* Setting up memory allocator..." << endl;
    MemAllocator mallocator(phymem, virmem);
    setup_kalloc(mallocator);
    dbgout << "Kernel initialized !";
    
    return 0;
}
