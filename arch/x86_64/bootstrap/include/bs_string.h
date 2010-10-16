/*  Bootstrap kernel's string manipulation routines

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

#ifndef _BS_STRING_H_
#define _BS_STRING_H_

#include <stdint.h>

#define NULL 0
/* Those are rather primitive implementations, but they are sufficient for us because we
  use "clean" strings of known length and which are always zero-terminated */
int strlen(const char* str);
int strcmp(const char* str1, const char* str2);
void* memcpy(void* destination, const void* source, const unsigned int num);
void* memset(void* ptr, int value, const unsigned int num);

#endif
