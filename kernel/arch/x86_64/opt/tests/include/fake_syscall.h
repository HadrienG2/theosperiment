 /* The functions inside of here emulate the overhead of a system call. Usable for
    benchmarking purposes.

      Copyright (C) 2011  Hadrien Grasland

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

#ifndef _FAKE_SYSCALL_H_
#define _FAKE_SYSCALL_H_

void fake_syscall(); //The overhead of a system call is taken as that of two context switches
                     //and two lock allocation/liberation cycles.

void fake_context_switch(); //Emulates the overhead of a context switch as closely as possible

#endif
