 /* Functions reading and displaying the multiboot information structure

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

#ifndef _MBDATA_DISP_H_
#define _MBDATA_DISP_H_
 
#include <multiboot.h>
 
//Print the map of drives provided by GRUB
void dbg_print_drvmap(const multiboot_info_t* mbd);
//Describes multiboot flags info, with one line of text per flag.
void dbg_print_mbflags(const multiboot_info_t* mbd);
//Prints the map of memory provided by GRUB
void dbg_print_mmap(const multiboot_info_t* mbd);

#endif
