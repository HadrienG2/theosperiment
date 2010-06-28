 /* Routines necessary in order to use some C++ features

      Copyright (C) 2010  Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cpp_support.h>
#include <panic.h>

/* This code allows use of pure virtual functions */
extern "C" void __cxa_pure_virtual() {
  //This function should not ever be called. Hang the kernel.
  panic();
}

int _purecall() {
  //Same as above
  panic();
  return 0;
}

/* This code allows use of stack smashing protection */
void * __stack_chk_guard = 0;

extern "C" void __stack_chk_guard_setup() {
    unsigned char * p;
    p = (unsigned char *) &__stack_chk_guard;
    p[sizeof(__stack_chk_guard)-1] = 255;  /* <- this should be probably randomized */
    p[sizeof(__stack_chk_guard)-2] = '\n';
    p[0] = 0;
}
 
extern "C" void __stack_chk_fail() {
  //Stack has been smashed. Hang the kernel
  panic();
}