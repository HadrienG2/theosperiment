#include <cpp_support.h>

char* const vmem = (char *) 0xb8000;
char* vmem2 = (char *) 0xb8000;
int vmem3;
const char notes = 0x0e;

class Truc {
  public:
    Truc();
    char toto;
};

Truc::Truc():
    toto(notes)
{
  vmem[0]='O';
}

Truc mon_truc;

extern "C" int kmain() {
  //Okay, everything is ready
  vmem[2] = 'K';
  vmem2[4] = '!';
  vmem2[6] = mon_truc.toto;
  vmem3 = mon_truc.toto;
  
  return 0;
}

