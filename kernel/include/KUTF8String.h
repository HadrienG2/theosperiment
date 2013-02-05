 /* An implementation of Unicode 6.2.0 strings in UTF-8 representation,
    for the purpose of text storage and interchange.

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

#ifndef _KUTF8STRING_H_
#define _KUTF8STRING_H_

#include <address.h>
#include <Array.h>
#include <KUTF32String.h>
#include <stdint.h>

typedef class KUTF32String KUTF32String;

//This is a way to express an UTF-8 encoded Unicode code point, featuring both its code point value
//and its length in bytes when encoded in UTF-8
struct KUTF8CodePoint {
    uint32_t code_point;
    size_t byte_length;
    operator uint32_t&() {return code_point;}
    KUTF8CodePoint& operator=(uint32_t value) {code_point = value; return *this;}
};

//This is an UTF-8 string, to be used for internal storage and interchange purpose. It is not normalized.
class KUTF8String {
  private:
    Array<uint8_t> contents;
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
    size_t byte_length() const {return contents.length();}
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

#endif
