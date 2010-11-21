#include <interrupts.h>
#include <kernel_information.h>
#include <physmem.h>
#include <virtmem.h>

#include <dbgstream.h>


extern "C" int kmain(const KernelInformation& kinfo) {
  dbgout << txtcolor(TXT_LIGHTRED) << "Kernel loaded !";
  dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
  dbgout << kinfo.cpu_info.core_amount << " CPU core(s) around." << endl << set_window(screen_win);
  dbgout << "* Setting up physical memory management..." << endl;
  PhyMemManager phymem(kinfo);
  dbgout << "* Setting up virtual memory management..." << endl;
  VirMemManager virmem(phymem);
  
  virmem.print_maplist();
  dbgout << endl;
  virmem.print_mmap(2);
  
  return 0;
}
