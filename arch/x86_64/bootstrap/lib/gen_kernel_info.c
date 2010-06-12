 /* Functions generating the kernel information structure

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

const char* MMAP_TOO_SMALL = "A core element of the system (memory map) is too small.\n\
This is a design mistake from our part.\nPlease tell us about this problem, so that we may fix it.";

kernel_memory_map* add_bios_mmap(kernel_memory_map* kmmap_buffer, int *index_ptr, multiboot_info_t* mbd) {
  if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
  if((*index_ptr < 0) || (*index_ptr>=MAX_KMMAP_SIZE)) return 0; //Parameter checking, round 2

  int index = *index_ptr;
  if(!(mbd->flags & 64)) return 0; //No memory map without multiboot's help
  //Variables for hacking through the memory map
  int remaining_mmap;
  memory_map_t* current_mmap;
  uint32_t hack_current_mmap;
  uint64_t addr64, size64;
  
  //BIOS memory map scanning
  remaining_mmap = mbd->mmap_length;
  current_mmap = mbd->mmap_addr;
  while(remaining_mmap > 0) {
    addr64 = current_mmap->base_addr_high;
    addr64 *= 0x100000000;
    addr64 += current_mmap->base_addr_low;
    size64 = current_mmap->length_high;
    size64 *= 0x100000000;
    size64 += current_mmap->length_low;
    kmmap_buffer[index].location = addr64;
    kmmap_buffer[index].size = size64;
    if(current_mmap->type == 1) {
      kmmap_buffer[index].nature = 0; //Memory is available
    } else {
      kmmap_buffer[index].nature = 1; //Memory is reserved
    }
    if(addr64 < 0x100000) {
      kmmap_buffer[index].name = (uint32_t) "Low mem";
    } else {
      kmmap_buffer[index].name = (uint32_t) "High mem";
    }
    
    //Move to next mmap item
    hack_current_mmap = (uint32_t) current_mmap;
    hack_current_mmap += current_mmap->size+4;
    remaining_mmap -= current_mmap->size+4;
    current_mmap = (memory_map_t*) hack_current_mmap;
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
  }
  
  *index_ptr = index;
  return kmmap_buffer;
}

kernel_memory_map* add_bskernel(kernel_memory_map* kmmap_buffer, int *index_ptr, multiboot_info_t* mbd) {
  if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
  if((*index_ptr < 0) || (*index_ptr>=MAX_KMMAP_SIZE)) return 0; //Parameter checking, round 2

  int index = *index_ptr;
  extern char sbs_kernel;
  extern char ebs_kernel;
  
  //Main multiboot structure
  kmmap_buffer[index].location = (uint32_t) &sbs_kernel;
  kmmap_buffer[index].size = (uint32_t) &ebs_kernel - (uint32_t) &sbs_kernel + 1;
  kmmap_buffer[index].nature = 2;
  kmmap_buffer[index].name = (uint32_t) "Bootstrap kernel";
  ++index;
  if(index>=MAX_KMMAP_SIZE) {
    //Not enough room for memory map, quitting...
    die(MMAP_TOO_SMALL);
  }  
  
  *index_ptr = index;
  return kmmap_buffer;  
}

kernel_memory_map* add_mbdata(kernel_memory_map* kmmap_buffer, int *index_ptr, multiboot_info_t* mbd) {
  if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
  if((*index_ptr < 0) || (*index_ptr>=MAX_KMMAP_SIZE)) return 0; //Parameter checking, round 2
  
  int index = *index_ptr;

  //Main multiboot structure
  kmmap_buffer[index].location = (uint32_t) mbd;
  kmmap_buffer[index].size = sizeof(multiboot_info_t);
  kmmap_buffer[index].nature = 2;
  kmmap_buffer[index].name = (uint32_t) "Main multiboot structure";
  ++index;
  if(index>=MAX_KMMAP_SIZE) {
    //Not enough room for memory map, quitting...
    die(MMAP_TOO_SMALL);
  }
  
  //Command line
  if(mbd->flags & 4) {
    kmmap_buffer[index].location = (uint32_t) mbd->cmdline;
    kmmap_buffer[index].size = strlen(mbd->cmdline)+1;
    kmmap_buffer[index].nature = 2;
    kmmap_buffer[index].name = (uint32_t) "Kernel command line";
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
  }
  
  //Modules information
  if(mbd->flags & 8) {
    kmmap_buffer[index].location = (uint32_t) mbd->mods_addr;
    kmmap_buffer[index].size = mbd->mods_count*sizeof(module_t);
    kmmap_buffer[index].nature = 2;
    kmmap_buffer[index].name = (uint32_t) "Multiboot modules information";
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
    
    unsigned int current_mod = 0;
    while(current_mod < mbd->mods_count) {
      kmmap_buffer[index].location = (uint32_t) mbd->mods_addr[current_mod].string;
      kmmap_buffer[index].size = strlen(mbd->mods_addr[current_mod].string)+1;
      kmmap_buffer[index].nature = 2;
      kmmap_buffer[index].name = (uint32_t) "Multiboot modules string";
      
      //Move to next module
      ++current_mod;
      ++index;
      if(index>=MAX_KMMAP_SIZE) {
        //Not enough room for memory map, quitting...
        die(MMAP_TOO_SMALL);
      }
    }
  }

  //BIOS drives structure
  if((mbd->flags & 128) && (mbd->drives_length!=0)) {
    kmmap_buffer[index].location = (uint32_t) mbd->drives_addr;
    kmmap_buffer[index].size = mbd->drives_length;
    kmmap_buffer[index].nature = 2;
    kmmap_buffer[index].name = (uint32_t) "BIOS drives structure";
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
  }
  
  //Bootloader name
  if(mbd->flags & 512) {
    kmmap_buffer[index].location = (uint32_t) mbd->boot_loader_name;
    kmmap_buffer[index].size = strlen(mbd->boot_loader_name)+1;
    kmmap_buffer[index].nature = 2;
    kmmap_buffer[index].name = (uint32_t) "Boot loader name";
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
  }
  
  //APM table
  if(mbd->flags & 1024) {
    kmmap_buffer[index].location = (uint32_t) mbd->apm_table;
    kmmap_buffer[index].size = sizeof(apm_table_t);
    kmmap_buffer[index].nature = 2;
    kmmap_buffer[index].name = (uint32_t) "BIOS APM table";
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
      //Not enough room for memory map, quitting...
      die(MMAP_TOO_SMALL);
    }
  }
  
  *index_ptr = index;
  return kmmap_buffer;  
}

kernel_memory_map* add_modules(kernel_memory_map* kmmap_buffer, int* index_ptr, multiboot_info_t* mbd) {
  if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
  if((*index_ptr < 0) || (*index_ptr>=MAX_KMMAP_SIZE)) return 0; //Parameter checking, round 2
  
  int index = *index_ptr;
  if(mbd->flags & 8) {
    unsigned int current_mod = 0;
    while(current_mod < mbd->mods_count) {
      kmmap_buffer[index].location = mbd->mods_addr[current_mod].mod_start;
      kmmap_buffer[index].size = mbd->mods_addr[current_mod].mod_end -
        mbd->mods_addr[current_mod].mod_start + 1;
      kmmap_buffer[index].nature = 3; //The kernel and its modules
      kmmap_buffer[index].name = (uint32_t) mbd->mods_addr[current_mod].string;
      
      //Move to next module
      ++current_mod;
      ++index;
      if(index>=MAX_KMMAP_SIZE) {
        //Not enough room for memory map, quitting...
        die(MMAP_TOO_SMALL);
      }
    }
  }
  
  *index_ptr = index;
  return kmmap_buffer;
}

kernel_memory_map* copy_memory_map_chunk(kernel_memory_map* source, kernel_memory_map* dest, unsigned int start, unsigned int length) {
  if((!source) || (!dest) || (length+start > MAX_KMMAP_SIZE)) {
    return 0;
  }
  
  unsigned int i;
  for(i=start; i<start+length; ++i) copy_memory_map_elt(source, dest, i, i);
  
  return dest;  
}

kernel_memory_map* copy_memory_map_elt(kernel_memory_map* source, kernel_memory_map* dest, unsigned int source_index, unsigned int dest_index) {
  if((!source) || (!dest) || (source_index >= MAX_KMMAP_SIZE) || (dest_index >= MAX_KMMAP_SIZE)) {
    return 0;
  }
  
  dest[dest_index].location = source[source_index].location;
  dest[dest_index].size = source[source_index].size;
  dest[dest_index].nature = source[source_index].nature;
  dest[dest_index].name = source[source_index].name;
  
  return dest;
}

kernel_information* generate_kernel_info(multiboot_info_t* mbd) {
  //Some buffers
  static kernel_information result;
  static startup_drive_info sd_buff;
  
  //Let's say we've nothing available in the beginning
  result.command_line = 0;
  result.kmmap = 0;
  result.kmmap_size = 0;
  result.arch_info.startup_drive = 0;
  
  //Get command line from bootloader (if available)
  if(mbd->flags & 4) result.command_line = (uint32_t) mbd->cmdline;
  //Same for startup drive
  if(mbd->flags & 2) {
    sd_buff.drive_number = mbd->boot_device/(256*256*256);
    sd_buff.partition_number = (mbd->boot_device/(256*256))%256;
    sd_buff.sub_partition_number = (mbd->boot_device/256)%256;
    sd_buff.subsub_partition_number = mbd->boot_device%256;
  }
  result.arch_info.startup_drive = (uint32_t) &sd_buff;
  /* Memory map generation is a bit more tricky, since we must take kernel memory
  into account, maybe multiboot data, and have to use dynamic memory allocation.
  We give the job to a dedicated function */
  generate_memory_map(mbd, &result);
  
  return &result;
}

