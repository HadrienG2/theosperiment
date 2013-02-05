 /* A very simple header with one goal : create an size_t type that is an unsigned integer of the
    same size as the standard pointer type, and its NULL value.

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

#ifndef _ADDRESS_H_
#define _ADDRESS_H_

#include <stdint.h>

#define NULL 0
#define MAX_RAM_ADDRESS 0xffffffffffffffff

typedef uint64_t size_t; //On x86_64, addresses are unsigned 64-bit integers

#endif
