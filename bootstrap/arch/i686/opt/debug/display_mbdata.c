 /* Functions reading and displaying the multiboot information structure

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

#include <display_mbdata.h>
#include "txt_videomem.h"
#include <stdint.h>

void dbg_print_mbflags(const multiboot_info_t* mbd) {
        print_str("Here's what GRUB tells us :\n");

    if(mbd->flags & 1) {
        print_str("* We know how much memory we have : ");
        print_uint32(mbd->mem_lower);
        print_str("kB + ");
        print_uint32(mbd->mem_upper);
        print_str("kB\n");
    }
    else print_str("* We don't know how much memory we have\n");

    if(mbd->flags & 2) {
        print_str("* We know where we boot from : ");
        print_uint8(mbd->boot_device/(256*256*256));
        print_chr('/');
        print_uint8((mbd->boot_device/(256*256))%256);
        print_chr('/');
        print_uint8((mbd->boot_device/256)%256);
        print_chr('/');
        print_uint8(mbd->boot_device%256);
        print_chr('\n');
    }
    else print_str("* We don't know where we boot from\n");

    if(mbd->flags & 4) {
        print_str("* We know the command line : ");
        print_str(mbd->cmdline);
        print_chr('\n');
    }
    else print_str("* We don't know the command line\n");

    if(mbd->flags & 8) {
        print_str("* We know about ");
        print_uint32(mbd->mods_count);
        print_str(" modules\n");
    }
    else print_str("* We don't know about modules\n");

    if(mbd->flags & 16) print_str("* HORROR ! GRUB has mistaken us for an a.out kernel !\n");
    else print_str("* Grub knows we're not an a.out kernel\n");

    if(mbd->flags & 32) {
        print_str("* We know something about our symbol table (and we don't care)\n");
    }
    else print_str("* We don't know anything about our symbol table\n");

    if(mbd->flags & 64) {
        print_str("* We have a map of memory (see below)\n");
    }
    else print_str("* We don't have a map of memory\n");

    if(mbd->flags & 128) {
        print_str("* We know about physical drives\n");
    }
    else print_str("* We don't know about physical drives\n");

    if(mbd->flags & 256) {
        print_str("* We know about ROM configuration, it's at : ");
        print_hex32(mbd->config_table);
        print_chr('\n');
    }
    else print_str("* We don't know about ROM configuration\n");

    if(mbd->flags & 512) {
        print_str("* We know our bootloader by name : it's ");
        print_str(mbd->boot_loader_name);
        print_chr('\n');
    }
    else print_str("* We don't know our bootloader by name\n");

    if(mbd->flags & 1024) {
        print_str("* We know about the APM table at ");
        print_hex32((uint32_t) mbd->apm_table);
        print_chr('\n');
    }
    else print_str("* We don't know about the APM table\n");

    if(mbd->flags & 2048) print_str("* We know about VBE graphics capabilities\n");
    else print_str("* We don't know about VBE graphics capabilities\n");
}

void dbg_print_mmap(const multiboot_info_t* mbd) {
    uint32_t hack_current_mmap;
    uint64_t data64;

    if(mbd->flags & 64) {
        int remaining_mmap = mbd->mmap_length;
        memory_map_t* current_mmap = mbd->mmap_addr;
        print_str("Memory map :\n");
        while(remaining_mmap > 0) {
            //Display contents
            print_str("* Memory slice at ");
            data64 = current_mmap->base_addr_high;
            data64 *= 0x100000000;
            data64 += current_mmap->base_addr_low;
            print_hex64(data64);
            print_str(" of size ");
            data64 = current_mmap->length_high;
            data64 *= 0x100000000;
            data64 += current_mmap->length_low;
            print_hex64(data64);
            if(current_mmap->type == 1) print_str(" is available\n");
            else print_str(" is reserved\n");
            //Move to next mmap item
            hack_current_mmap = (uint32_t) current_mmap;
            hack_current_mmap += current_mmap->size+4;
            remaining_mmap -= current_mmap->size+4;
            current_mmap = (memory_map_t*) hack_current_mmap;
        }
    } else print_str("No memory map found\n");
}

void dbg_print_drvmap(const multiboot_info_t* mbd) {
    uint32_t remaining_drvmap;
    drive_structure_t* current_drvmap;
    uint32_t hack_current_drvmap;
    unsigned char* current_port;

    if((mbd->flags & 128) && (mbd->drives_length!=0)) {
        remaining_drvmap = mbd->drives_length;
        current_drvmap = mbd->drives_addr;
        print_str("Drives map :\n");
        print_int32(remaining_drvmap);
        while(remaining_drvmap > 0) {
            //Display contents
            print_str("* Drive number ");
            print_int32(current_drvmap->drive_number);
            print_str(" uses ");
            if(current_drvmap->drive_mode == 0) print_str("CHS");
            if(current_drvmap->drive_mode == 1) print_str("LBA");
            print_str(", has ");
            print_int32(current_drvmap->drive_cylinders);
            print_str(" cylinders, ");
            print_int32(current_drvmap->drive_heads);
            print_str("heads, and has ");
            print_int32(current_drvmap->drive_sectors);
            print_str(" sectors/tracks\n    It is used via ports ");
            current_port = &(current_drvmap->first_port);
            while(*current_port!=0) {
                print_int32(*current_port);
                ++current_port;
            }
            //Move to next drvmap item
            hack_current_drvmap = (uint32_t) current_drvmap;
            hack_current_drvmap += current_drvmap->size;
            remaining_drvmap -= current_drvmap->size;
            current_drvmap = (drive_structure_t*) hack_current_drvmap;
        }
    } else print_str("No drives map found\n");
}
