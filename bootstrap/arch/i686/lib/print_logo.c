 /* Code used to display the TOSP logo on boot

      Copyright (C) 2013  Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#include <print_logo.h>
#include "txt_videomem.h"

void print_logo() {
    //A TOSP logo, made in ASCII art. It is 78x13 characters large
    //
    //o   o
    print_chr(-36);  movecur_rel(+3,0);  print_chr(-36); movecur_rel(-5,+1);
    //O   O
    print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(-5,+1);
    //O°° O°°o o°°o o°°°°°°o o°°°°°°° O                                          0
    print_chr(-37);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+42,0);  print_chr(-37);  movecur_rel(-76,+1);
    //O   O  O OooO O      O O        O                                          0
    print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+2,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-36);  print_chr(-36);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+8,0);  print_chr(-37);  movecur_rel(+42,0);  print_chr(-37);  movecur_rel(-76,+1);
    //O   O  O O    O      O O        O                  o                       0
    print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+2,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+4,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+8,0);  print_chr(-37);  movecur_rel(+18,0); print_chr(-33); movecur_rel(+23,0);  print_chr(-37);  movecur_rel(-76,+1);
    // °° °  °  °°° O      O  °°°°°°o O 0°°°o o°°°o o°°° O O°°°O°°°o o°°°o O°°°o 0°°°
    movecur_rel(+1,0);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+2,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-36);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-36);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  movecur_rel(-78,+1);
    //              0      0        0 0 0   0 0   0 0    0 0   0   0 0   0 0   0 0
    movecur_rel(+14,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+8,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+4,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(-76,+1);
    //              0      0        0 0 0   0 0°°°° 0    0 0   0   0 0°°°° 0   0 0
    movecur_rel(+14,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+8,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+4,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(-76,+1);
    //              0      0        0 0 0   0 0     0    0 0   0   0 0     0   0 0
    movecur_rel(+14,0);  print_chr(-37);  movecur_rel(+6,0);  print_chr(-37);  movecur_rel(+8,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+5,0);  print_chr(-37);  movecur_rel(+4,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(+5,0);  print_chr(-37);  movecur_rel(+3,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(-76,+1);
    //               °°°°°°  °°°°°°°  0 O°°°   °°°° °    ° °   °   °  °°°° °   °  °°
    movecur_rel(+15,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+3,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-33);  movecur_rel(+4,0);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-33);  movecur_rel(+3,0);  print_chr(-33);  movecur_rel(+3,0);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-33);  print_chr(-33);  print_chr(-33);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-33);  movecur_rel(+3,0);  print_chr(-33);  movecur_rel(+2,0);  print_chr(-33);  print_chr(-33);  movecur_rel(-78,+1);
    //                                0 O
    movecur_rel(+32,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(-35,+1);
    //                                0 O
    movecur_rel(+32,0);  print_chr(-37);  movecur_rel(+1,0);  print_chr(-37);  movecur_rel(-35,+1);
    //                                ° °
    movecur_rel(+32,0);  print_chr(-33);  movecur_rel(+1,0);  print_chr(-33);  print_chr('\n');
}
