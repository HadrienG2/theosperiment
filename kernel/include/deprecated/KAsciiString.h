 /* An ASCII string manipulation class, to be removed from future releases

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

#ifndef _KASCIISTRING_H_
#define _KASCIISTRING_H_

#include <address.h>
#include <stdint.h>

class KAsciiString {
  private:
    size_t len;
    char* contents;
    size_t current_location;
  public:
    //Initialization and destruction
    KAsciiString(const char* source = "");
    KAsciiString(const KAsciiString& source);
    ~KAsciiString() {clear();}
    KAsciiString& operator=(const char* source);
    KAsciiString& operator=(const KAsciiString& source);
    void clear(); //Resets a KAsciiString to a blank state

    //Length of the string in ASCII chars
    size_t length() const {return len;}
    void set_length(size_t desired_length, bool keep_contents = true);

    //Comparison
    bool operator==(const char* param) const;
    bool operator==(const KAsciiString& param) const;
    bool operator!=(const char* param) const {return !operator==(param);}
    bool operator!=(const KAsciiString& param) const {return !operator==(param);}

    //Concatenation
    KAsciiString& operator+=(const char* source);
    KAsciiString& operator+=(const KAsciiString& source);
    //No operator+ because it cannot be implemented with good performance in C++

    //Extract a subset of another string into this string
    bool extract_from(KAsciiString& source, size_t first_char, size_t length);

    //Paste a string, or a portion of it, in the middle of another, erasing existing characters
    bool paste(KAsciiString& source, size_t dest_index) {return paste(source, 0, source.length(), dest_index);}
    bool paste(KAsciiString& source, size_t first_char, size_t length, size_t dest_index);

    //Indexed character access
    char& operator[](const uint32_t index) const {return contents[index];}

    //Text file parsing
    bool read_line(KAsciiString& dest); //Returns false if at end of file, true otherwise.
    size_t line_index() {return current_location;}
    void goto_index(size_t index) {current_location = index;}

    //Returns the size of the heap object associated to this string
    size_t heap_size();
};

#endif
