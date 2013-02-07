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

#include <ProcessManager.h>
#include <dbgstream.h>

ProcessManager::ProcessManager(MemAllocator& mem_allocator) {
    //TODO : Put the process manager's initialization code here.
    dbgout << "Error: ProcessManager() is not implemented yet !" << endl;

    //Initialize the process-related functionality in memory management
    mem_allocator.init_process(*this);
}
