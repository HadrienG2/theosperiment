 /* Testing routines displaying kernel-related information

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
 
#include "display_kinfo.h"
#include "txt_videomem.h"
 
void dbg_print_kinfo(kernel_information* kinfo) {
  if(kinfo) {
    startup_drive_info* sdr_info = (startup_drive_info*) (uint32_t) kinfo->arch_info.startup_drive;
    //Display command line
    dbg_print_str("Kernel command line : ");
    if(kinfo->command_line) dbg_print_str((char*) (uint32_t) kinfo->command_line);
    dbg_print_chr('\n');
    
    //Memory map size
    dbg_print_str("Memory map size : ");
    dbg_print_int32(kinfo->kmmap_size);
    dbg_print_chr('\n');
    
    //Startup drive information
    if(kinfo->arch_info.startup_drive) {
      dbg_print_str("Startup drive is : ");
      dbg_print_uint8(sdr_info->drive_number);
      dbg_print_chr('/');
      dbg_print_uint8(sdr_info->partition_number);
      dbg_print_chr('/');
      dbg_print_uint8(sdr_info->sub_partition_number);
      dbg_print_chr('/');
      dbg_print_uint8(sdr_info->subsub_partition_number);
      dbg_print_chr('\n');
    }
    
    //Kernel memory map
    dbg_print_chr('\n');
    dbg_print_kmmap(kinfo);
  } else {
    dbg_print_str("Sorry, no kernel information available.\n");
  }
}

void dbg_print_kmmap(kernel_information* kinfo) {
  unsigned int i = 0;
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  dbg_print_str("Address            | Size               | Type | Name\n");
  dbg_print_str("-------------------------------------------------------------------------------\n");
  for(; i<kinfo->kmmap_size; ++i) {
    dbg_print_hex64(kmmap[i].location);
    dbg_print_str(" | ");
    dbg_print_hex64(kmmap[i].size);
    dbg_print_str(" | ");
    
    switch(kmmap[i].nature) {
      case 0:
        dbg_print_str("FREE | ");
        break;
      case 1:
        dbg_print_str("RES  | ");
        break;
      case 2:
        dbg_print_str("BSK  | ");
        break;
      case 3:
        dbg_print_str("KNL  | ");
        break;
      default:
        dbg_print_str("UNSP | ");
    }
    
    if(!kmmap[i].name) {
      dbg_print_str("\n");
    } else {
      dbg_print_str((char*) (uint32_t) kmmap[i].name);
      dbg_print_chr('\n');
    }
  }
}