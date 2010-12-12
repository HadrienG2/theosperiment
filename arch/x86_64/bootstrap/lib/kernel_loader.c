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

#include <align.h>
#include <bs_string.h>
#include <die.h>
#include <kernel_loader.h>
#include <kinfo_handling.h>
#include <paging.h>

//Path to the kernel
const char* KERNEL_NAME="/system/kernel.bin";

void load_kernel(KernelInformation* kinfo,
                 const KernelMemoryMap* kernel,
                 const Elf64_Ehdr* main_header,
                 const uint32_t cr3_value) {
  unsigned int i, size;
  uint64_t load_addr, current_offset, flags;
  void *source, *dest;
  char* mmap_name;
  //The program header table
  const Elf64_Phdr* phdr_table = (const Elf64_Phdr*) (uint32_t) (kernel->location + main_header->e_phoff);
  
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
    if(phdr_table[i].p_memsz == 0) continue; //Loading an empty segment is pointless.
    
    //We have encountered a loadable segment. Let's allocate enough space to load it. The virtual
    //address will later be mapped there using some paging tricks later
    switch(phdr_table[i].p_flags) {
      case PF_R:
        mmap_name = "Kernel R-- segment";
        flags = PBIT_NOEXECUTE;
        break;
      case PF_R+PF_W:
        mmap_name =  "Kernel RW- segment";
        flags = PBIT_NOEXECUTE + PBIT_WRITABLE;
        break;
      case PF_R+PF_X:
        mmap_name =  "Kernel R-X segment";
        flags = 0;
        break;
      default:
        mmap_name = "";
        flags = 0;
        die(INVALID_KERNEL);
    }
    load_addr = kmmap_alloc_pgalign(kinfo, phdr_table[i].p_memsz, NATURE_KNL, mmap_name);
    
    //Then we can load the part located in the ELF file
    source = (void*) (uint32_t) (kernel->location + phdr_table[i].p_offset);
    dest = (void*) (uint32_t) (load_addr);
    size = phdr_table[i].p_filesz;
    memcpy(dest, source, size);
    //And then zero out the part which is *not* in the file (That's probably some kind of .bss)
    dest = (void*) (uint32_t) (load_addr + phdr_table[i].p_filesz);
    size = align_up((uint32_t) phdr_table[i].p_memsz, (uint32_t) phdr_table[i].p_align);
    size -= phdr_table[i].p_filesz;
    memset(dest, 0, size);
    
    //Now setup paging so that p_vaddr *does* point to load_addr
    for(current_offset=0; current_offset<phdr_table[i].p_memsz; current_offset+=PG_SIZE) {
      setup_pagetranslation(kinfo, cr3_value, phdr_table[i].p_vaddr+current_offset,
        load_addr+current_offset, flags);
    }
    
    //These are the sole page translations we'll ever use in the kernel. If we could do otherwise,
    //we would just ignore them, but making a relocatable kernel binary seems too complex.
    //As the kernel is small, we can afford losing some memory by mapping the virtual memory region
    //where the kernel is located as reserved.
    kmmap_add(kinfo,
              phdr_table[i].p_vaddr,
              align_pgup(phdr_table[i].p_memsz),
              NATURE_RES,
              "Kernel mapping region");
    kmmap_update(kinfo);
  }
}

const KernelMemoryMap* locate_kernel(const KernelInformation* kinfo) {
  unsigned int i;
  const KernelMemoryMap* kmmap = (const KernelMemoryMap*) (uint32_t) kinfo->kmmap;
  for(i=0; i<kinfo->kmmap_size; ++i) {
    if(!strcmp((char*) (uint32_t) kmmap[i].name, KERNEL_NAME)) {
      return &(kmmap[i]);
    }
  }
  
  die(KERNEL_NOT_FOUND);
  return 0; //This is totally useless, but GCC will issue a warning if it's not present
}

const Elf64_Ehdr* read_kernel_headers(const KernelMemoryMap* kernel) {
  const Elf64_Ehdr *header = (const Elf64_Ehdr*) ((uint32_t) kernel->location);
  
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