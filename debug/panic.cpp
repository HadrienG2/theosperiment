 /* Function used to kill the kernel

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

#include <panic.h>
#include <dbgstream.h>

const char* PANIC_TITLE = "Oh my god, it sounds like the kernel just died !";

const char* PANIC_IMPOSSIBLE_KERNEL_FLAGS = "MemAllocator : Allocation of memory to the kernel with\
 anything but RW- flags has been forced";
const char* PANIC_IMPOSSIBLE_SHARING = "MemAllocator : An impossible sharing operation (nonexistent\
 shared object, nonexistent source PID, object has not been allocated as shareable, impossible to\
 create recipient) has been forced";
const char* PANIC_MM_UNINITIALIZED = "MemAllocator : Called memory management functions while \
memory management itself was not initialized yet";
const char* PANIC_OUT_OF_MEMORY = "MemAllocator : Out of memory";

void panic(const char* error_message) {
    dbgout << bkgcolor(BKG_PURPLE);
    dbgout << cls();
    dbgout << txtcolor(TXT_WHITE) << PANIC_TITLE << endl << endl;
    dbgout << txtcolor(TXT_YELLOW) << error_message;
    while(1) __asm__("hlt");
}
