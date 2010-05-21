/* multiboot.h - the header for Multiboot */
/* Copyright (C) 2010   Hadrien Grasland
   Copyright (C) 1999, 2001  Free Software Foundation, Inc.

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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <hack_stdint.h>

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* The flags for the Multiboot header. */
#define MULTIBOOT_HEADER_FLAGS         0x00000007

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

#ifndef ASM
  /* Do not include here in boot.S. */

  /* Types. */

  /* The Multiboot header. */
  typedef struct multiboot_header
  {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
  } multiboot_header_t;

  /* The symbol table for a.out. */
  typedef struct aout_symbol_table
  {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
  } aout_symbol_table_t;

  /* The section header table for ELF. */
  typedef struct elf_section_header_table
  {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
  } elf_section_header_table_t;

  /* The module structure. */
  typedef struct module
  {
    uint32_t mod_start;
    uint32_t mod_end;
    char* string;
    uint32_t reserved;
  } module_t;

  /* The memory map. Be careful that the offset 0 is base_addr_low
    but no size. */
  typedef struct memory_map
  {
    uint32_t size;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
  } memory_map_t;
  
  /* The drive structure */
  typedef struct drive_structure
  {
    uint32_t size;
    uint8_t drive_number;
    uint8_t drive_mode; //0=CHS, 1=LBA
    uint16_t drive_cylinders;
    uint8_t drive_heads;
    uint8_t drive_sectors;
    uint8_t first_port; //There may be more than one. Use pointers if needed
  } drive_structure_t;
  
  typedef struct apm_table
  {
     uint16_t version;
     uint16_t cseg;
     uint32_t offset;
     uint16_t cseg_16;
     uint16_t dseg;
     uint16_t flags;
     uint16_t cseg_len;
     uint16_t cseg_16_len;
     uint16_t dseg_len;
  } apm_table_t;
  
  /* The Multiboot information. */
  typedef struct multiboot_info
  {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    char* cmdline;
    uint32_t mods_count;
    module_t* mods_addr;
    union
    {
      aout_symbol_table_t aout_sym;
      elf_section_header_table_t elf_sec;
    } u;
    uint32_t mmap_length;
    memory_map_t* mmap_addr;
    uint32_t drives_length;
    drive_structure_t* drives_addr;
    uint32_t config_table;
    char* boot_loader_name;
    apm_table_t* apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
  } multiboot_info_t;

#endif /* ! ASM */
#endif