 /* Functions generating the kernel information structure

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

#include <align.h>
#include <asm_routines.h>
#include <bs_string.h>
#include <die.h>
#include <kinfo_handling.h>
#include <x86asm.h>
#include <x86multiproc.h>

//This variable is use as a buffer for the merge and sort memory map transformations
static KernelMMapItem kmmap_transform_buffer[MAX_KMMAP_SIZE];

KernelMMapItem* add_bios_mmap(KernelMMapItem* kmmap_buffer, bs_size_t *index_ptr, const multiboot_info_t* mbd) {
    if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
    if(*index_ptr>=MAX_KMMAP_SIZE) return 0; //Parameter checking, round 2

    bs_size_t index = *index_ptr;
    if(!(mbd->flags & 64)) return 0; //No memory map without multiboot's help
    //Variables for hacking through the memory map
    int remaining_mmap;
    memory_map_t* current_mmap;
    bs_size_t hack_current_mmap;
    knl_size_t addr64 = 0, size64 = 0, new_addr64;

    //BIOS memory map scanning
    remaining_mmap = mbd->mmap_length;
    current_mmap = mbd->mmap_addr;
    while(remaining_mmap > 0) {
        new_addr64 = current_mmap->base_addr_high;
        new_addr64 *= 0x100000000;
        new_addr64 += current_mmap->base_addr_low;
        if((addr64 + size64 < 0x100000) && (new_addr64 > addr64 + size64)) {
            //Holes in low memory must be memory-mapped ROM. We want this to be accessible to bootstrap code
            kmmap_buffer[index].location = addr64 + size64;
            kmmap_buffer[index].size = new_addr64 - kmmap_buffer[index].location;
            kmmap_buffer[index].nature = NATURE_RES;
            kmmap_buffer[index].name = TO_KNL_PTR("Memory-mapped ROM");
            ++index;
            if(index>=MAX_KMMAP_SIZE) {
                //Not enough room for memory map, quitting...
                die(MMAP_TOO_SMALL);
            }
        }
        addr64 = new_addr64;
        size64 = current_mmap->length_high;
        size64 *= 0x100000000;
                size64 += current_mmap->length_low;
                kmmap_buffer[index].location = addr64;
                kmmap_buffer[index].size = size64;
                if(current_mmap->type == 1) {
            kmmap_buffer[index].nature = NATURE_FRE; //Memory is available
        } else {
            kmmap_buffer[index].nature = NATURE_RES; //Memory is reserved
        }
        if(addr64>=0x100000) {
            kmmap_buffer[index].name = TO_KNL_PTR("High mem");
        } else {
            kmmap_buffer[index].name = TO_KNL_PTR("Low mem");
        }

        //Move to next mmap item
        hack_current_mmap = (bs_size_t) current_mmap;
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

KernelMMapItem* add_bskernel(KernelMMapItem* kmmap_buffer, bs_size_t *index_ptr, const multiboot_info_t* mbd) {
    if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
    if(*index_ptr>=MAX_KMMAP_SIZE) return 0; //Parameter checking, round 2

    bs_size_t index = *index_ptr;
    extern char sbs_kernel;
    extern char ebs_kernel;

    //Add bootstrap kernel location
    kmmap_buffer[index].location = (bs_size_t) &sbs_kernel;
    kmmap_buffer[index].size = (bs_size_t) &ebs_kernel - (bs_size_t) &sbs_kernel;
    kmmap_buffer[index].nature = NATURE_BSK;
    kmmap_buffer[index].name = TO_KNL_PTR("Bootstrap kernel");
    ++index;
    if(index>=MAX_KMMAP_SIZE) {
        //Not enough room for memory map, quitting...
        die(MMAP_TOO_SMALL);
    }

    *index_ptr = index;
    return kmmap_buffer;
}

KernelMMapItem* add_mbdata(KernelMMapItem* kmmap_buffer, bs_size_t *index_ptr, const multiboot_info_t* mbd) {
    if((index_ptr == 0) || (mbd == 0) || (kmmap_buffer == 0)) return 0; //Parameter checking
    if(*index_ptr>=MAX_KMMAP_SIZE) return 0; //Parameter checking, round 2

    bs_size_t index = *index_ptr;

    //We only save string data from GRUB. All useful information in the rest has been converted to
    //kernel information when necessary.

    //Command line
    if(mbd->flags & 4) {
        kmmap_buffer[index].location = (bs_size_t) mbd->cmdline;
        kmmap_buffer[index].size = strlen(mbd->cmdline)+1;
        kmmap_buffer[index].nature = NATURE_BSK;
        kmmap_buffer[index].name = TO_KNL_PTR("Kernel command line");
        ++index;
        if(index>=MAX_KMMAP_SIZE) {
            //Not enough room for memory map, quitting...
            die(MMAP_TOO_SMALL);
        }
    }

    //Save module strings
    if(mbd->flags & 8) {
        bs_size_t current_mod;

        for(current_mod = 0; current_mod < mbd->mods_count; ++current_mod) {
            kmmap_buffer[index].location = (bs_size_t) (mbd->mods_addr[current_mod].string);
            kmmap_buffer[index].size = strlen(mbd->mods_addr[current_mod].string)+1;
            kmmap_buffer[index].nature = NATURE_BSK;
            kmmap_buffer[index].name = TO_KNL_PTR("Multiboot modules string");
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

void add_modules(KernelInformation* kinfo, const multiboot_info_t* mbd) {
    bs_size_t current_mod = 0;

    //Add module information only if it is present (it should be)
    if(mbd->flags & 8) {
        while(current_mod < mbd->mods_count) {
            kmmap_add(kinfo,
                      mbd->mods_addr[current_mod].mod_start,
                      mbd->mods_addr[current_mod].mod_end-mbd->mods_addr[current_mod].mod_start+1,
                      NATURE_MOD,
                      mbd->mods_addr[current_mod].string);

            //Move to next module
            ++current_mod;
        }
    }
    kmmap_update(kinfo);
}

KernelMMapItem* copy_memory_map_chunk(const KernelMMapItem* source,
                                       KernelMMapItem* dest,
                                       const bs_size_t start,
                                       const bs_size_t length) {
    if((!source) || (!dest) || (length+start > MAX_KMMAP_SIZE)) {
        return 0;
    }

    bs_size_t i;
    for(i=start; i<start+length; ++i) copy_memory_map_elt(source, dest, i, i);

    return dest;
}

KernelMMapItem* copy_memory_map_elt(const KernelMMapItem* source,
                                     KernelMMapItem* dest,
                                     const bs_size_t source_index,
                                     const bs_size_t dest_index) {
    if((!source) || (!dest) || (source_index >= MAX_KMMAP_SIZE) || (dest_index >= MAX_KMMAP_SIZE)) {
        return 0;
    }

    dest[dest_index].location = source[source_index].location;
    dest[dest_index].size = source[source_index].size;
    dest[dest_index].nature = source[source_index].nature;
    dest[dest_index].name = source[source_index].name;

    return dest;
}

KernelMMapItem* find_freemem(const KernelInformation* kinfo, const knl_size_t minimal_size) {
    bs_size_t index;
    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);

    //Find the beginning of high memory (>1MB)
    for(index=0; kmmap[index].location<0x100000; ++index);
    //Find a suitable free chunk of high memory
    for(; (kmmap[index].size < minimal_size) || (kmmap[index].nature!=NATURE_FRE); ++index);

    return (KernelMMapItem*) &(kmmap[index]);
}

KernelMMapItem* find_freemem_pgalign(const KernelInformation* kinfo, const knl_size_t minimal_size) {
    bs_size_t index;
    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);

    //Find the beginning of high memory (>1MB)
    for(index=0; kmmap[index].location<0x100000; ++index);
    //Find a suitable free chunk of high memory
    for(; (kmmap[index].size < minimal_size + (align_pgup(kmmap[index].location)-kmmap[index].location))
                     || (kmmap[index].nature!=NATURE_FRE); ++index);

    return (KernelMMapItem*) &(kmmap[index]);
}

KernelCPUInfo* generate_cpu_info(KernelInformation* kinfo) {
    uint32_t max_std_function, max_ext_function, eax, ebx, ecx, edx;

    //Check CPUID support by trying to toggle bit 21 (ID) in EFLAGS.
    //If CPU does not support CPUID, it won't support long mode either
    if(!cpuid_check()) die(INADEQUATE_CPU);

    //Get max accessible standard CPUID function.
    cpuid(0,eax,ebx,ecx,edx);
    max_std_function=eax;

    //CPUID function 1 must be supported for long mode support to be available
    if(max_std_function==0) die(INADEQUATE_CPU);

    //Get the information of CPUID standard function 1
    cpuid(1,eax,ebx,ecx,edx);
    if(edx&0x2000) { //Global page support (enable if available)
        __asm__ volatile("mov %%cr4, %%eax; bts $7, %%eax; mov %%eax, %%cr4":::"%eax");
    }

    //If CPU does not support PAE or APIC, it won't support long mode either.
    //We also require SSE2 support, since every AMD64 CPU has it.
    if(!(edx&0x4000240)) die(INADEQUATE_CPU);

    //Now, we're going to examine CPUID's extended fields. Note that those may vary from
    //one vendor to another.
    cpuid(0x80000000,eax,ebx,ecx,edx);
    max_ext_function=eax;

    //If CPU does not support CPUID function 0x80000001, it does not support long mode
    if(max_ext_function==0x80000000) die(INADEQUATE_CPU);

    //Get information from CPUID's extended function 0x80000001
    cpuid(0x80000001,eax,ebx,ecx,edx);

    //If CPU does not support long mode and NX/DEP, it doesn't fit our needs
    if(!(edx&0x20100000)) die(INADEQUATE_CPU);

    //A separate function takes care of the multiprocessing thing, as it is... say...
    //more complicated than it could be.
    return generate_multiprocessing_info(kinfo);
}

KernelMMapItem* generate_memory_map(const multiboot_info_t* mbd, KernelInformation* kinfo) {
    //A buffer for memory map.
    static KernelMMapItem kmmap_buffer[MAX_KMMAP_SIZE];
    bs_size_t index = 0;

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
    if(add_bios_mmap(kmmap_buffer, &index, mbd) == 0) die(NO_MEMORYMAP);
    if(add_mbdata(kmmap_buffer, &index, mbd) == 0) die(NO_MEMORYMAP);
    if(add_bskernel(kmmap_buffer, &index, mbd) == 0) die(NO_MEMORYMAP);
    kinfo->kmmap = TO_KNL_PTR(kmmap_buffer);
    kinfo->kmmap_size = index;

    //Here's part 2
    if(sort_memory_map(kinfo) == 0) die(NO_MEMORYMAP);

    //Here's part 3
    if(merge_memory_map(kinfo) == 0) die(NO_MEMORYMAP);

    //Finally, protect the first page of memory from tampering by marking it as reserved
    kmmap_add(kinfo, 0, PG_SIZE, NATURE_RES, "NULL pointer's page");
    kmmap_update(kinfo);

    return kmmap_buffer;
}

KernelCPUInfo* generate_multiprocessing_info(KernelInformation* kinfo) {
    mp_floating_ptr* floating_ptr;
    mp_config_table_hdr* mpconfig_hdr;
    mpconfig_proc_entry* proc_entry;
    int current_entry, core_amount = 0;

    floating_ptr = find_fptr();
    if(!floating_ptr) {
        //No floating pointer. Assume the system is monoprocessor.
        kinfo->cpu_info.core_amount = 1;
        return &(kinfo->cpu_info);
    }

    //Are we, perchance, using any default 2-processor configuration ?
    if(floating_ptr->mp_sys_config_type) {
        kinfo->cpu_info.core_amount = 2;
        return &(kinfo->cpu_info);
    }

    //Otherwise, check the MP configuration table before opening it...
    mpconfig_hdr = mpconfig_check(floating_ptr);
    if(!mpconfig_hdr) {
        //The MP configuration table is incorrect. Better not use it and assume the system is monoprocessor.
        kinfo->cpu_info.core_amount = 1;
        return &(kinfo->cpu_info);
    }

    //Parse MP configuration table, looking for processors...
    proc_entry = (mpconfig_proc_entry*) (mpconfig_hdr+1);
    for(current_entry=0; current_entry<mpconfig_hdr->entry_count; ++current_entry, ++proc_entry) {
        if(proc_entry->entry_type!=0) break; //We went past processor entries region.
        if(proc_entry->cpu_flags & 1) ++core_amount;
    }

    //Store core amount in the kernel information structure.
    kinfo->cpu_info.core_amount = core_amount;

    return &(kinfo->cpu_info);
}

KernelInformation* kinfo_gen(const multiboot_info_t* mbd) {
    //Some buffers
    static KernelInformation result;
    static StartupDriveInfo sd_buff;

    //Let's say we've nothing available in the beginning
    result.command_line = 0;
    result.kmmap = 0;
    result.kmmap_size = 0;
    result.arch_info.startup_drive = 0;

    //Get command line from bootloader (if available)
    if(mbd->flags & 4) result.command_line = TO_KNL_PTR(mbd->cmdline);
    //Same for startup drive
    if(mbd->flags & 2) {
        sd_buff.drive_number = mbd->boot_device/(256*256*256);
        sd_buff.partition_number = (mbd->boot_device/(256*256))%256;
        sd_buff.sub_partition_number = (mbd->boot_device/256)%256;
        sd_buff.subsub_partition_number = mbd->boot_device%256;
    }
    result.arch_info.startup_drive = TO_KNL_PTR(&sd_buff);
    //Generate a memory map
    generate_memory_map(mbd, &result);
    //Generate module-related information
    add_modules(&result, mbd);
    //Gather CPU information
    generate_cpu_info(&result);

    return &result;
}

void kmmap_add(KernelInformation* kinfo,
                             const knl_size_t location,
                             const knl_size_t size,
                             const uint8_t nature,
                             const char* name) {
    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);

    if(++(kinfo->kmmap_size) == MAX_KMMAP_SIZE) die(MMAP_TOO_SMALL);
    kmmap[kinfo->kmmap_size-1].location = location;
    kmmap[kinfo->kmmap_size-1].size = size;
    kmmap[kinfo->kmmap_size-1].nature = nature;
    kmmap[kinfo->kmmap_size-1].name = TO_KNL_PTR(name);
}

knl_size_t kmmap_alloc(KernelInformation* kinfo, const knl_size_t size, const uint8_t nature, const char* name) {
    KernelMMapItem* kmmap_free;

    kmmap_free = find_freemem(kinfo, size);
    kmmap_add(kinfo, kmmap_free->location, size, nature, name);
    kmmap_update(kinfo);
    return kmmap_free->location;
}

knl_size_t kmmap_alloc_pgalign(KernelInformation* kinfo, const knl_size_t size, const uint8_t nature, const char* name) {
    KernelMMapItem* kmmap_free;
    knl_size_t location;

    kmmap_free = find_freemem_pgalign(kinfo, align_pgup(size));
    location = align_pgup(kmmap_free->location);
    kmmap_add(kinfo, location, align_pgup(size), nature, name);
    kmmap_update(kinfo);
    return location;
}

//void kmmap_free_pgalign(KernelInformation* kinfo, const bs_size_t index) {
    //KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);
    
    //TODO : COMPLETE THIS JOB !
//}

knl_size_t kmmap_mem_amount(const KernelInformation* kinfo) {
    const KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);

    knl_size_t memory_amount = kmmap[kinfo->kmmap_size-1].location+kmmap[kinfo->kmmap_size-1].size;
    #ifdef DEBUG
        //x86 reserves a small amount of memory around the end of the adressable space.
        //I don't use it at the moment, and it slows Bochs down a lot. Therefore, it should not be
        //mapped during the testing period.
        if(memory_amount==0x100000000) memory_amount = kmmap[kinfo->kmmap_size-2].location+kmmap[kinfo->kmmap_size-2].size;
    #endif
    return memory_amount;
}

void kmmap_update(KernelInformation* kinfo) {
    sort_memory_map(kinfo);
    merge_memory_map(kinfo);
}

KernelMMapItem* merge_memory_map(KernelInformation* kinfo) {
    /* Goal of this function : parts of the memory map (segments) are overlapping, due to the fact that
       memory map provided by GRUB does not take account of used memory, only knowing the difference
       between reserved ram and "free" ram (which, in fact, isn't necessarily free).

       We want to merge overlapping parts and get a flat map (something like...
                  AAAAAAAAAAA
                      BBBB
                                      CCC
               => AAAABBBBAAA         CCC )

     Simplifications :
        -> Two segments overlapping simulatneously as a maximum, one of them being free memory
           and the other being some kind of used memory region.
        -> Free memory segments always begin before and end after the overlapping segments. */

    KernelMMapItem tmp = {0,0,0,0};
    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);
    bs_size_t dest_index = 0, source_index, free_index;

    /*  It's simpler if the free memory segments are listed before their used counterpart in
        main memory. The only case where this may not happen is when a used segment begins at the same
        address as a non-free one, so it does rarely happen. We'll take care of that. */
    for(source_index = 0; source_index < kinfo->kmmap_size; ++source_index) {
        if(kmmap[source_index].nature == NATURE_FRE && source_index != 0) {
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
        //Is this a busy segment ?
        if(kmmap[source_index].nature != NATURE_FRE) {
            copy_memory_map_elt(kmmap, kmmap_transform_buffer, source_index, dest_index++);
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
                        kmmap_transform_buffer[dest_index].location =    kmmap[source_index-1].location + kmmap[source_index-1].size;
                        kmmap_transform_buffer[dest_index].size = kmmap[source_index].location - (kmmap[source_index-1].location + kmmap[source_index-1].size);
                        kmmap_transform_buffer[dest_index].nature = NATURE_FRE;
                        kmmap_transform_buffer[dest_index].name = kmmap[free_index].name;
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
                        kmmap_transform_buffer[dest_index].location = kmmap[free_index].location;
                        kmmap_transform_buffer[dest_index].size = kmmap[source_index].location - kmmap[free_index].location;
                        kmmap_transform_buffer[dest_index].nature = NATURE_FRE;
                        kmmap_transform_buffer[dest_index].name = kmmap[free_index].name;
                        ++dest_index;
                        if(dest_index >= MAX_KMMAP_SIZE) {
                            //Memory map overflow. Quitting...
                            die(MMAP_TOO_SMALL);
                        }
                    }
                }

                //Then add the actual segment
                copy_memory_map_elt(kmmap, kmmap_transform_buffer, source_index, dest_index++);
                if(dest_index >= MAX_KMMAP_SIZE) {
                    //Memory map overflow. Quitting...
                    die(MMAP_TOO_SMALL);
                }
            }


            //Fill free space after the end of the last used segment, if needed
            if(source_index != free_index) {
                if(kmmap[source_index].location + kmmap[source_index].size != kmmap[free_index].location + kmmap[free_index].size) {
                    kmmap_transform_buffer[dest_index].location = kmmap[source_index].location + kmmap[source_index].size;
                    kmmap_transform_buffer[dest_index].size = kmmap[free_index].location + kmmap[free_index].size -
                        (kmmap[source_index].location + kmmap[source_index].size);
                    kmmap_transform_buffer[dest_index].nature = NATURE_FRE;
                    kmmap_transform_buffer[dest_index].name = kmmap[free_index].name;
                    ++dest_index;
                    if(dest_index >= MAX_KMMAP_SIZE) {
                        //Memory map overflow. Quitting...
                        die(MMAP_TOO_SMALL);
                    }
                }
            } else {
                copy_memory_map_elt(kmmap, kmmap_transform_buffer, free_index, dest_index++);
                if(dest_index >= MAX_KMMAP_SIZE) {
                    //Memory map overflow. Quitting...
                    die(MMAP_TOO_SMALL);
                }
            }
        }
    }

    copy_memory_map_chunk(kmmap_transform_buffer, kmmap, 0, dest_index);
    kinfo->kmmap_size = dest_index;
    return kmmap;
}

KernelMMapItem* sort_memory_map(KernelInformation* kinfo) {
    //Implemented algorithm : fusion sort
    /* Idea : A E F B D C
             -> Make couples
                         AE    FB    DC
             -> Sort couples
                         AE    BF    CD
             -> Merge couples (while sorting)
                         ABEF        CD
             -> Repeat operation
                         ABCDEF */

    if(!kinfo) return 0;
    else if((!kinfo->kmmap) || (kinfo->kmmap_size == 0)) return 0;

    bs_size_t granularity, left_pointer, right_pointer, dest_pointer = 0, current_pair;

    KernelMMapItem* kmmap = FROM_KNL_PTR(KernelMMapItem*, kinfo->kmmap);
    KernelMMapItem* source = kmmap;
    KernelMMapItem* dest = kmmap_transform_buffer;
    KernelMMapItem* tmp;

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
