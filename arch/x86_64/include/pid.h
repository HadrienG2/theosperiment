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

#ifndef _PID_H_
#define _PID_H_

#include <address.h>
#include <hack_stdint.h>

typedef uint32_t PID;
const PID PID_NOBODY = 0;
const PID PID_KERNEL = 1;

class PIDs {
    private:
        PID current_pid;
        PIDs* next_item;
        PIDs& copy_pids(const PIDs& source); //Copy the contents of a PIDs in there
        void free_members(); //Free all dynamically allocated data of this PIDs structure
    public:
        //Constructors and destructors
        PIDs() : current_pid(PID_NOBODY),
                         next_item(NULL) {}
        PIDs(const PID& first) : current_pid(first),
                                 next_item(NULL) {}
        PIDs(const PIDs& source) {copy_pids(source);}
        ~PIDs() {free_members();}
        //Various management functions
        unsigned int add_pid(const PID new_pid); //Returns 0 if it failed, 1 if it was successful,
                                        //2 if the item already existed
        void clear_pids();
        void del_pid(const PID old_pid);
        bool has_pid(const PID the_pid) const;
        unsigned int length() const;
        size_t heap_size() const {return sizeof(PIDs)*(length()-1);}
        PID operator[](const unsigned int index) const; //Accessing the contents in an
                                                        //implementation-independent, read-only way.
        
        //Duplicate a PIDs
        PIDs& operator=(const PIDs& source) {return copy_pids(source);}
        //Comparison of PIDs
        bool operator==(const PIDs& param) const;
        bool operator!=(const PIDs& param) const {return !(*this==param);}
} __attribute__((packed));

#endif