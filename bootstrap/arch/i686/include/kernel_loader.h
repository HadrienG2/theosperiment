 /* Routines for parsing ELF64 headers and loading the kernel where it ought to be

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

#ifndef _ELF64_PARSER_H_
#define _ELF64_PARSER_H_

#include <address.h>
#include <elf.h>
#include <bs_kernel_information.h>

//This function reads program headers and loads all loadable program segments in memory
void load_kernel(KernelInformation* kinfo,
                 const bs_size_t kernel_item,
                 const Elf64_Ehdr* main_header,
                 const bs_size_t cr3_value);
//Locate the kernel in the memory map
bs_size_t locate_kernel(const KernelInformation* kinfo);
//This function returns the ELF64 headers of the kernel after performing some checks on them
const Elf64_Ehdr* read_kernel_headers(const KernelMMapItem* kernel);

#endif
