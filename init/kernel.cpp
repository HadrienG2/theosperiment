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
  addr_t locations[0x5000];
  dbgout << endl << "Allocating..." << endl;
  for(index=0; index<0x5000; ++index) locations[index] = phymem.alloc_page(3);
  dbgout << "Freeing..." << endl;
  for(index=0; index<0x5000; ++index) phymem.free(locations[index]);
  dbgout << "Allocating chunk...";
  phymem.alloc_chunk(3, 0x5000000);
  phymem.print_highmmap();
  
  return 0;
}
