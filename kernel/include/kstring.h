 /* Features basic C string and RAM manipulation, in the spirit of cstring,
    along with an implementation of Unicode 6.2.0 strings in UTF-32 and UTF-8 form.

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

#ifndef _KSTRING_H_
#define _KSTRING_H_

#include <address.h>
#include <stdint.h>

//These are functions for the manipulation of legacy ASCII strings and other kinds of
//low-level memory manipulation
void* memcpy(void* destination, const void* source, size_t num);
size_t strlen(const char* str);


//These support structures are used internally by Unicode string routines
//They store enough information to compute a characters' NFD decomposition
//They are filled by the InitializeKString() function from information extracted
//out of the Unicode database.
struct CombiningClassMapping {
    uint32_t codepoint;
    uint8_t combining_class;
};

struct CombiningClassDB {
    size_t table_length;
    CombiningClassMapping* table;
};

extern CombiningClassDB* combining_class_db;

struct CanonicalDecompositionMapping {
    uint32_t codepoint;
    uint32_t canonical_decomposition[2];
};

struct CanonicalDecompositionDB {
    size_t table_length;
    CanonicalDecompositionMapping* table;
};

extern CanonicalDecompositionDB* canonical_decomposition_db;

void InitializeUnicodeSupport();

#endif
