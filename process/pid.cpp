 /* Process identifier definition, along with support classes
 
    Copyright (C) 2010  Hadrien Grasland

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
 
#include <pid.h>

void PIDs::copy_from(const PIDs& source) {
  current_pid = source.current_pid;
  //Currently, due to lack of memory allocation, there can't be several PIDs in a PIDs structure
  next_pid = NULL;
}

void PIDs::free_members() {
  //Do nothing. There is no memory allocation around right now.
}

#include <dbgstream.h>

PIDs& PIDs::operator=(const PIDs& source) {
  if(&source!=this) {
    free_members();
    copy_from(source);
  }
  return *this;
}

int PIDs::add_pid(PID new_pid) {
  if(!current_pid) {
    current_pid = new_pid;
    return 0;
  } else {
    if(current_pid == new_pid) return 1;
    else return -1; //We do not have memory allocation at the moment
  }
}

void PIDs::clear_pids() {
  current_pid = PID_NOBODY;
  //We do not have memory allocation at the moment, so this ends here
}

void PIDs::del_pid(PID old_pid) {
  if(current_pid == old_pid) {
    current_pid = PID_NOBODY;
  } else {
    //We do not have memory allocation at the moment, so nothing to do
  }
}

bool PIDs::has_pid(PID the_pid) {
  if(current_pid == the_pid) {
    return 1;
  } else {
    return 0; //We do not have memory allocation at the moment
  }
}
