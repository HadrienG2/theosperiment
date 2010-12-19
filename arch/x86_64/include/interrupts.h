 /* Everything you need to handle interrupts, both software and IRQs

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

#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include <hack_stdint.h>

//*******************************************************
//********* X86_64-SPECIFIC CONSTANTS AND TYPES *********
//*******************************************************

//Exception vectors
const unsigned int IVEC_DIVIDEBYZERO = 0; //Division by zero
const unsigned int IVEC_DEBUG = 1; //Various debug-oriented exceptions (legacy)
const unsigned int IVEC_NMI = 2; //Non-maskable interrupt from external hardware
const unsigned int IVEC_BREAKPOINT = 3; //Breakpoint exception
const unsigned int IVEC_OVERFLOW = 4; //Overflow in an arithmetic operation
const unsigned int IVEC_BOUNDRANGE = 5; //Index out of bounds of an array (legacy)
const unsigned int IVEC_INVOPCODE = 6; //An invalid opcode was specified
const unsigned int IVEC_DEVNOTAVAILABLE = 7; //Attempt to run an x87 instruction when CR0.TS = 1
const unsigned int IVEC_DOUBLEFAULT = 8; //An exception occurred during handling of another one
const unsigned int IVEC_INVTSS = 10; //An invalid TSS was referenced
const unsigned int IVEC_SEGNOTPRESENT = 11; //Attempt to load a segment with P=0
const unsigned int IVEC_STACK = 12; //Bad stack (non-present, non-canonical, or out of segt limit)
const unsigned int IVEC_GENERALPROTECTION = 13; //Violation of various protection mechanisms
const unsigned int IVEC_PAGEFAULT = 14; //Violation of the page protection mechanism
const unsigned int IVEC_X87EXCEPTION = 16; //x87 floating-point exception
const unsigned int IVEC_ALIGNCHECK = 17; //Unaligned memory reference while alignment checking is on
const unsigned int IVEC_MACHINECHECK = 18; //Machine-check exceptions. Enabled by CR0.MCE
const unsigned int IVEC_SIMDFLOAT = 19; //128-bit media exception
const unsigned int IVEC_SECURITY = 30; //Security-sensitive event in host

const unsigned int IDT_LENGTH = 256; //The length of an interrupt descriptor table
const unsigned int IDTE_TARGET1_LENGTH = 16; //Length of the first part of the handler address
const unsigned int IDTE_CS_OFFSET = 16; //Bit where the CS descriptor begins
const uint64_t IDTE_CS_MASK = 0x00000000ffff0000; //For quick access to the CS descriptor in a IDTe
const unsigned int IDTE_IST_OFFSET = 32; //Bit where the IST descriptor
                                         //(index of the stack to use, in the TSS) begins
const uint64_t IDTE_TYPE_IGATE = 0x00000e0000000000; //Interrupt gate (IF cleared during treatment)
const uint64_t IDTE_TYPE_TGATE = 0x00000f0000000000; //Trap gate (IF NOT cleared)
const uint64_t IDTE_DPL_KERNEL = 0; //Interrupt can only be triggered by kernel or hardware
const uint64_t IDTE_DPL_USER = 0x0000600000000000; //Interrupt can be triggered from user mode
const uint64_t IDTE_PRESENT = 0x0000800000000000; //Interrupt handler is present
const unsigned int IDTE_TARGET2_LEN = 16; //Length of the second part of the handler address.
                                          //Remaining part goes in the second qword of the IDTe
const unsigned int IDTE_TARGET2_OFFSET = 48; //Offset of the second part of the handler.

typedef uint64_t IDTe[2]; //An element of an IDT

//*******************************************************
//************* THE INTERRUPT MANAGER CLASS *************
//*******************************************************

class InterruptManager {
  private:
    IDTe IDT[IDT_LENGTH]; //Our IDT
    uint16_t knlCS; //The kernel code segment
    void setup_handler(const uint8_t vector, const void* handler); //Setup a handler for a specific interrupt
  public:
    //Part which is common to all interrupt managers
    InterruptManager(); //Generates an empty interrupt table
    void setup_processor(); //Setup the processor with our interrupt handlers and enable interrupts
};

#endif