 /* A simple way to wait a bit before doing something

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

#include "wait.h"

void dbg_wait(const int factor) {
  int j;
  
  if(factor>0) {
    for(j=1; j<=2000*factor; ++j);
  } else {
    __asm__("infinite_loop: hlt;\
                            jmp infinite_loop;");
  }
}