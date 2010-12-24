 /* Process identifier definition, along with support classes

    Copyright (C) 2010    Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */
 
#include <pid.h>
#include <kmem_allocator.h>

PIDs& PIDs::copy_pids(const PIDs& source) {
    current_pid = source.current_pid;

    PIDs* source_parser = source.next_item;
    PIDs* dest_parser = this;
    while(source_parser) {
        if(dest_parser->next_item == NULL) {
            dest_parser->next_item = (PIDs*) kalloc(sizeof(PIDs), PID_KERNEL, VMEM_FLAGS_RW, true);
        }
        dest_parser = dest_parser->next_item;
        dest_parser->current_pid = source_parser->current_pid;
        source_parser = source_parser->next_item;
    }
    
    return *this;
}

void PIDs::free_members() {
    PIDs *to_delete = next_item, *following_one;
    
    while(to_delete) {
        following_one = to_delete->next_item;
        kfree(to_delete);
        to_delete = following_one;
    }
}

unsigned int PIDs::add_pid(const PID new_pid) {
    if(current_pid == PID_NOBODY) {
        current_pid = new_pid;
        return 1;
    }
    if(current_pid == new_pid) return 2;
    
    PIDs* parser = this;
    while(parser->next_item) {
        if(parser->next_item->current_pid == new_pid) return 2;
        parser = parser->next_item;
    }
    parser->next_item = (PIDs*) kalloc(sizeof(PIDs));
    if(parser->next_item == NULL) return 0;
    parser->next_item->current_pid = new_pid;
    
    return 1;
}

void PIDs::clear_pids() {
    current_pid = PID_NOBODY;
    free_members();
}

void PIDs::del_pid(const PID old_pid) {
    PIDs* former_item = NULL;
    
    if(current_pid == old_pid) {
        if(!next_item) {
            current_pid = PID_NOBODY;
        } else {
            former_item = next_item;
            current_pid = next_item->current_pid;
            next_item = next_item->next_item;
        }
    } else {
        PIDs* parser = (PIDs*) this;
        while(parser->next_item) {
            if(parser->next_item->current_pid == old_pid) {
                former_item = parser->next_item;
                parser->next_item = parser->next_item->next_item;
                break;
            }
            parser = parser->next_item;
        }
    }
    
    if(former_item) kfree(former_item);
}

bool PIDs::has_pid(const PID the_pid) const {
    PIDs* parser = (PIDs*) this;
    
    while(parser) {
        if(parser->current_pid == the_pid) return true;
        parser = parser->next_item;
    }
    return false;
}

unsigned int PIDs::length() const {
    PIDs* parser = (PIDs*) this;
    unsigned int result;
    
    for(result = 0; parser; ++result, parser = parser->next_item);
    return result;
}

PID PIDs::operator[](const unsigned int index) const {
    PIDs* parser = (PIDs*) this;
    
    for(unsigned int i = 0; (i<index) && parser; ++i, parser = parser->next_item);
    if(parser) {
        return parser->current_pid;
    } else {
        return PID_NOBODY;
    }
}


bool PIDs::operator==(const PIDs& param) const {
    PIDs *source, *dest;
    //This function compares two lists of PIDs, checking if each element of one is in the other.
    //(The algorithm is O(N^2), but sharing should not often occur between more than ~10 processes)
    
    source = (PIDs*) this;
    while(source) {
        dest = (PIDs*) &param;
        while(dest) {
            if(dest->current_pid == source->current_pid) break;
            dest = dest->next_item;
        }
        if(!dest) return false;
        source = source->next_item;
    }
    
    source = (PIDs*) &param;
    while(source) {
        dest = (PIDs*) this;
        while(dest) {
            if(dest->current_pid == source->current_pid) break;
            dest = dest->next_item;
        }
        if(!dest) return false;
        source = source->next_item;
    }
    
    return true;
}