kernel_memory_map* generate_memory_map(multiboot_info_t* mbd, kernel_information* kinfo) {
  //A buffer for memory map.
  static kernel_memory_map kmmap_buffer[MAX_KMMAP_SIZE];
  int index = 0;

  /*This function
    1/Packs all memory-related data in a memory map
    2/Sorts said memory map and
    3/Removes duplicated entries
    
    The map is made using
      *BIOS-provided memory map
      *Multiboot modules information
      *Multiboot information structure elements' address and size.
      *Bootstrap kernel's data address and size
  */
  
  //Here's part 1
  if(add_bios_mmap(kmmap_buffer, &index, mbd) == 0) return 0;
  if(add_modules(kmmap_buffer, &index, mbd) == 0) return 0;
  if(add_mbdata(kmmap_buffer, &index, mbd) == 0) return 0;
  if(add_bskernel(kmmap_buffer, &index, mbd) == 0) return 0;
  kinfo->kmmap = (uint32_t) kmmap_buffer;
  kinfo->kmmap_size = index;
  
  //Here's part 2
  if(sort_memory_map(kinfo) == 0) return 0;
  
  //Here's part 3
  if(merge_memory_map(kinfo) == 0) return 0;
  
  return kmmap_buffer;
}

kernel_memory_map* merge_memory_map(kernel_information* kinfo) {
  /* Goal of this function : parts of the memory map (segments) are overlapping, due to the fact that
     memory map provided by GRUB does not take account of used memory, only knowing the difference
     between reserved ram and "free" ram (which, in fact, isn't necessarily free).
     
     We want to merge overlapping parts and get a flat map (something like...
          AAAAAAAAAAA
              BBBB
                          CCC
       => AAAABBBBAAA     CCC )
     
     Simplifications :
      -> Two segments overlapping simulatneously as a maximum, one of them being free memory
          and the other being some kind of used memory region.
      -> Free memory segments always begin before and end after the overlapping segments. */
  
  kernel_memory_map tmp = {0,0,0,0};
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  static kernel_memory_map kmmap_buffer[MAX_KMMAP_SIZE];
  unsigned int dest_index = 0, source_index, free_index;
  
  /* It's simpler if the free memory segments are listed before their used counterpart in
    main memory. The only case where this may not happen is when a used segment begins at the same
    address as a non-free one, so it does rarely happen. We'll take care of that. */
  for(source_index = 0; source_index < kinfo->kmmap_size; ++source_index) {
    if(kmmap[source_index].nature == 0 && source_index != 0) {
      if(kmmap[source_index-1].location == kmmap[source_index].location)
      {
        copy_memory_map_elt(kmmap, &tmp, source_index, 0);
        copy_memory_map_elt(kmmap, kmmap, source_index-1, source_index);
        copy_memory_map_elt(&tmp, kmmap, 0, source_index-1);
      }
    }
  }

  // Now, let's begin the real stuff...
  for(source_index = 0; source_index<kinfo->kmmap_size; ++source_index) {
    //Is this a reserved segment ?
    if(kmmap[source_index].nature != 0) {
      copy_memory_map_elt(kmmap, kmmap_buffer, source_index, dest_index++);
      if(dest_index >= MAX_KMMAP_SIZE) {
        //Memory map overflow. Quitting...
        die(MMAP_TOO_SMALL);
      }
    } else {
      //It is a free segment. Overlap may occur.
      free_index = source_index;
      
      
      while((source_index < kinfo->kmmap_size) && (kmmap[source_index+1].location < kmmap[free_index].location + kmmap[free_index].size)) {
        ++source_index;
        //Add free space behind this new segment to memory map, if needed.
        if(source_index != free_index+1) {
          if(kmmap[source_index].location != kmmap[source_index-1].location + kmmap[source_index-1].size) {
            kmmap_buffer[dest_index].location =  kmmap[source_index-1].location + kmmap[source_index-1].size;
            kmmap_buffer[dest_index].size = kmmap[source_index].location - (kmmap[source_index-1].location + kmmap[source_index-1].size);
            kmmap_buffer[dest_index].nature = 0;
            kmmap_buffer[dest_index].name = kmmap[free_index].name;
            ++dest_index;
            if(dest_index >= MAX_KMMAP_SIZE) {
              //Memory map overflow. Quitting...
              die(MMAP_TOO_SMALL);
            }
          }
        } else {
          if(kmmap[source_index].location !=
            kmmap[free_index].location)
          {
            kmmap_buffer[dest_index].location = kmmap[free_index].location;
            kmmap_buffer[dest_index].size = kmmap[source_index].location - kmmap[free_index].location;
            kmmap_buffer[dest_index].nature = 0;
            kmmap_buffer[dest_index].name = kmmap[free_index].name;
            ++dest_index;
            if(dest_index >= MAX_KMMAP_SIZE) {
              //Memory map overflow. Quitting...
              die(MMAP_TOO_SMALL);
            }
          }          
        }
        
        //Then add the actual segment
        copy_memory_map_elt(kmmap, kmmap_buffer, source_index, dest_index++);
        if(dest_index >= MAX_KMMAP_SIZE) {
          //Memory map overflow. Quitting...
          die(MMAP_TOO_SMALL);
        }
      }
      
      
      //Fill free space after the end of the last used segment, if needed
      if(source_index != free_index) {
        if(kmmap[source_index].location + kmmap[source_index].size != kmmap[free_index].location + kmmap[free_index].size) {
          kmmap_buffer[dest_index].location = kmmap[source_index].location + kmmap[source_index].size;
          kmmap_buffer[dest_index].size = kmmap[free_index].location + kmmap[free_index].size -
            (kmmap[source_index].location + kmmap[source_index].size);
          kmmap_buffer[dest_index].nature = 0;
          kmmap_buffer[dest_index].name = kmmap[free_index].name;
          ++dest_index;
          if(dest_index >= MAX_KMMAP_SIZE) {
            //Memory map overflow. Quitting...
            die(MMAP_TOO_SMALL);
          }
        }
      } else {
        copy_memory_map_elt(kmmap, kmmap_buffer, free_index, dest_index++);
        if(dest_index >= MAX_KMMAP_SIZE) {
          //Memory map overflow. Quitting...
          die(MMAP_TOO_SMALL);
        }        
      }
    }
  }
  
  copy_memory_map_chunk(kmmap_buffer, kmmap, 0, dest_index);
  kinfo->kmmap_size = dest_index;
  return kmmap_buffer;
}

