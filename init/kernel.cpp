#include <interrupts.h>
#include <kernel_information.h>
#include <kmem_allocator.h>
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
    dbgout << "* Setting up memory allocator..." << endl;
    MemAllocator mallocator(phymem, virmem);
    dbgout << set_window(screen_win);
    addr_t test = mallocator.malloc(0x200, 2);
    addr_t test2 = mallocator.malloc(0x200, 2);
    addr_t test3 = mallocator.malloc(0xd00, 2);
    addr_t test4 = mallocator.malloc(0xd00, 2);
    mallocator.print_maplist();
    mallocator.print_busymap(2);
    mallocator.print_freemap(2);
    dbgout << *((VirMemMap*) 0x2b6000);
    
    return 0;
}
