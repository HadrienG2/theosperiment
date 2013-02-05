 /* Kernel module management facilities, used to handle those files which
    are loaded to RAM by the bootloader before we can actually load stuff
    from disk

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

#ifndef _MODULE_MANAGER_H_
#define _MODULE_MANAGER_H_

#include <address.h>
#include <KernelInformation.h>
#include <KUtf32String.h>
#include <pid.h>

typedef size_t ModuleID; //Nonzero identifier used to label individual instances of a given module


struct ModuleDescriptor { //"Public" description of a loaded kernel module
    size_t location;
    size_t size;
    ModuleDescriptor() : location(0), size(0) {}
};


struct ModulePrivateDescriptor { //Internal description of a kernel module
    ModuleDescriptor public_description;
    KUtf32String name;
    //TODO: To be completed...
};


const int MODULEMANAGER_VERSION = 1; //Increase this when deep changes require a modification of
                                      //the testing protocol


class ModuleManager {
  private:
    //TODO: Implement this !
  public:
    ModuleManager(KernelInformation& kinfo);
    
    //Request access to a module, labeled using its filename. Returns zero identifier if the module does not exist.
    ModuleID request_module(PID requester, KUtf32String& filename);
    
    //Get data about a given module after having been granted access to it.
    ModuleDescriptor get_module_descriptor(PID requester, ModuleID module);
    
    //Notify the module manager that a given module is not needed anymore
    void liberate_module(PID requester, ModuleID module);
    
    //Methods below that are for internal use only, and may change at any time without previous notice
    ModuleID request_module_ascii(PID requester, char* ascii_filename);
    //TODO: Implement void file_system_ready(<unknown parameters>), used when the filesystem is ready to take over
};

#endif
