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

#include <mem_allocator.h>
#include <kmaths.h>
#include <kstring.h>
#include <new.h>

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

//KUTF32String member functions

KUTF32String::KUTF32String(const KUTF32String& source) : file_index(0) {
    //Initialization from a KUTF32String is the easiest case, because we can assume
    //that these strings are well-formed Unicode and in NFD normalization form.
    len = source.len;
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) uint32_t[len+1];
    memcpy((void*) contents, (const void*) source.contents, (len+1)*sizeof(uint32_t));
}

KUTF32String::KUTF32String(KUTF8String& source) : file_index(0) {
    size_t source_index = 0;
    KUTF8CodePoint source_codepoint;

    //UTF-8 strings can be assumed to be well-formed, but are not NFD-normalized.
    len = source.codepoint_length();
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) uint32_t[len+1];
    for(size_t dest_index = 0; dest_index < len; ++dest_index) {
        source_codepoint = source.peek_codepoint(source_index);
        source_index+= source_codepoint.byte_length;
        contents[dest_index] = source_codepoint;
    }
    normalize_to_nfd();
}

KUTF32String::KUTF32String(const char* source) : file_index(0) {
    //ASCII strings can be assumed to be well-formed, but are not NFD-normalized
    len = strlen(source);
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) uint32_t[len];
    for(size_t index = 0; index < len; ++index) {
        contents[index] = source[index];
    }
    normalize_to_nfd();
}

void KUTF32String::clear() {
    len = 0;
    delete[] contents;  contents = NULL;
    file_index = 0;
}

void KUTF32String::normalize_to_nfd() {
    //Load the Unicode database if it hasn't been done yet (as probed by combining_class_db's value)
    if(combining_class_db == NULL) {
        InitializeKString();
    }
    
    //Display a warning 
    dbgout << "Error: normalize_to_nfd() is not implemented yet !" << endl;
}

//KUTF8String member functions

size_t KUTF8String::codepoint_length() const {
    size_t index = 0, result = 0;
    
    while(index < len) {
        index+= peek_codepoint(index).byte_length;
        ++result;
    }
    
    return result;
}

KUTF8CodePoint KUTF8String::peek_codepoint(const size_t index) const {
    KUTF8CodePoint result;
    size_t codepoint_index = index, remaining_bytes;
    uint32_t buffer;
    uint8_t min_second_byte = 0x80, max_second_byte = 0xBF;
    
    //Check if there still is at least one byte left in the string, otherwise return null code point
    if(codepoint_index >= len) {
        result = 0;
        result.byte_length = 0;
        return result;
    }
    
    //Extract all the information stored in the first byte of the string
    if(contents[codepoint_index] < 0x80) {
        remaining_bytes = 0;
        result = contents[codepoint_index];
    } else if(contents[codepoint_index] < 0xC2) {
        remaining_bytes = 0;
        result = 0xFFFD;
    } else if(contents[codepoint_index] < 0xE0) {
        remaining_bytes = 1;
        result = contents[codepoint_index] - 0xC0;
        result*= 0x40;
    } else if(contents[codepoint_index] < 0xF0) {
        remaining_bytes = 2;
        result = contents[codepoint_index] - 0xE0;
        result*= 0x1000;
        if(contents[codepoint_index] == 0xE0) {
            min_second_byte = 0xA0;
        } else if(contents[codepoint_index] == 0xED) {
            max_second_byte = 0x9F;
        }
    } else if(contents[codepoint_index] < 0xF5) {
        remaining_bytes = 3;
        result = contents[codepoint_index] - 0xF0;
        result*= 0x40000;
        if(contents[codepoint_index] == 0xF0) {
            min_second_byte = 0x90;
        } else if(contents[codepoint_index] == 0xF4) {
            max_second_byte = 0x8F;
        }
    } else {
        remaining_bytes = 0;
        result = 0xFFFD;
    }
    result.byte_length = remaining_bytes+1;
    
    //Move to the next byte of the string, if any
    ++codepoint_index;
    if(remaining_bytes == 0) return result;
    --remaining_bytes;
    
    //Check if the remaining bytes of the string do exist
    if(codepoint_index+remaining_bytes < len) {
        result = 0xFFFD;
        result.byte_length = len-codepoint_index;
        return result;
    }
    
    //Check if the second byte of the string is well-formed
    if((contents[codepoint_index] < min_second_byte) || (contents[codepoint_index] > max_second_byte)) {
        result = 0xFFFD;
        result.byte_length = codepoint_index-index;
        return result;
    }
    
    //If so extract its contents and add them to the result
    buffer = contents[codepoint_index] - 0x80;
    for(unsigned int i = 0; i < remaining_bytes; ++i) {
        buffer*= 0x40;
    }
    result+= buffer;
    ++codepoint_index;
    
    //Manage remaining bytes
    while(remaining_bytes > 0) {
        --remaining_bytes;
        if((contents[codepoint_index] < 0x80) || (contents[codepoint_index] > 0xBF)) {
            result = 0xFFFD;
            result.byte_length = codepoint_index-index;
            return result;
        }
        buffer = contents[codepoint_index] - 0x80;
        if(remaining_bytes) buffer*= 0x40;
        result+= buffer;
        ++codepoint_index;
    }
      
    return result;
}

