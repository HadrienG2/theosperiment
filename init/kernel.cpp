#include <cpp_support.h>
#include <kernel_information.h>

char* const vmem = (char *) 0xb8000;
char* vmem2 = (char *) 0xb8000;
char* vmem3;
const char notes = 0x0e;

class Truc {
  public:
    Truc();
    char toto;
};

Truc::Truc():
    toto(notes)
{
  vmem[320]='K';
  vmem[322]='e';
  vmem[324]='r';
  vmem[326]='n';
  vmem[328]='e';
  vmem[330]='l';
  vmem[332]=' ';
}

Truc mon_truc;

extern "C" int kmain(kernel_information* kinfo) {
  //Okay, everything is ready
  vmem[334] = 'O';
  vmem[336] = 'K';
  vmem2[338] = '!';
  vmem2[340] = ' ';
  vmem2[342] = mon_truc.toto;
  vmem3 = kinfo->command_line;
  
  //Next thing to do : display vmem3 to make sure everything is okay
  
  /* This function doesn't work and should be fixed
  __stack_chk_guard_setup(); */
  
  return 0;
}

