 /* Some macros that align things on a page (or other) basis.

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

#ifndef _ALIGN_H_
#define _ALIGN_H_

#include <address.h>

#define align_up(quantity, align) (((quantity)%(align)!=0) \
                                   ?(((quantity)/(align) + 1) * (align))\
                                   :(quantity))
#define align_down(quantity, align) (((quantity)/(align)) * (align))

#define PG_SIZE 0x1000 //Page size is 4KB
#define align_pgup(quantity) align_up(quantity, PG_SIZE)
#define align_pgdown(quantity) align_down(quantity, PG_SIZE)

#endif