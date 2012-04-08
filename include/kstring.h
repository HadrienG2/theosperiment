 /* Equivalent of cstring (aka string.h), but not a full implementation. Manipulates chunks of RAM,
    and maybe C-style strings and array in the future.
    Also includes definition of the KString object, a high-level string container used for
    communication between the kernel and processes.

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

#ifndef _KSTRING_H_
#define _KSTRING_H_

#include <address.h>

void* memcpy(void* destination, const void* source, size_t num);
uint32_t strlen(const char* str);

class KString {
  private:
    char* contents;
    uint32_t len;
  public:
    //KString construction and destruction
    KString(const char* source = "");
    KString(const KString& source);
    ~KString();
    KString& operator=(const char* source);
    KString& operator=(const KString& source);

    //KString concatenation
    KString& operator+=(const char* source);
    KString& operator+=(const KString& source);
    //No operator+. It can be implemented very easily, but not with good performance (when doing
    //a = b+c, a new object is created, only to be copied to a and destroyed shortly thereafter)

    //KString comparison
    bool operator==(const char* param) const;
    bool operator==(const KString& param) const;
    bool operator!=(const char* param) const {return !operator==(param);}
    bool operator!=(const KString& param) const {return !operator==(param);}

    //KString indexed access
    char& operator[](const uint32_t index) const {return contents[index];}

    //Length access
    uint32_t length() const {return len;}

    //Returns the size of the heap object associated to this string
    size_t heap_size();
};

#endif
