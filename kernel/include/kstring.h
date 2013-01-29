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

void* memcpy(void* destination, const void* source, size_t num);
size_t strlen(const char* str);

typedef class KUTF32String KUTF32String;
typedef class KUTF8String KUTF8String;

class KUTF32String { //An NFD-normalized UTF-32 string
  private:
    size_t len;
    uint32_t* contents;
    size_t file_index;
  public:
    //Initialization and destruction
    KUTF32String(); //TO BE IMPLEMENTED
    KUTF32String(const KUTF32String& source);
    KUTF32String(KUTF8String& source);
    KUTF32String(const char32_t* source); //TO BE IMPLEMENTED
    KUTF32String(const char* source); //For use with ASCII strings only //TO BE IMPLEMENTED
    KUTF32String& operator=(const KUTF32String& source); //TO BE IMPLEMENTED
    KUTF32String& operator=(const KUTF8String& source); //TO BE IMPLEMENTED
    KUTF32String& operator=(const char32_t* source); //TO BE IMPLEMENTED
    KUTF32String& operator=(const char* source); //For use with ASCII strings only //TO BE IMPLEMENTED
    ~KUTF32String() {clear();}
    void clear(); //Resets a KUTF32String to a blank state //TO BE IMPLEMENTED

    //Length of the string in Unicode code points
    size_t length() const {return len;}
    void set_length(size_t desired_length, bool keep_contents = true); //TO BE IMPLEMENTED
    
    //NFD normalization : replace characters by their canonical decomposition.
    //This operation is to be carried out on any external input, and relies on external
    //databases that must be updated as new versions of the Unicode standard are supported
    void normalize_to_nfd(); //TO BE IMPLEMENTED
    
    //Indexed character access
    uint32_t& operator[](const size_t index) const {return contents[index];}

    //String comparison
    bool operator==(const KUTF32String& param) const; //TO BE IMPLEMENTED
    bool operator!=(const KUTF32String& param) const {return !operator==(param);}

    //String concatenation
    //
    //IMPLEMENTATION WARNING : Must keep strings NFD-normalized
    KUTF32String& operator+=(const KUTF32String& source); //TO BE IMPLEMENTED
    //No operator+ because it cannot be implemented efficiently in C++

    //Extract a subset of another string into this string
    bool extract_from(KUTF32String& source, size_t first_char, size_t length); //TO BE IMPLEMENTED

    //Paste a string, or a portion of it, in the middle of this one, erasing existing characters.
    //Leaving length to zero means that the whole source string must be pasted.
    //
    //IMPLEMENTATION WARNING : Parameter order is different from KAsciiString's paste() !
    //IMPLEMENTATION WARNING : Must keep strings NFD-normalized
    bool paste(KUTF32String& source, size_t dest_index, size_t first_char = 0, size_t length = 0); //TO BE IMPLEMENTED

    //Text file parsing
    bool read_line(KUTF32String& dest); //Returns false if at end of file, true otherwise. //TO BE IMPLEMENTED
    size_t line_index() {return file_index;}
    void seek_line_index(size_t index) {file_index = index;}

    //Returns the size of the heap objects associated to this string
    size_t heap_size(); //TO BE IMPLEMENTED
};

struct KUTF8CodePoint {
    uint32_t code_point;
    size_t byte_length;
    operator uint32_t&() {return code_point;}
    KUTF8CodePoint& operator=(uint32_t value) {code_point = value; return *this;}
};

class KUTF8String {
  private:
    size_t len;
    uint8_t* contents;
  public:
    //Initialization and destruction
    KUTF8String(); //TO BE IMPLEMENTED
    KUTF8String(const KUTF8String& source);  //TO BE IMPLEMENTED
    KUTF8String(const KUTF32String& source);  //TO BE IMPLEMENTED
    KUTF8String(const char* source); //For use with UTF-8 strings only (Is it possible to programmatically differentiate those ?)  //TO BE IMPLEMENTED
    KUTF8String& operator=(const KUTF8String& source);  //TO BE IMPLEMENTED
    KUTF8String& operator=(const KUTF32String& source);  //TO BE IMPLEMENTED
    KUTF8String& operator=(const char* source); //For use with UTF-8 strings only  //TO BE IMPLEMENTED
    ~KUTF8String() {clear();}
    void clear(); //Resets a KUTF8String to a blank state  //TO BE IMPLEMENTED
    
    //Individual byte access
    uint8_t& operator[](const size_t index) const {return contents[index];}
    
    //Length of the string in bytes and code points (note : ill-formed sequences are treated as a single 0xFFFD code point)
    size_t byte_length() const {return len;}
    void set_byte_length(size_t desired_length, bool keep_contents = true);  //TO BE IMPLEMENTED
    size_t codepoint_length() const;
    
    //Attempt to read a code point located at a user-specified location.
    //In the returned structure, code_point is set to * 0 at or after the end of string
    //                                                * 0xFFFD for invalid characters
    //                                                * The parsed code point otherwise
    //and byte_length is sent to the total length of the detected code point in bytes, or 0 if no code point has been read.
    KUTF8CodePoint peek_codepoint(const size_t index) const;
    
    //Returns the size of the heap objects associated to this string
    size_t heap_size();  //TO BE IMPLEMENTED
};

//These support structures are used internally by Unicode manipulation strings
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

// (!) The following code is deprecated and to be replaced or removed (!)
typedef class KAsciiString KString;

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
