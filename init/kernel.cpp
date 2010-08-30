#include <interrupts.h>
#include <kernel_information.h>
#include <physmem.h>

#include <dbgstream.h>


extern "C" int kmain(KernelInformation& kinfo) {
  dbgout << txtcolor(TXT_LIGHTRED) << "Kernel loaded !";
  dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
  dbgout << "CPU name : " << kinfo.cpu_info.arch_info.processor_name << endl;
  dbgout << "We have " << kinfo.cpu_info.core_amount << " CPU core(s)" << endl;
  dbgout << txtcolor(TXT_LIGHTBLUE) << "* Setting up physical memory management...";
  PhyMemManager phymem(kinfo);
  dbgout << txtcolor(TXT_LIGHTGREEN) << " DONE !" << endl;
  
  return 0;
}

