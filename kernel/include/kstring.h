 /* Equivalent of cstring (aka string.h), but not a full implementation. Manipulates chunks of RAM,
    and maybe C-style strings and array in the future.
    Also includes definition of the KAsciiString object, a high-level string container used for
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
size_t strlen(const char* str);

typedef class KAsciiString KString;

class KAsciiString { //TO BE REMOVED
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

    //Extract of a subset of another string into this string
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

typedef class KUTF32String KUTF32String;
typedef class KUTF8String KUTF8String;

class KUTF32String {
  private:
    size_t len;
    uint32_t* contents;
    size_t current_location;
  public:
    //Initialization and destruction
    KUTF32String(const KUTF32String& source);
    KUTF32String(const KUTF8String& source);
    KUTF32String(const char32_t* source);
    KUTF32String(const char* source = ""); //For use with ASCII strings only (Is it possible to programmatically differentiate those ?)
    KUTF32String& operator=(const KUTF32String& source);
    KUTF32String& operator=(const KUTF8String& source);
    KUTF32String& operator=(const char32_t* source);
    KUTF32String& operator=(const char* source); //For use with ASCII strings only
    ~KUTF32String() {clear();}
    void clear(); //Resets a KUTF32String to a blank state

    //Length of the string in Unicode code points
    size_t length() const {return len;}
    void set_length(size_t desired_length, bool keep_contents = true);

    //String comparison
    bool operator==(const KUTF32String& param) const;
    bool operator!=(const KUTF32String& param) const {return !operator==(param);}

    //String concatenation
    KUTF32String& operator+=(const KUTF32String& source);
    //No operator+ because it cannot be implemented efficiently in C++

    //Extract of a subset of another string into this string
    bool extract_from(KUTF32String& source, size_t first_char, size_t length);

    //Paste a string, or a portion of it, in the middle of another, erasing existing characters.
    //Leaving length to zero means that the whole source string must be pasted.
    //WARNING : This is semantically different from KAsciiString's paste() !
    bool paste(KUTF32String& source, size_t dest_index, size_t first_char = 0, size_t length = 0);

    //Indexed character access
    uint32_t& operator[](const size_t index) const {return contents[index];}

    //Text file parsing
    bool read_line(KUTF32String& dest); //Returns false if at end of file, true otherwise.
    size_t line_index() {return current_location;}
    void seek_index(size_t index) {current_location = index;}

    //Returns the size of the heap objects associated to this string
    size_t heap_size();
};

class KUTF8String {
  private:
    size_t len;
    uint8_t* contents;
  public:
    //Initialization and destruction
    KUTF8String(const KUTF8String& source);
    KUTF8String(const KUTF32String& source);
    KUTF8String(const char* source = ""); //For use with UTF-8 strings only (Is it possible to programmatically differentiate those ?)
    KUTF8String& operator=(const KUTF8String& source);
    KUTF8String& operator=(const KUTF32String& source);
    KUTF8String& operator=(const char* source); //For use with UTF-8 strings only
    ~KUTF8String() {clear();}
    void clear(); //Resets a KUTF8String to a blank state
    
    //Length of the string in bytes (which is NOT the same as code points)
    size_t length() const {return len;}
    void set_length(size_t desired_length, bool keep_contents = true);
    
    //Individual byte access
    uint8_t& operator[](const size_t index) const {return contents[index];}
    
    //Returns the size of the heap objects associated to this string
    size_t heap_size();
};

#endif
