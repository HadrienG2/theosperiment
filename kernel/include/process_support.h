 /* This header specifies support structure used by the process manager, such as the process
    descriptor (a data structure which centralizes a process' properties) or the notion of insulator
    IDs (unique integer identifiers for insulators)

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

#ifndef _PROCESS_SUPPORT_H_
#define _PROCESS_SUPPORT_H_

#include <deprecated/KAsciiString.h>
#include <ProcessProperties.h>
#include <stdint.h>

//*************************************************
//******* Process properties and descriptor *******
//*************************************************

struct ProcessDescriptor {
    KAsciiString filename; //The filename is a string identifier which can be used to retrieve the PID of
                      //a known running process. As the name suggests, process image locations are
                      //generally used for this purpose.
    bool visible_to_filename_lookup; //Set this to false if you do not want this process to be visible
                                     //to filename lookup. Generally used when updating processes.
    ProcessProperties process_properties; //The process' per-insulator properties

    ProcessDescriptor() : visible_to_filename_lookup(true) {}
};

//*************************************************
//********** Insulator descriptor and ID **********
//*************************************************

typedef uint32_t InsulatorID;
const InsulatorID INSULATORID_INVALID = 0;

struct InsulatorDescriptor {
    KAsciiString insulator_name; //Allows an insulator to get a fancy name when described in a process
                            //descriptor. If this string is left blank, the filename of the host
                            //process is used instead.
    void (*add_process)(PID,ProcessProperties); //Pointer to the add_process(PID, ProcessProperties) function of the insulator.
                       //Is either a casted function pointer for in-kernel insulators or a casted SID
                       //for out-of-kernel insulators.
    void (*remove_process)(PID); //Pointer to the remove_process(PID) function of the insulator. Is either
                          //a casted function pointer for in-kernel insulators or a casted SID for
                          //out-of-kernel insulators.
    void (*update_process)(PID, PID); //Pointer to the update_process(PID, PID) function of the insulator. Is
                          //either a casted function pointer for in-kernel insulators or a casted
                          //SID for out-of-kernel insulators.

    InsulatorDescriptor() : add_process(NULL),
                            remove_process(NULL),
                            update_process(NULL) {}
};

#endif
