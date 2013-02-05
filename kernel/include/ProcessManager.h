 /* The process manager is a kernel component which centrally manages the various parts of the
    process abstraction in kernel and user space.

      Copyright (C) 2012  Hadrien Grasland

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

#ifndef _PROCESS_MANAGER_H_
#define _PROCESS_MANAGER_H_

#include <deprecated/KAsciiString.h>
#include <MemAllocator.h>
#include <pid.h>
#include <process_support.h>

const int PROCESSMANAGER_VERSION = 1; //Increase this when deep changes require a modification of
                                      //the automated testing protocol

class MemAllocator;
class ProcessManager {
  public:
    ProcessManager(MemAllocator& mem_allocator);

    //Add, remove, and update processes
    PID add_process(ProcessDescriptor process_desc);
    void remove_process(PID target);
    PID update_process(PID old_process, PID new_process);

    //Find a PID by file name
    PID find_pid(KAsciiString filename);

    //Add and remove insulators, system components which manage a part of the process abstraction
    InsulatorID add_insulator(PID host_process, InsulatorDescriptor insulator_descriptor);
    void remove_insulator(InsulatorID insulator);
};

#endif
