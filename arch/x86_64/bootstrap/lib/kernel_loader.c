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

#include <bs_string.h>
#include <die.h>
#include <gen_kernel_info.h>
#include <kernel_loader.h>

//Path to the kernel
const char* KERNEL_NAME="/system/kernel.bin";
//Error messages
const char* KERNEL_NOT_FOUND="Sorry, vital file /system/kernel.bin could not be found.";
const char* INVALID_KERNEL="Sorry, vital file /system/kernel.bin is corrupted and can't be safely used.";

void load_kernel(kernel_information* kinfo, kernel_memory_map* kernel, Elf64_Ehdr* main_header) {
  int i, loadable_count = 0;
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  //The program header table
  Elf64_Phdr* phdr_table = (Elf64_Phdr*) (uint32_t) (kernel->location + main_header->e_phoff);
  
  /* Locate program segments and load them in memory */
  for(i=0; i<main_header->e_phnum; ++i) {
    switch(phdr_table[i].p_type) {
      case PT_LOAD:
        break;
      case PT_DYNAMIC:
      case PT_INTERP:
      case PT_SHLIB:
        die(INVALID_KERNEL);
      default:
        continue;
    }
    if(phdr_table[i].p_filesz == 0) continue; //Loading an empty segment is pointless.
    
    //We have encountered a loadable segment. Let's load it...
    void* source = (void*) (uint32_t) (kernel->location + phdr_table[i].p_offset);
    void* dest = (void*) (uint32_t) (phdr_table[i].p_vaddr);
    uint32_t size = phdr_table[i].p_filesz;
    memcpy(dest, source, size);
    //...and add it to the memory map
    kmmap[kinfo->kmmap_size+loadable_count].location = phdr_table[i].p_vaddr;
    kmmap[kinfo->kmmap_size+loadable_count].size =
      ((uint32_t) (phdr_table[i].p_filesz) / (uint32_t) (phdr_table[i].p_align) + 1) * phdr_table[i].p_align;
    kmmap[kinfo->kmmap_size+loadable_count].nature = 3;
    if(phdr_table[i].p_flags == PF_R) {
      kmmap[kinfo->kmmap_size+loadable_count].name = (uint32_t) "Kernel R-- segment";
    } else if(phdr_table[i].p_flags == (PF_R + PF_W)) {
      kmmap[kinfo->kmmap_size+loadable_count].name = (uint32_t) "Kernel RW- segment";
    } else if(phdr_table[i].p_flags == (PF_R + PF_X)) {
      kmmap[kinfo->kmmap_size+loadable_count].name = (uint32_t) "Kernel R-X segment";
    } else die(INVALID_KERNEL);
    if(kinfo->kmmap_size+loadable_count > MAX_KMMAP_SIZE) die(MMAP_TOO_SMALL);
    ++loadable_count;
  }
  
  /* Update, sort, and merge the memory map */
  kinfo->kmmap_size+=loadable_count;
  sort_memory_map(kinfo);
  merge_memory_map(kinfo);
}

kernel_memory_map* locate_kernel(kernel_information* kinfo) {
  unsigned int i;
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  for(i=0; i<kinfo->kmmap_size; ++i) {
    if(!strcmp((char*) (uint32_t) kmmap[i].name, KERNEL_NAME)) {
      return &(kmmap[i]);
    }
  }
  
  die(KERNEL_NOT_FOUND);
  return 0; //This is totally useless, but GCC will issue a warning if it's not present
}

Elf64_Ehdr* read_kernel_headers(kernel_memory_map* kernel) {
  Elf64_Ehdr *header = (Elf64_Ehdr*) ((uint32_t) kernel->location);
  
  /* e_ident fields checks : Kernel must
      -Be a valid elf64 file compliant to the current standard
      -Use Little-endian (as this is a x86_64 port)
      -Not target any specific OS ABI or version */
  if(header->e_ident[0]!=0x7f || header->e_ident[1]!='E' || header->e_ident[2]!='L' || header->e_ident[3]!='F' ||
    header->e_ident[4]!=ELFCLASS64 || header->e_ident[5]!=ELFDATA2LSB || header->e_ident[6]!=EV_CURRENT ||
    header->e_ident[7]!=ELFOSABI_NONE || header->e_ident[8]!=0) die(INVALID_KERNEL);
  // e_type field check : Kernel must be an executable file
  if(header->e_type != ET_EXEC) die(INVALID_KERNEL);
  // e_machine field check : Kernel must target the AMD64 architecture
  if(header->e_machine != EM_X86_64) die(INVALID_KERNEL);
  // e_version field check : This must be current file version
  if(header->e_version != EV_CURRENT) die(INVALID_KERNEL);
  // e_entry check : The kernel must have an entry point
  if(!header->e_entry) die(INVALID_KERNEL);
  // e_phoff/e_phnum and e_shoff/e_phnum check : The kernel must have some valid program and session headers
  if(!header->e_phoff || !header->e_shoff || !header->e_phnum || !header->e_shnum) die(INVALID_KERNEL);
  // e_shstrndx check : There must be a section name string table
  if(header->e_shstrndx == SHN_UNDEF) die(INVALID_KERNEL);
  
  return header;
}