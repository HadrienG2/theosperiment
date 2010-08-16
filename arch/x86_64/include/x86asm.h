/* Copyright (C) 2010  Hadrien Grasland
   Copyright (C) 2004  All GPL'ed OS
   Copyright (C) 1999  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. 
*/
#ifndef _ASM_H_
#define _ASM_H_

/**
 * x86asm.h
 *
 * Some Intel ASM routines transcribed into C
 */

// CPUID instruction
#define cpuid(code, eax, ebx, ecx, edx) \
  __asm__ volatile ("mov %4, %%eax; \
                     cpuid; \
                     movl %%eax, %0; \
                     movl %%ebx, %1;\
                     movl %%ecx, %2;\
                     movl %%edx, %3"\
                    :"=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx)\
                    :"r"(code)\
                    :"%eax", "%ebx", "%ecx", "%edx")
 
// Write a byte to an I/O port
#define outb(value, port)                                       \
  __asm__ volatile (                                            \
        "outb %b0,%w1"                                          \
        ::"r" (value),"Nd" (port))

// Read a byte from an I/O port
#define inb(port)                                               \
({                                                              \
  unsigned char _v;                                             \
  __asm__ volatile (                                            \
        "inb %w1,%0"                                            \
        :"=r" (_v)                                              \
        :"Nd" (port));                                          \
  _v;                                                           \
})

#endif /* _ASM_H_ */
