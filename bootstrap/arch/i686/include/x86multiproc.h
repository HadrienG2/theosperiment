 /* Everything needed to support the Intel MultiProcessor specification (currently version 1.4).
    Read chapter 4 of this specification ("MP configuration table") for more information.

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

#ifndef _X86MP_H_
#define _X86MP_H_

#include <stdint.h>

typedef struct mp_floating_ptr mp_floating_ptr;
typedef struct mp_config_table_hdr mp_config_table_hdr;
typedef struct mpconfig_proc_entry mpconfig_proc_entry;

struct mp_floating_ptr {
    char signature[4]; //Must contain "_MP_"
    uint32_t phy_addr_ptr; //If nonzero, pointer to the MP config table's header
    uint8_t length; //Length of this structure in 16-bytes chunks.
    uint8_t spec_rev; //Spec revision supported. 1 for version 1.1 and 4 for version 1.4 (current version)
    uint8_t checksum; //All bytes withing this structure (including optional addenda specified by the
                                        //"length" field) must add up to zero.
    uint8_t mp_sys_config_type; //If a default 2-core configuration is being used, this field is nonzero.
    uint32_t mp_feature_info_bits;
} __attribute__((packed));

struct mp_config_table_hdr {
    char signature[4]; //Must contain "PCMP"
    uint16_t base_table_length; //Length of the base configuration table in bytes, including header.
    uint8_t spec_rev; //Contains 1 for revision 1.1 and 4 for revision 1.4 of the MultiProcessor spec
    uint8_t checksum; //All bytes of the base config table (including header) must add up to zero.
    char oem_id[8];
    char product_id[12];
    uint32_t oem_table_ptr;
    uint16_t oem_table_sz;
    uint16_t entry_count; //Number of entries in the base config table
    uint32_t lapic_address; //Memory-mapped address of the Local APIC
    uint16_t ext_table_length; //Length of extended table entries (located after base table)
    uint8_t ext_table_checksum; //Checksum of the extended table entries. Same system as before.
    uint8_t reserved;
} __attribute__((packed));

struct mpconfig_proc_entry {
    uint8_t entry_type; //Must be zero
    uint8_t lapic_id;
    uint8_t lapic_version;
    uint8_t cpu_flags; //First bit tells if processor is usable, second bit tells if it is the BSP.
    uint32_t signature;
    uint32_t feature_flags;
    uint64_t reserved;
} __attribute__((packed));

mp_floating_ptr* find_fptr(); //Finds the floating pointer, if it exists, otherwise returns 0
uint8_t fptr_check(const uint32_t location); //Check if there's a valid floating pointer at this location
mp_config_table_hdr* mpconfig_check(const mp_floating_ptr* floating_ptr); //Check MP configuration table.

#endif
