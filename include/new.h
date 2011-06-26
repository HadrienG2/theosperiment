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

#include <kmem_allocator.h>

#include <dbgstream.h>

//Operator new and a special version with full mallocator powers
inline void* operator new(const size_t size) throw() {return kalloc(size);}
inline void* operator new(const size_t size,
                          PID target,
                          const VirMemFlags flags = VMEM_FLAGS_RW,
                          const bool force = false) throw() {
    return kalloc(size, target, flags, force);
}

//Array versions of new
inline void* operator new[](const size_t size) throw() {return operator new(size);}
inline void* operator new[](const size_t size,
                            PID target,
                            const VirMemFlags flags = VMEM_FLAGS_RW,
                            const bool force = false) throw() {
    return operator new(size, target, flags, force);
}

//Placement new
inline void* operator new(const size_t, void* place) throw() {return place;}

//Operator delete and, again, the special version
inline void operator delete(void* ptr) throw() {kfree(ptr);}
inline void operator delete(void* ptr, PID target) throw() {kfree(ptr, target);}

//Array versions of delete
inline void operator delete[](void* ptr) throw() {operator delete(ptr);}
inline void operator delete[](void* ptr, PID target) throw() {operator delete(ptr, target);}

#endif
