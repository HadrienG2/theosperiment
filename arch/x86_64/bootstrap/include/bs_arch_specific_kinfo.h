 /* Structures that are passed to the main kernel by the bootstrap kernel -- arch-specific data

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

#ifndef _BS_ARCH_INFO_H_
#define _BS_ARCH_INFO_H_

#include <stdint.h>

typedef struct arch_specific_info arch_specific_info;
typedef struct startup_drive_info startup_drive_info;

struct startup_drive_info {
  /* Notice : This is maybe x86-specific. It may be removed
     or improved when we start to support other architectures */
  uint8_t drive_number;
  uint8_t partition_number;
  uint8_t sub_partition_number;
  uint8_t subsub_partition_number;
};

struct arch_specific_info {
  uint64_t startup_drive; /* 64-bit pointer to a startup_drive_info structure */
};

#endif 
