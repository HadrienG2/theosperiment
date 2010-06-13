 /* Display paging structures on screen for debugging purposes

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

#ifndef _DISPLAY_PAGING_H_
#define _DISPLAY_PAGING_H_

#include <hack_stdint.h>

void dbg_print_pd(const uint32_t location);
void dbg_print_pdpt(const uint32_t location);
void dbg_print_pml4t(const uint32_t cr3_value);
void dbg_print_pt(const uint32_t location);

#endif