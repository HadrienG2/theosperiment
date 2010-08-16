#include <interrupts.h>
#include <kernel_information.h>
#include <physmem.h>

#include <dbgstream.h>


extern "C" int kmain(KernelInformation& kinfo) {
  dbgout << "* Setting up physical memory management..." << endl;
  PhyMemManager phymem(kinfo);
  dbgout << (uint32_t) sizeof(MemMap) << endl << (uint32_t) sizeof(VirMemMap);
  
  return 0;
}

