 /* Synchronization mechanisms : mutexes, semaphores, and friends...

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

#ifndef _SYNCHRONIZATION_H_
#define _SYNCHRONIZATION_H_

#include <hack_stdint.h>

class KernelSemaphore8 { //A 8-bit semaphore
  protected:
    uint8_t availability;
    uint8_t max_avl;
  public:
    KernelSemaphore8(const uint8_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint8_t state() {return availability;}
} __attribute__((packed));

class KernelSemaphore32 {
  protected:
    uint32_t availability;
    uint32_t max_avl;
  public:
    KernelSemaphore32(const uint32_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint32_t state() {return availability;}
} __attribute__((packed));

class KernelSemaphore64 {
  protected:
    uint64_t availability;
    uint64_t max_avl;
  public:
    KernelSemaphore64(const uint64_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint64_t state() {return availability;}
} __attribute__((packed));

typedef KernelSemaphore64 KernelSemaphore;

class KernelMutex : public KernelSemaphore8 {
  public:
    KernelMutex() : KernelSemaphore8(1) {}
} __attribute__((packed));

#endif