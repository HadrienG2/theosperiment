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
  
  PhyMemMap* test1 = phymem.alloc_page(2);
  PhyMemMap* test2 = phymem.alloc_page(2);
  PhyMemMap* test3 = phymem.alloc_page(2);
  virmem.map(test1, VMEM_FLAG_R, 2);
  VirMemMap* vtest = virmem.map(test2, VMEM_FLAG_R + VMEM_FLAG_W, 2);
  virmem.map(test3, VMEM_FLAG_R + VMEM_FLAG_X, 2);
  virmem.free(vtest, 2);
  virmem.map(test2, VMEM_FLAG_R + VMEM_FLAG_W, 2);
  //...Then free test2 and re-map it.
  
  virmem.print_maplist();
  dbgout << endl;
  virmem.print_mmap(2);
  
  return 0;
}
