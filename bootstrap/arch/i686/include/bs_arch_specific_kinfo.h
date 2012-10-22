 /* Structures that are passed to the main kernel by the bootstrap kernel -- arch-specific data

    Copyright (C) 2010    Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#ifndef _BS_ARCH_INFO_H_
#define _BS_ARCH_INFO_H_

#include <address.h>
#include <stdint.h>

//WARNING : ANY CHANGE MADE TO THIS FILE SHOULD BE MIRRORED TO ARCH_SPECIFIC_KINFO.H.
//OTHERWISE, INCONSISTENT BEHAVIOR WILL OCCUR.

typedef struct ArchSpecificKInfo ArchSpecificKInfo;
typedef struct StartupDriveInfo StartupDriveInfo;

struct StartupDriveInfo {
    uint8_t drive_number;
    uint8_t partition_number;
    uint8_t sub_partition_number;
    uint8_t subsub_partition_number;
} __attribute__ ((packed));

struct ArchSpecificKInfo {
    size_t startup_drive; /* 64-bit pointer to a StartupDriveInfo structure */
} __attribute__ ((packed));

#endif
