 /* Synchronization mechanisms : mutexes, semaphores, and friends...

      Copyright (C) 2010-2011  Hadrien Grasland

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

#include <stdint.h>

class OwnerlessSemaphore8 { //A 8-bit semaphore
  protected:
    volatile uint8_t availability;
    volatile uint8_t max_avl;
  public:
    OwnerlessSemaphore8(const uint8_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint8_t state() {return availability;}
    //Comparing C structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const OwnerlessSemaphore8& param) const;
    bool operator!=(const OwnerlessSemaphore8& param) const {return !(*this==param);}
} __attribute__((packed));

class OwnerlessSemaphore32 {
  protected:
    volatile uint32_t availability;
    volatile uint32_t max_avl;
  public:
    OwnerlessSemaphore32(const uint32_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint32_t state() {return availability;}
    //Comparing C structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const OwnerlessSemaphore32& param) const;
    bool operator!=(const OwnerlessSemaphore32& param) const {return !(*this==param);}
} __attribute__((packed));

class OwnerlessSemaphore64 {
  protected:
    volatile uint64_t availability;
    volatile uint64_t max_avl;
  public:
    OwnerlessSemaphore64(const uint64_t max_users) : availability(max_users), max_avl(max_users) {}
    bool grab_attempt(); //Attempt to grab the semaphore. Return true if successful.
    void grab_spin() {while(!grab_attempt());} //Wait for semaphore availability
    void release() {if(availability < max_avl) ++availability;} //Release the semaphore
    uint64_t state() {return availability;}
    //Comparing C structs is fairly straightforward and should be done by default
    //by the C++ compiler, but well...
    bool operator==(const OwnerlessSemaphore64& param) const;
    bool operator!=(const OwnerlessSemaphore64& param) const {return !(*this==param);}
} __attribute__((packed));

typedef OwnerlessSemaphore64 OwnerlessSemaphore;

class OwnerlessMutex : public OwnerlessSemaphore8 {
  public:
    OwnerlessMutex() : OwnerlessSemaphore8(1) {}
} __attribute__((packed));

#endif
