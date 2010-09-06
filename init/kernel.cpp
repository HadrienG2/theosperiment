#include <interrupts.h>
#include <kernel_information.h>
#include <physmem.h>

#include <dbgstream.h>


extern "C" int kmain(KernelInformation& kinfo) {
  dbgout << txtcolor(TXT_LIGHTRED) << "Kernel loaded !";
  dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
  dbgout << "CPU name : " << kinfo.cpu_info.arch_info.processor_name << endl;
  dbgout << "We have " << kinfo.cpu_info.core_amount << " CPU core(s)" << endl;
  dbgout << "* Setting up physical memory management...";
  PhyMemManager phymem(kinfo);
  dbgout << " DONE !" << endl << set_window(screen_win);
  int index;
  addr_t locations[3];
  for(index=0; index<3; ++index) locations[index] = phymem.alloc_lowpage(3);
  for(index=2; index>=0; --index) phymem.free(locations[index]);
  phymem.alloc_lowchunk(3, 0x3000);
  phymem.print_lowmmap();
  
  return 0;
}