//These private types are defined as permanent storage location for binary Unicode data
CombiningClassDB* combining_class_db = NULL;
CanonicalDecompositionDB* canonical_decomposition_db = NULL;

void InitializeKString() {
    dbgout << "Probing kernel modules..." << endl;
    dbgout << "Error: InitializeKString() is not implemented yet !" << endl;
}

// (!) The following code is deprecated and to be replaced or removed (!)

KAsciiString::KAsciiString(const char* source) {
    len = strlen(source);
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) char[len+1];
    memcpy((void*) contents, (const void*) source, len+1);
    current_location = 0;
}

KAsciiString::KAsciiString(const KAsciiString& source) {
    len = source.len;
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) char[len+1];
    memcpy((void*) contents, (const void*) source.contents, len+1);
    current_location = 0;
}

KAsciiString& KAsciiString::operator=(const char* source) {
    set_length(strlen(source), false);
    memcpy((void*) contents, (const void*) source, len);
    current_location = 0;

    return *this;
}

KAsciiString& KAsciiString::operator=(const KAsciiString& source) {
    if(&source == this) return *this;

    set_length(source.len, false);
    memcpy((void*) contents, (const void*) source.contents, len);
    current_location = source.current_location;

    return *this;
}

void KAsciiString::clear() {
    len = 0;
    if(contents) delete[] contents;
    current_location = 0;
}

void KAsciiString::set_length(size_t desired_length, bool keep_contents) {
    if(len == desired_length) return;

    //Keep track of old string contents, if asked to
    size_t old_len = len;
    char* old_contents = contents;
    if(!keep_contents) {
        delete[] old_contents;
        old_contents = NULL;
    }

    //Change string length
    len = desired_length;
    contents = new(PID_KERNEL, PAGE_FLAGS_RW, true) char[len+1];
    if(old_contents) {
        memcpy((void*) contents, (const void*) old_contents, min(len, old_len));
    }
    contents[len] = '\0';
    current_location = max(current_location, len);

    //Delete old string contents
    if(old_contents) delete[] old_contents;
}

bool KAsciiString::operator==(const char* param) const {
    for(uint32_t i = 0; i < len; ++i) {
        if(contents[i]!=param[i]) return false;
    }

    return true;
}

bool KAsciiString::operator==(const KAsciiString& param) const {
    if(param.len != len) return false;
    for(uint32_t i = 0; i < len; ++i) {
        if(contents[i]!=param[i]) return false;
    }

    return true;
}

KAsciiString& KAsciiString::operator+=(const char* source) {
    if(!contents) {
        return operator=(source);
    }

    //Resize string and concatenate new content
    size_t source_len = strlen(source);
    set_length(len+source_len);
    memcpy((void*) (len + (size_t) contents), (const void*) source, source_len);

    return *this;
}

KAsciiString& KAsciiString::operator+=(const KAsciiString& source) {
    if(!contents) {
        return operator=(source);
    }

    //Resize string and concatenate new content
    set_length(len+source.len);
    memcpy((void*) (len + (size_t) contents), (const void*) source.contents, source.len);

    return *this;
}

bool KAsciiString::extract_from(KAsciiString& source, size_t first_char, size_t length) {
    if(length > source.length()-first_char) return false;
    if(&source == this) return false;

    //Set string length to the appropriate value, extract new content
    set_length(length, false);
    memcpy((void*) contents, (const void*) (first_char + (size_t) source.contents), length);

    return true;
}

bool KAsciiString::paste(KAsciiString& source, size_t first_char, size_t length, size_t dest_index) {
    if(len < length + dest_index) return false;

    memcpy((void*) (dest_index + (size_t) contents),
           (const void*) (first_char + (size_t) source.contents),
           length);

    return true;
}

bool KAsciiString::read_line(KAsciiString& dest) {
    //Check if we are at end of file
    if(current_location == len) return false;

    //Extract a line of text from the string
    size_t old_location = current_location;
    while((current_location<len) && (contents[current_location]!='\n')) ++current_location;
    dest.extract_from(*this, old_location, current_location-old_location);
    if(current_location != len) ++current_location;

    return true;
}

size_t KAsciiString::heap_size() {
    start_faking_allocation();

        size_t to_be_allocd = (size_t) new(PID_KERNEL, PAGE_FLAGS_RW, true) char[len+1];

    stop_faking_allocation();

    return to_be_allocd;
}
