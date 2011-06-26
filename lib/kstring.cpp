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

#include <kmem_allocator.h>
#include <kstring.h>

void* memcpy(void* destination, const void* source, size_t num) {
    const char* new_source = (const char*) source;
    char* new_dest = (char*) destination;

    for(size_t i=0; i<num; ++i) new_dest[i] = new_source[i];

    return destination;
}

uint32_t strlen(const char* str) {
    uint32_t len;
    for(len = 0; str[len]!=0; ++len);
    return len;
}

KString::KString(const char* source) {
    len = strlen(source);
    contents = (char*) kalloc((len+1)*sizeof(char));
    memcpy((void*) contents, (const void*) source, len+1);
}

KString::KString(const KString& source) {
    len = source.len;
    contents = (char*) kalloc((len+1)*sizeof(char));
    memcpy((void*) contents, (const void*) source.contents, len);
}

KString::~KString() {
    if(contents) kfree((void*) contents);
}

KString& KString::operator=(const char* source) {
    uint32_t source_len = strlen(source);
    if(source_len != len) {
        len = source_len;
        if(contents) kfree((void*) contents);
        contents = (char*) kalloc((len+1)*sizeof(char));
    }
    memcpy((void*) contents, (const void*) source, len);

    return *this;
}

KString& KString::operator=(const KString& source) {
    if(&source == this) return *this;

    if(source.len != len) {
        len = source.len;
        if(contents) kfree((void*) contents);
        contents = (char*) kalloc((len+1)*sizeof(char));
    }
    memcpy((void*) contents, (const void*) source.contents, len+1);

    return *this;
}

KString& KString::operator+=(const char* source) {
    if(!contents) {
        return operator=(source);
    }

    uint32_t source_len = strlen(source);
    if(!source_len) return *this;
    
    //Creation of the concatenated string
    char* buffer = (char*) kalloc((len+source_len+1)*sizeof(char));
    memcpy((void*) buffer, (const void*) contents, len);
    buffer = (char*) (((size_t) buffer) + len*sizeof(char));
    memcpy((void*) buffer, (const void*) source, source_len+1);
    
    //Replacement of the contents of the string
    kfree((void*) contents);
    contents = buffer;
    len+= source_len;
    return *this;
}

KString& KString::operator+=(const KString& source) {
    if(!contents) {
        return operator=(source);
    }

    if(!source.len) return *this;
    
    //Creation of the concatenated string
    char* buffer = (char*) kalloc((len+source.len+1)*sizeof(char));
    memcpy((void*) buffer, (const void*) contents, len);
    buffer = (char*) (((size_t) buffer) + len*sizeof(char));
    memcpy((void*) buffer, (const void*) source.contents, source.len+1);
    
    //Replacement of the contents of the string
    kfree((void*) contents);
    contents = buffer;
    len+= source.len;
    return *this;
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

uint32_t KString::heap_size() {
    return (len+1)*sizeof(char);
}
