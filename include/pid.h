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

typedef unsigned int PID;
const PID PID_NOBODY = 0;
const PID PID_KERNEL = 1;

class PIDs {
    private:
        PID current_pid;
        PIDs* next_pid;
        void copy_from(const PIDs& source); //Make this PIDs a copy of source
        void free_members(); //Free all dynamically allocated data of this PIDs structure
    public:
        //Constructors and destructors
        PIDs() : current_pid(PID_NOBODY),
                         next_pid(NULL) {}
        PIDs(const PID& first) : current_pid(first),
                                 next_pid(NULL) {}
        ~PIDs() {free_members();}
        //Copy constructors and allocation operator
        PIDs(const PIDs& source) {copy_from(source);}
        PIDs& operator=(const PIDs& source);
        //Various management functions
        int add_pid(const PID new_pid); //Returns 0 if successful, 1 if it already exists
                                        //a negative error code in case of allocation failure
        void clear_pids();
        void del_pid(const PID old_pid);
        bool has_pid(const PID the_pid) const;
        unsigned int length() const;
        PID operator[](const unsigned int index) const; //Accessing the contents in an
                                                        //implementation-independent, read-only way.
        //Comparison of PIDs
        bool operator==(const PIDs& param) const;
        bool operator!=(const PIDs& param) const {return !(*this==param);}
} __attribute__((packed));

#endif