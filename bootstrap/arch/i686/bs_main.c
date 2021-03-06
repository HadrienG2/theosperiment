/* The bootstrap kernel, whose goal is to load the actual kernel

    Copyright (C) 2010-2013    Hadrien Grasland

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

#include <asm_routines.h>
#include <bs_KernelInformation.h>
#include <die.h>
#include <print_logo.h>
#include <sgt_generation.h>
#include <stdint.h>
#include <kernel_loader.h>
#include <kinfo_handling.h>
#include <multiboot.h>
#include <paging.h>
#include <txt_videomem.h>

int bootstrap_longmode(const multiboot_info_t* mbd, const uint32_t magic) {
    KernelInformation* kinfo;
    int i;

    //Video memory initialization
    init_videomem();
    clear_screen();

    //GRUB magic number check
    if(magic!=MULTIBOOT_BOOTLOADER_MAGIC) {
        die(MULTIBOOT_MISSING);
    }
    
    //Display TOSP logo
    set_attr(TXT_WHITE);
    movecur_abs(1,0);
    print_logo();
    set_attr(TXT_LIGHTGRAY);
    for(i = 0; i<24; ++i) print_chr(-60);
    set_attr(TXT_LIGHTPURPLE);
    print_str(" Booting kernel, please wait... ");
    set_attr(TXT_LIGHTGRAY);
    for(i = 0; i<24; ++i) print_chr(-60);
    set_attr(TXT_LIGHTGRAY);

    //Generate kernel information and check CPU features (we need long mode, SSE2 and NX to be available)
    kinfo = kinfo_gen(mbd);

    //Set up segmentation structures which are more safe than GRUB's ones
    replace_sgt(kinfo);

    //Generate a page table
    const uint32_t cr3_value = generate_paging(kinfo);

    //Switch to the 32-bit subset of long mode, enable SSE2
    enable_longmode(cr3_value);

    //Locate the kernel in the memory map
    const bs_size_t kernel_item = locate_kernel(kinfo);
    //Get kernel headers
    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);
    const Elf64_Ehdr* main_header = read_kernel_headers(&(kmmap[kernel_item]));
    //Load the kernel's ELF binary in memory
    load_kernel(kinfo, kernel_item, main_header, cr3_value);

    //Switch to long mode, run the kernel
    run_kernel(main_header->e_entry, kinfo);

    return 0;
}
