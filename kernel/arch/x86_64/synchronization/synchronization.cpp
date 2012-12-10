 /* Synchronization mechanisms : mutexes, semaphores, and friends

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

#include <synchronization.h>

bool OwnerlessSemaphore8::grab_attempt() {
  uint8_t availability_before;
  uint8_t availability_after;
  uint8_t real_availability;

  //Make a copy of semaphore availability
  availability_before = availability;
  //If semaphore was available at copy time, generate new semaphore availability.
  //If not, operation has failed.
  if(!availability_before) return false;
  availability_after = availability_before-1;
  //Check if semaphore availability is still the same. If it does, operation is successful :
  //set new availability. If not, operation has failed.
  __asm__ volatile ("movb %2, %%al;\
                     lock cmpxchgb %%bl, %0"
                   :"=m" (availability), "=a" (real_availability)
                   :"m" (availability_before), "b" (availability_after));
  if(real_availability==availability_before) return true;
  else return false;
}

bool OwnerlessSemaphore8::operator==(const OwnerlessSemaphore8& param) const {
    if(availability != param.availability) return false;
    if(max_avl != param.max_avl) return false;
    return true;
}

bool OwnerlessSemaphore32::grab_attempt() {
  uint32_t availability_before;
  uint32_t availability_after;
  uint32_t real_availability;

  //Make a copy of semaphore availability
  availability_before = availability;
  //If semaphore was available at copy time, generate new semaphore availability.
  //If not, operation has failed.
  if(!availability_before) return false;
  availability_after = availability_before-1;
  //Check if semaphore availability is still the same. If it does, operation is successful :
  //set new availability. If not, operation has failed.
  __asm__ volatile ("movl %2, %%eax;\
                     lock cmpxchgl %%ebx, %0"
                   :"=m" (availability), "=a" (real_availability)
                   :"m" (availability_before), "b" (availability_after));
  if(real_availability==availability_before) return true;
  else return false;
}

bool OwnerlessSemaphore32::operator==(const OwnerlessSemaphore32& param) const {
    if(availability != param.availability) return false;
    if(max_avl != param.max_avl) return false;
    return true;
}

bool OwnerlessSemaphore64::grab_attempt() {
  uint64_t availability_before;
  uint64_t availability_after;
  uint64_t real_availability;

  //Make a copy of semaphore availability
  availability_before = availability;
  //If semaphore was available at copy time, generate new semaphore availability.
  //If not, operation has failed.
  if(!availability_before) return false;
  availability_after = availability_before-1;
  //Check if semaphore availability is still the same. If it does, operation is successful :
  //set new availability. If not, operation has failed.
  __asm__ volatile ("movq %2, %%rax;\
                     lock cmpxchgq %%rbx, %0"
                   :"=m" (availability), "=a" (real_availability)
                   :"m" (availability_before), "b" (availability_after));
  if(real_availability==availability_before) return true;
  else return false;
}

bool OwnerlessSemaphore64::operator==(const OwnerlessSemaphore64& param) const {
    if(availability != param.availability) return false;
    if(max_avl != param.max_avl) return false;
    return true;
}
