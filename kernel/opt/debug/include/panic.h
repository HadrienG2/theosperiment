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

#ifndef _PANIC_H_
#define _PANIC_H_

extern const char* PANIC_NONEXISTENT_PID; //Someone attempted to force something upon a nonexistent PID
extern const char* PANIC_IMPOSSIBLE_SHARING; //Someone attempted to force an impossible sharing
                                             //operation
extern const char* PANIC_MAXIMAL_SHARING_REACHED; //Someone has attempted to share a single page of
                                                  //memory with a single process more than 2^32
                                                  //times.
extern const char* PANIC_MM_UNINITIALIZED; //Someone called kalloc() and such with the force
                                           //switch on while they were not initialized yet
extern const char* PANIC_OUT_OF_MEMORY; //MemAllocator runs out of memory

void panic(const char* error_message);

#endif
