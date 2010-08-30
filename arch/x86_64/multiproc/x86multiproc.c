 /* Everything needed to support the Intel MultiProcessor specification (currently version 1.4).
    Read chapter 4 of this specification ("MP configuration table") for more information.

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
 
#include <x86multiproc.h>

mp_floating_ptr* find_fptr() {
  //We want to check if multiprocessing is supported on this system. According to the
  //Intel MP specification, we should look for a "MP floating pointer structure" (defined
  //in x86multiproc.h as "mp_floating_ptr" and described in the Intel MP spec)
  //in the following places (in that order) :
  //  1/Extended BIOS Data Area (EBDA)
  //  2/Last kb of base memory (ie 639k->640k for modern computers) if EBDA does not exist
  //  3/Bios rom address space (0xf0000 -> 0xfffff)
  
  mp_floating_ptr* floating_ptr = 0;
  uint8_t fptr_found = 0;
  
  //EBDA base address is located in the BIOS Data Area (BDA). One has to use (the word at location
  //0x40e) << 4.
  //We can safely assume that EBDA does not exist if that address is less than 0x80000
  //(EBDA is guaranteed to be less than 128KB large) or equal to 0xA0000 (in that case, EBDA
  //has a null size).
  uint16_t* ebda_base_ptr = (uint16_t*) 0x40e;
  uint32_t location = *ebda_base_ptr << 4;
  uint32_t limit = 0xa0000 - sizeof(mp_floating_ptr);
  
  if((location>=0x80000) && (location<=0xa0000)) {
    //Method 1 is valid, method 2 is not.
    for(; location<=limit; ++location) {
      fptr_found = fptr_check(location);
      if(fptr_found) {
        floating_ptr = (mp_floating_ptr*) location;
        break;
      }
    }
  } else {
    //Method 2 is valid, method 1 is not.
    limit = 0xa0000 - sizeof(mp_floating_ptr);
    for(location = 0x9fc00; location<=limit; ++location) {
      fptr_found = fptr_check(location);
      if(fptr_found) {
        floating_ptr = (mp_floating_ptr*) location;
        break;
      }
    }
  }
  if(!fptr_found) {
    //Methods 1 and 2 have failed, use method 3
    limit = 0xfffff - sizeof(mp_floating_ptr);
    for(location = 0xf0000; location<=limit; ++location) {
      fptr_found = fptr_check(location);
      if(fptr_found) {
        floating_ptr = (mp_floating_ptr*) location;
        break;
      }
    }
  }
  
  return floating_ptr;
}

uint8_t fptr_check(uint32_t location) {
  mp_floating_ptr* floating_ptr = (mp_floating_ptr*) location;
  //Check floating pointer signature
  if(floating_ptr->signature[0]!='_') return 0;
  if(floating_ptr->signature[1]!='M') return 0;
  if(floating_ptr->signature[2]!='P') return 0;
  if(floating_ptr->signature[3]!='_') return 0;
  //Check that length is nonzero and checksum
  if(!(floating_ptr->length)) return 0;
  uint32_t remaining_size = floating_ptr->length*16;
  uint8_t checksum_check = 0;
  uint8_t* byte_ptr = (uint8_t*) floating_ptr;
  while(remaining_size) {
    checksum_check += *byte_ptr;
    ++byte_ptr;
    --remaining_size;
  }
  if(checksum_check) return 0;
  //All good !
  return 1;
}

mp_config_table_hdr* mpconfig_check(mp_floating_ptr* floating_ptr) {
  mp_config_table_hdr* config_hdr = (mp_config_table_hdr*) floating_ptr->phy_addr_ptr;
  //Check that MP configuration table is present
  if(!config_hdr) return 0;
  //Check configuration table signature
  if(config_hdr->signature[0]!='P') return 0;
  if(config_hdr->signature[1]!='C') return 0;
  if(config_hdr->signature[2]!='M') return 0;
  if(config_hdr->signature[3]!='P') return 0;
  //Check the checksum of the base configuration table
  if(config_hdr->base_table_length < sizeof(mp_config_table_hdr)) return 0; //Size check
  uint8_t checksum_check = 0;
  uint32_t remaining_size = config_hdr->base_table_length;
  uint8_t* byte_ptr = (uint8_t*) config_hdr;
  while(remaining_size) {
    checksum_check += *byte_ptr;
    ++byte_ptr;
    --remaining_size;
  }
  if(checksum_check) return 0;
  //All good !
  return config_hdr;
}
