 /* Equivalent of cstring (aka string.h), but not a full implementation. Manipulates chunks of RAM,
    and maybe C-style strings and array in the future.

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

#include <mem_allocator.h>
#include <kmaths.h>
#include <kstring.h>
#include <new.h>

void* memcpy(void* destination, const void* source, size_t num) {
    const char* new_source = (const char*) source;
    char* new_dest = (char*) destination;

    for(size_t i=0; i<num; ++i) new_dest[i] = new_source[i];

    return destination;
}

size_t strlen(const char* str) {
    uint32_t len;
    for(len = 0; str[len]!='\0'; ++len);
    return len;
}

KString::KString(const char* source) {
    len = strlen(source);
    contents = new(PID_KERNEL, VIRMEM_FLAGS_RW, true) char[len+1];
    memcpy((void*) contents, (const void*) source, len+1);
    current_location = 0;
}

KString::KString(const KString& source) {
    len = source.len;
    contents = new(PID_KERNEL, VIRMEM_FLAGS_RW, true) char[len+1];
    memcpy((void*) contents, (const void*) source.contents, len+1);
    current_location = 0;
}

KString& KString::operator=(const char* source) {
    set_length(strlen(source), false);
    memcpy((void*) contents, (const void*) source, len);
    current_location = 0;

    return *this;
}

KString& KString::operator=(const KString& source) {
    if(&source == this) return *this;

    set_length(source.len, false);
    memcpy((void*) contents, (const void*) source.contents, len);
    current_location = source.current_location;

    return *this;
}

void KString::clear() {
    len = 0;
    if(contents) delete[] contents;
    current_location = 0;
}

void KString::set_length(size_t desired_length, bool keep_contents) {
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
    contents = new(PID_KERNEL, VIRMEM_FLAGS_RW, true) char[len+1];
    if(old_contents) {
        memcpy((void*) contents, (const void*) old_contents, min(len, old_len));
    }
    contents[len] = '\0';
    current_location = max(current_location, len);

    //Delete old string contents
    if(old_contents) delete[] old_contents;
}

bool KString::operator==(const char* param) const {
    for(uint32_t i = 0; i < len; ++i) {
        if(contents[i]!=param[i]) return false;
    }

    return true;
}

bool KString::operator==(const KString& param) const {
    if(param.len != len) return false;
    for(uint32_t i = 0; i < len; ++i) {
        if(contents[i]!=param[i]) return false;
    }

    return true;
}

KString& KString::operator+=(const char* source) {
    if(!contents) {
        return operator=(source);
    }

    //Resize string and concatenate new content
    size_t source_len = strlen(source);
    set_length(len+source_len);
    memcpy((void*) (len + (size_t) contents), (const void*) source, source_len);

    return *this;
}

KString& KString::operator+=(const KString& source) {
    if(!contents) {
        return operator=(source);
    }

    //Resize string and concatenate new content
    set_length(len+source.len);
    memcpy((void*) (len + (size_t) contents), (const void*) source.contents, source.len);

    return *this;
}

bool KString::extract_from(KString& source, size_t first_char, size_t length) {
    if(length > source.length()-first_char) return false;
    if(&source == this) return false;

    //Set string length to the appropriate value, extract new content
    set_length(length, false);
    memcpy((void*) contents, (const void*) (first_char + (size_t) source.contents), length);

    return true;
}

bool KString::paste(KString& source, size_t first_char, size_t length, size_t dest_index) {
    if(len < length + dest_index) return false;

    memcpy((void*) (dest_index + (size_t) contents),
           (const void*) (first_char + (size_t) source.contents),
           length);

    return true;
}

bool KString::read_line(KString& dest) {
    //Check if we are at end of file
    if(current_location == len) return false;

    //Extract a line of text from the string
    size_t old_location = current_location;
    while((current_location<len) && (contents[current_location]!='\n')) ++current_location;
    dest.extract_from(*this, old_location, current_location-old_location);
    if(current_location != len) ++current_location;

    return true;
}

size_t KString::heap_size() {
    start_faking_allocation();

        size_t to_be_allocd = (size_t) new(PID_KERNEL, VIRMEM_FLAGS_RW, true) char[len+1];

    stop_faking_allocation();

    return to_be_allocd;
}