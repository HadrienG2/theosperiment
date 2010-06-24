/*  Code used to enable long mode

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

#ifndef _ENABLE_LONGMODE_H_
#define _ENABLE_LONGMODE_H_

#include <bs_kernel_information.h>
#include <paging.h>

/* Returns 0 if success or -1 if 64-bit mode is not available */
uint32_t enable_compatibility(uint32_t cr3_value);
/* Load 64-bit GDT and run the kernel */
uint32_t enable_longmode(uint64_t gdtr, uint32_t kernel_entry, uint32_t kernel_information);

#endif