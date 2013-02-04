 /* Testing routines displaying kernel-related information

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

#include "display_kinfo.h"
#include <txt_videomem.h>

void dbg_print_kinfo(const KernelInformation* kinfo) {
    if(kinfo) {
        StartupDriveInfo* sdr_info = (StartupDriveInfo*) (uint32_t) kinfo->arch_info.startup_drive;
        //Display command line
        print_str("Kernel command line : ");
        if(kinfo->command_line) print_str((char*) (uint32_t) kinfo->command_line);
        print_chr('\n');

        //Memory map size
        print_str("Memory map size : ");
        print_int32(kinfo->kmmap_length);
        print_chr('\n');

        //Startup drive information
        if(kinfo->arch_info.startup_drive) {
            print_str("Startup drive is : ");
            print_uint8(sdr_info->drive_number);
            print_chr('/');
            print_uint8(sdr_info->partition_number);
            print_chr('/');
            print_uint8(sdr_info->sub_partition_number);
            print_chr('/');
            print_uint8(sdr_info->subsub_partition_number);
            print_chr('\n');
        }
    } else {
        print_str("Sorry, no kernel information available.\n");
    }
}

void dbg_print_kmmap(const KernelInformation* kinfo) {
    unsigned int i = 0;
    KernelMMapItem* kmmap = (KernelMMapItem*) (uint32_t) kinfo->kmmap;
    print_str("Address            | Size               | Type | Name\n");
    print_str("-------------------------------------------------------------------------------\n");
    for(; i<kinfo->kmmap_length; ++i) {
        print_hex64(kmmap[i].location);
        print_str(" | ");
        print_hex64(kmmap[i].size);
        print_str(" | ");

        switch(kmmap[i].nature) {
            case 0:
                print_str("FREE | ");
                break;
            case 1:
                print_str("RES  | ");
                break;
            case 2:
                print_str("BSK  | ");
                break;
            case 3:
                print_str("KNL  | ");
                break;
            case 4:
                print_str("MOD  | ");
                break;
            default:
                print_str("???  | ");
        }

        if(!kmmap[i].name) {
            print_str("\n");
        } else {
            print_str((char*) (uint32_t) kmmap[i].name);
            print_chr('\n');
        }
    }
}
