#include <cpp_support.h>
#include <kernel_information.h>

char* const vmem = (char *) 0xb8000;
char* vmem2 = vmem; //(char *) 0xb8000;
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
  vmem[160]='K';
  vmem[162]='e';
  vmem[164]='r';
  vmem[166]='n';
  vmem[168]='e';
  vmem[170]='l';
  vmem[172]=' ';
}

Truc mon_truc;

extern "C" int kmain(/*kernel_information* kinfo*/) {
  //unsigned int index;
  //Okay, everything is ready
  vmem[174] = 'O';
  vmem[176] = 'K';
  vmem2[178] = '!';
  vmem2[180] = ' ';
  vmem2[182] = mon_truc.toto;
/*  vmem3 = kinfo->command_line;
  unsigned int pos=484;
  for(index=kinfo->kmmap_size; index!=0; index/=10, pos-=2) {
    vmem[pos]='0'+index%10;
  }*/
  
  //Next thing to do : display vmem3 to make sure everything is okay
  
  /* This function doesn't work and should be fixed
  __stack_chk_guard_setup(); */
  
  return 0;
}

