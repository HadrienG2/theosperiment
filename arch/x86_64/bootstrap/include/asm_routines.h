/*  Access to various assembly routines

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

#ifndef _ASM_ROUTINES_H_
#define _ASM_ROUTINES_H_

#include <bs_kernel_information.h>
#include <stdint.h>

/* Check for CPUID availability and returns a boolean value */
int cpuid_check();
/* Enables long mode (more precisely the 32-bit subset of it, called compatibility mode) */
int enable_longmode(uint32_t cr3_value);
/* Run the kernel */
int run_kernel(uint32_t kernel_entry, KernelInformation* kinfo);

#endif