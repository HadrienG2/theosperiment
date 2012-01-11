 /* New and delete implementations based on the kernel memory manager

      Copyright (C) 2011  Hadrien Grasland

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

#ifndef _NEW_H_
#define _NEW_H_

#include <address.h>
#include <kmem_allocator.h>

//State of the new allocator, and functions which alter it
extern unsigned int fake_allocation; //Allocation is not actually performed, instead the number of
                                    //bytes that would have been allocated is returned as the result
inline void start_faking_allocation() {fake_allocation+=1;}
inline void stop_faking_allocation() {if(fake_allocation) fake_allocation-=1;}

//new operator
inline void* operator new(const size_t size,
                          PID target,
                          const VirMemFlags flags = VMEM_FLAGS_RW,
                          const bool force = false) throw() {
    if(!fake_allocation) {
        return kalloc(size, target, flags, force);
    } else {
        return (void*) size;
    }
}
inline void* operator new(const size_t size) throw() {return operator new(size, PID_KERNEL);}

//Array versions of new
inline void* operator new[](const size_t size,
                            PID target,
                            const VirMemFlags flags = VMEM_FLAGS_RW,
                            const bool force = false) throw() {
    return operator new(size, target, flags, force);
}
inline void* operator new[](const size_t size) throw() {return operator new[](size, PID_KERNEL);}

//Placement new
inline void* operator new(const size_t, void* place) throw() {return place;}

//Operator delete and, again, the special version
inline void operator delete(void* ptr, PID target) throw() {kfree(ptr, target);}
inline void operator delete(void* ptr) throw() {operator delete(ptr, PID_KERNEL);}

//Array versions of delete
inline void operator delete[](void* ptr, PID target) throw() {operator delete(ptr, target);}
inline void operator delete[](void* ptr) throw() {operator delete[](ptr, PID_KERNEL);}

#endif
