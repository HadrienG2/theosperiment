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

#include <KUtf8String.h>

size_t KUtf8String::codepoint_length() const {
    size_t index = 0, result = 0;
    
    while(index < contents.length()) {
        index+= peek_codepoint(index).byte_length;
        ++result;
    }
    
    return result;
}

KUtf8CodePoint KUtf8String::peek_codepoint(const size_t index) const {
    KUtf8CodePoint result;
    size_t codepoint_index = index, remaining_bytes;
    uint32_t buffer;
    uint8_t min_second_byte = 0x80, max_second_byte = 0xBF;
    
    //Check if there still is at least one byte left in the string, otherwise return null code point
    if(codepoint_index >= contents.length()) {
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
    if(codepoint_index+remaining_bytes < contents.length()) {
        result = 0xFFFD;
        result.byte_length = contents.length()-codepoint_index;
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
