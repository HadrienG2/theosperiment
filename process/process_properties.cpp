 /* Home of process properties and their standard parser

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

#include <process_properties.h>

#include <dbgstream.h>

bool ProcessPropertiesParser::open_and_check(ProcessProperties& properties) {
    parsed_copy = properties;

    //Check that the header is valid and corresponds to the right version of the spec
    //bool result = check_header();
    //if(!result) return false;

    dbgout << "Not implemented yet !";
    return false;
}
