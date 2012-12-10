 /* A way to make the kernel "die" when there's nothing else to do

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

#ifndef _DIE_H_
#define _DIE_H_

//Error messages displayed when dying
extern const char* INVALID_KERNEL; //Kernel file is corrupt
extern const char* KERNEL_NOT_FOUND;
extern const char* MMAP_TOO_SMALL;
extern const char* MULTIBOOT_MISSING;
extern const char* NO_MEMORYMAP;
extern const char* INADEQUATE_CPU;

/* Display a purple screen of death with the provided invoked cause */
void die(const char* issue);

#endif
