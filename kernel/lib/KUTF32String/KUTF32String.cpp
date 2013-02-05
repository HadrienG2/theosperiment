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

#include <KUTF32String.h>

#include <dbgstream.h>

KUTF32String::KUTF32String(const KUTF32String& source) : file_index(0) {
    //Initialization from a KUTF32String is the easiest case, because we can assume
    //that these strings are well-formed Unicode and in NFD normalization form.
    contents = source.contents;
}

KUTF32String::KUTF32String(KUTF8String& source) : file_index(0) {
    size_t source_index = 0;
    KUTF8CodePoint source_codepoint;

    //UTF-8 strings can be assumed to be well-formed, since peek_codepoint() takes care
    //of ill-formed sequences, but they are not NFD-normalized.
    contents.set_length(source.codepoint_length(), false, true);
    for(size_t dest_index = 0; dest_index < contents.length(); ++dest_index) {
        source_codepoint = source.peek_codepoint(source_index);
        source_index+= source_codepoint.byte_length;
        contents[dest_index] = source_codepoint;
    }
    normalize_to_nfd();
}

KUTF32String::KUTF32String(const char* source) : file_index(0) {
    //ASCII strings can be assumed to be well-formed, but are not NFD-normalized
    contents.set_length(strlen(source));
    for(size_t index = 0; index < contents.length(); ++index) {
        contents[index] = source[index];
    }
    normalize_to_nfd();
}

void KUTF32String::clear() {
    contents.clear();
    file_index = 0;
}

void KUTF32String::normalize_to_nfd() {
    //Load the Unicode database if it hasn't been done yet (as probed by combining_class_db's value)
    if(combining_class_db == NULL) {
        InitializeUnicodeSupport();
    }
    
    //Display a warning 
    dbgout << "Error: normalize_to_nfd() is not implemented yet !" << endl;
}