kernel_memory_map* sort_memory_map(kernel_information* kinfo) {
  //Implemented algorithm : fusion sort
  /* Idea : A E F B D C
       -> Make couples
             AE  FB  DC
       -> Sort couples
             AE  BF  CD
       -> Merge couples (while sorting)
             ABEF    CD
       -> Repeat operation
             ABCDEF */
  
  if(!kinfo) return 0;
  else if((!kinfo->kmmap) || (kinfo->kmmap_size == 0)) return 0;
  
  static kernel_memory_map sorting_buffer[MAX_KMMAP_SIZE];
  unsigned int granularity, left_pointer, right_pointer, dest_pointer = 0, current_pair;
  
  kernel_memory_map* kmmap = (kernel_memory_map*) (uint32_t) kinfo->kmmap;
  kernel_memory_map* source = kmmap;
  kernel_memory_map* dest = sorting_buffer;
  kernel_memory_map* tmp;
  
  /* We use a cyclic behavior : at the beginning of each cycle, we have some amount of
     sorted 2^N-elements lists that we want to merge together */
  for(granularity = 1; granularity < kinfo->kmmap_size; granularity*=2, dest_pointer = 0) {
    /* First, we merge "usual" couples of lists (two elements, each one has size = granularity) */
    for(current_pair = 0, left_pointer = 0, right_pointer = granularity;
      right_pointer + granularity < kinfo->kmmap_size;
      ++current_pair,
      left_pointer = current_pair*2*granularity,
      right_pointer = left_pointer + granularity)
    {
      /* Pick the smallest of each couple, and copy it to the destination.
         If we have AB DC, we copy A, and B DC remains, then we copy B... */
      while((left_pointer < current_pair*2*granularity+granularity) &&
        (right_pointer < (current_pair+1)*2*granularity))
      {        
        if(source[left_pointer].location < source[right_pointer].location) {
          copy_memory_map_elt(source, dest, left_pointer++, dest_pointer++);
        } else {
          copy_memory_map_elt(source, dest, right_pointer++, dest_pointer++);
        }
      }
      
      /* Now, we only have one sorted list of elements left. Let's add it at the
         end of the destination */
      if(left_pointer == current_pair*2*granularity+granularity) {
        while(right_pointer < (current_pair+1)*2*granularity) {
          copy_memory_map_elt(source, dest, right_pointer++, dest_pointer++);
        }
      } else {
        while(left_pointer < current_pair*2*granularity+granularity) {
          copy_memory_map_elt(source, dest, left_pointer++, dest_pointer++);
        }
      }
    }
    
    /* Then we manage the case were there is an isolated list of elements on the right (AB CD EF case)
       or when the rightmost list is smaller than current granularity (AB CD EF G case) */
    //AB CD EF-like case :
    if(right_pointer >= kinfo->kmmap_size) {
      while(left_pointer < kinfo->kmmap_size) {
        copy_memory_map_elt(source, dest, left_pointer++, dest_pointer++);
      }
    } else
    //AB CD EF G-like case : prior merging is necessary
    {
      /* Pick the smallest of each couple, and copy it to the destination.
         If we have AB DC, we copy A, and B DC remains, then we copy B... */
      while((left_pointer < current_pair*2*granularity+granularity) &&
        (right_pointer < kinfo->kmmap_size))
      {        
        if(source[left_pointer].location < source[right_pointer].location) {
          copy_memory_map_elt(source, dest, left_pointer++, dest_pointer++);
        } else {
          copy_memory_map_elt(source, dest, right_pointer++, dest_pointer++);
        }
      }
      
      /* Now, we only have one sorted list of elements left. Let's add it at the
         end of the destination */
      if(left_pointer == current_pair*2*granularity+granularity) {
        while(right_pointer < kinfo->kmmap_size) {
          copy_memory_map_elt(source, dest, right_pointer++, dest_pointer++);
        }
      } else {
        while(left_pointer < current_pair*2*granularity+granularity) {
          copy_memory_map_elt(source, dest, left_pointer++, dest_pointer++);
        }
      }
    }
    
    tmp = source;
    source = dest;
    dest = tmp;
  }
  
  //At this point, the resulting, sorted memory map is stocked in "source"
  //Since this function may be called multiple times, it's best to copy the result in the original buffer.
  if(kmmap!=source) copy_memory_map_chunk(source, kmmap, 0, kinfo->kmmap_size);
  return source;
}
