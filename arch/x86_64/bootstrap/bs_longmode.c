/* The bootstrap kernel, whose goal is to load the actual kernel

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

#include <bs_kernel_information.h>
#include <die.h>
#include <display_paging.h>
#include <enable_longmode.h>
#include <gen_kernel_info.h>
#include <hack_stdint.h>
#include <kernel_loader.h>
#include <multiboot.h>
#include <paging.h>
#include <txt_videomem.h>


const char* MULTIBOOT_MISSING = "The operating system was apparently not loaded by GRUB.\n\
GRUB brings mandatory information to it. Hence I'm afraid operation must stop.";
const char* NO_MEMORYMAP = "A mandatory part of the system, the memory map, could not be found.\n\
This may come from a GRUB malfunction.\nIn doubt, please contact us and tell us about this issue.";
const char* NO_LONGMODE = "Sorry, but we require 64-bit support, which your computer does not seem to provide.";

int bootstrap_longmode(multiboot_info_t* mbd, uint32_t magic) { 
  kernel_information* kinfo;
  kernel_memory_map* kernel_location;
  Elf64_Ehdr *main_header;
  uint64_t cr3_value;
  
  //Video memory initialization (for kernel silencing purposes)
  dbg_init_videomem();
  dbg_clear_screen();
  
  //GRUB magic number check
  if(magic!=MULTIBOOT_BOOTLOADER_MAGIC) {
    die(MULTIBOOT_MISSING);
  }
  
  //Some silly text
  dbg_set_attr(DBG_TXT_WHITE);
  dbg_print_str("Welcome to ToolbOS v0.0.3 ");
  dbg_set_attr(DBG_TXT_LIGHTPURPLE);
  dbg_print_str("\"Spartan's Delight\"\n\n");
  dbg_set_attr(DBG_TXT_LIGHTGRAY);
  
  //Generate kernel information
  kinfo = generate_kernel_info(mbd);
  if(!kinfo->kmmap) die(NO_MEMORYMAP);
  //Locate the kernel in memory
  kernel_location = locate_kernel(kinfo);
  //Get kernel headers
  main_header = read_kernel_headers(kernel_location);
  //Load the kernel in memory and add its "segments" to the memory map
  load_kernel(kinfo, kernel_location, main_header);
  //Generate a page table
  cr3_value = generate_paging(kinfo);
  dbg_print_pml4t(cr3_value);
  
  //Switch to longmode, run the kernel
  /*int ret = run_kernel(cr3_value, (uint32_t) main_header->e_entry, (uint32_t) kinfo);
  if(ret==-1) die(NO_LONGMODE);*/

  return 0;
}