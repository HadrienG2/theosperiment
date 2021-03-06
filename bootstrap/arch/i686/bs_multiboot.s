/*  This is assembly code between GRUB and the bootstrap 32-bit kernel,
    whose goal is to prepare launch of the actual long mode kernel.

    Copyright (C) 1999, 2001    Free Software Foundation, Inc.
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

    /* Our stack area. The first and the last page of it should be flagged as non-present */
    .bss
    .globl begin_stack
    .globl end_stack
begin_stack:
    .space 0x100000 /* Initial stack size is 1MB */
end_stack:

    .text
    .globl    bootstrap
bootstrap:
    jmp multiboot_entry

    /* Multiboot header. */
multiboot_header:
    .align 4
    /* magic */
    .long     0x1BADB002
    /* flags */
    .long     0x00000003
    /* checksum */
    .long     -(0x1BADB002 + 0x00000003)
    /* Reserved */
    .long 0x0
    .long 0x0
    .long 0x0
    .long 0x0
    .long 0x0
    /* Graphics mode. 0 = linear graphics
                                        1 = EGA color text */
    .long 0x00000001
    /* Preferred width in pixels/characters of the screen */
    .long 0x00000050
    /* Preferred height in pixels/characters of the screen */
    .long 0x00000019
    /* Color depth of the screen. 0 in text mode */
    .long 0x00000000

multiboot_entry:
    /* Initialize the stack pointer, while leaving one spare page. */
    movl $end_stack, %esp
    sub    $0x1000, %esp

    /* Reset EFLAGS. */
    push $0
    popf

    /* Push the pointer to the Multiboot information structure. */
    push %eax
    /* Push the magic value. */
    push %ebx

    /* Now enter the C main function... */
    call    bootstrap_longmode

loop:
    xchg    %bx, %bx
    jmp     loop
