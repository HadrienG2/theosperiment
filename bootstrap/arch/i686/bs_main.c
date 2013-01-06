/* The bootstrap kernel, whose goal is to load the actual kernel

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

#include <asm_routines.h>
#include <bs_kernel_information.h>
#include <die.h>
#include <sgt_generation.h>
#include <stdint.h>
#include <kernel_loader.h>
#include <kinfo_handling.h>
#include <multiboot.h>
#include <paging.h>
#include <txt_videomem.h>

int bootstrap_longmode(const multiboot_info_t* mbd, const uint32_t magic) {
    KernelInformation* kinfo;

    //Video memory initialization
    init_videomem();
    clear_screen();

    //GRUB magic number check
    if(magic!=MULTIBOOT_BOOTLOADER_MAGIC) {
        die(MULTIBOOT_MISSING);
    }
    
    //Some silly text
    movecur_abs(26, 11);
    set_attr(TXT_WHITE);
    print_str("Greetings, OS|perimenter !");
    movecur_rel(-20, +1);
    set_attr(TXT_LIGHTPURPLE);
    print_str("Please wait...\n\n");
    set_attr(TXT_LIGHTRED);

    //Generate kernel information and check CPU features (we need long mode, SSE2 and NX to be available)
    kinfo = kinfo_gen(mbd);

    //Set up segmentation structures which are more safe than GRUB's ones
    replace_sgt(kinfo);

    //Generate a page table
    const uint32_t cr3_value = generate_paging(kinfo);

    //Switch to the 32-bit subset of long mode, enable SSE2
    enable_longmode(cr3_value);

    //Locate the kernel in memory
    const KernelMemoryMap* kernel_location = locate_kernel(kinfo);
    //Get kernel headers
    const Elf64_Ehdr* main_header = read_kernel_headers(kernel_location);
    //Load the kernel's ELF binary in memory
    load_kernel(kinfo, kernel_location, main_header, cr3_value);

    //Switch to long mode, run the kernel
    run_kernel(main_header->e_entry, kinfo);

    return 0;
}
