 /* Equivalent of cstring (aka string.h), but not a full implementation. Features basic
    C string and RAM manipulation, along with an implementation of UTF-32 and UTF-8 strings
    complying with Unicode version 6.2.0

    Copyright (C) 2011-2013  Hadrien Grasland

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

#include <kstring.h>

#include <dbgstream.h>

void* memcpy(void* destination, const void* source, size_t num) {
    const uint8_t* new_source = (const uint8_t*) source;
    uint8_t* new_dest = (uint8_t*) destination;

    for(size_t i=0; i<num; ++i) new_dest[i] = new_source[i];

    return destination;
}

size_t strlen(const char* str) {
    size_t len;
    for(len = 0; str[len]!='\0'; ++len);
    return len;
}

//These private types are defined as permanent storage location for binary Unicode data
CombiningClassDB* combining_class_db = NULL;
CanonicalDecompositionDB* canonical_decomposition_db = NULL;

void InitializeUnicodeSupport() {
    //TODO : Brainstorm ways to probe kernel modules without global variables, is possible
    dbgout << "Error: InitializeKString() is not implemented yet !" << endl;
}
