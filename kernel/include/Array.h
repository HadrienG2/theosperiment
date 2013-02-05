 /* Simple dynamically resizable array for internal use

      Copyright (C) 2013  Hadrien Grasland

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

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <kmath.h>
#include <kstring.h>
#include <new.h>

template <class Item> class Array {
  private:
    size_t heap_length;
    Item* heap;
  public:
    //Blank constructor, copy constructor, and assignment operator
    Array() : heap_length(0), heap(NULL) {}
    Array& operator=(const Array& source) {
        set_length(source.length(), false, true);
        memcpy((void*) heap, (const void*) source.heap, length()*sizeof(Item));
        return *this;
    }
    
    //Array length management
    void clear() {
        heap_length = 0;
        if(heap) {
            delete[] heap;
            heap = NULL;
        }
    }
    size_t length() const { return heap_length; }
    bool set_length(size_t new_length, bool preserve_contents = true, bool force = false) {
        if(heap_length == new_length) return true;
        if(new_length == 0) clear();
        if(!preserve_contents) {
            delete[] heap;
            heap = NULL;
        }
        Item* temp_heap = new(PID_KERNEL, PAGE_FLAGS_RW, force) Item[new_length+1];
        if(temp_heap == NULL) return false;
        heap_length = new_length;
        if(heap != NULL) {
            memcpy((void*) temp_heap, (void*) heap, min(heap_length, new_length)*sizeof(Item));
            delete[] heap;
        }
        heap = temp_heap;
    }
    
    //Array contents access
    Item& operator[](size_t index) const { return heap[index]; }
};

#endif
