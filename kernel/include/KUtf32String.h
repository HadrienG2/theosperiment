 /* An implementation of NFD-normalized Unicode 6.2.0 strings in UTF-32 representation,
    for the purpose of internal text processing.

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

#ifndef _KUTF32STRING_H_
#define _KUTF32STRING_H_

#include <address.h>
#include <Array.h>
#include <KUtf8String.h>
#include <stdint.h>

typedef class KUtf8String KUtf8String;

class KUtf32String {
  private:
    Array<uint32_t> contents;
    size_t file_index;
  public:
    //Initialization and destruction
    KUtf32String() : file_index(0) {}
    KUtf32String(const KUtf32String& source);
    KUtf32String(KUtf8String& source);
    KUtf32String(const char32_t* source); //TO BE IMPLEMENTED
    KUtf32String(const char* source); //For use with Latin-x ASCII strings only
    KUtf32String& operator=(const KUtf32String& source); //TO BE IMPLEMENTED
    KUtf32String& operator=(const KUtf8String& source); //TO BE IMPLEMENTED
    KUtf32String& operator=(const char32_t* source); //TO BE IMPLEMENTED
    KUtf32String& operator=(const char* source); //For use with Latin-x ASCII strings only //TO BE IMPLEMENTED
    ~KUtf32String() {clear();}
    void clear(); //Resets a KUtf32String to a blank state

    //Length of the string in Unicode code points
    size_t length() const {return contents.length();}
    void set_length(size_t desired_length, bool preserve_contents = true); //TO BE IMPLEMENTED
    
    //NFD normalization : replace characters by their canonical decomposition.
    //This operation is to be carried out on any external input, and relies on external
    //databases that must be updated as new versions of the Unicode standard are supported
    void normalize_to_nfd(); //TO BE IMPLEMENTED
    
    //Indexed character access
    uint32_t& operator[](const size_t index) const {return contents[index];}

    //String comparison
    bool operator==(const KUtf32String& param) const; //TO BE IMPLEMENTED
    bool operator!=(const KUtf32String& param) const {return !operator==(param);}

    //String concatenation
    //
    //IMPLEMENTATION WARNING : Must keep strings NFD-normalized
    KUtf32String& operator+=(const KUtf32String& source); //TO BE IMPLEMENTED
    //No operator+ because it cannot be implemented efficiently in C++

    //Extract a subset of another string into this string
    bool extract_from(KUtf32String& source, size_t first_char, size_t length); //TO BE IMPLEMENTED

    //Paste a string, or a portion of it, in the middle of this one, erasing existing characters.
    //Leaving length to zero means that the whole source string must be pasted.
    //
    //IMPLEMENTATION WARNING : Parameter order is different from KAsciiString's paste() !
    //IMPLEMENTATION WARNING : Must keep strings NFD-normalized
    bool paste(KUtf32String& source, size_t dest_index, size_t first_char = 0, size_t length = 0); //TO BE IMPLEMENTED

    //Text file parsing
    bool read_line(KUtf32String& dest); //Returns false if at end of file, true otherwise. //TO BE IMPLEMENTED
    size_t line_index() {return file_index;}
    void seek_line_index(size_t index) {file_index = index;}

    //Returns the size of the heap objects associated to this string
    size_t heap_size(); //TO BE IMPLEMENTED
};

#endif
