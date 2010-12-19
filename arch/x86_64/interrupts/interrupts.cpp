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
 
#include <interrupts.h>

InterruptManager::InterruptManager() {
  //Generate an almost empty IDT based on two assumptions
  //  1/We'll only use interrupt gates as they are safer
  //  2/Interrupts will be at DPL 0 unless declared otherwise
  unsigned int index;
  for(index=0; index<IDT_LENGTH; ++index) {
    IDT[index][1] = 0;
    IDT[index][0] = 8 << IDTE_CS_OFFSET; //Kernel CS is segment 8
    IDT[index][0] = IDTE_TYPE_IGATE+IDTE_DPL_KERNEL;
  }
}

void InterruptManager::setup_handler(const uint8_t vector, const void* handler) {
  uint64_t address = (uint64_t) handler;
  
  //Remove old address
  IDT[vector][1] = 0;
  IDT[vector][0] = IDT[vector][0] - (IDT[vector][0]&0xffff00000000ffff);
  //Setup new address
  IDT[vector][1] = (address & 0xffffffff00000000) >> 32;
  IDT[vector][0] += (address & 0xffff0000) << 32;
  IDT[vector][0] += (address & 0xffff);
  //Acknowledge that the handler is now present
  if(!(IDT[vector][0] & IDTE_PRESENT)) IDT[vector][0]+=IDTE_PRESENT;
}

void InterruptManager::setup_processor() {
  uint64_t idtr[2];
  
  idtr[1]=(((uint64_t) &IDT) & 0xffff000000000000) >> 48; //Upper 16 bits go in the second part of IDTR
  idtr[0]=(((uint64_t) &IDT) & 0x0000ffffffffffff) << 16;
  idtr[0]+=16*IDT_LENGTH;
  __asm__ volatile ("lidt %0"::"m" (idtr[0]));
}
