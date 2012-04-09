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

#ifndef _PROCESS_PROPERTIES_H_
#define _PROCESS_PROPERTIES_H_

#include <kstring.h>

//Process properties are described in a text configuration file, whose specification can currently be
//found at http://theosperiment.wordpress.com/2012/03/25/process-properties-specification-version-1/
typedef KString ProcessProperties;

//Currently supported types of process properties
typedef int ProcessPropertyType;
const ProcessPropertyType INVALID = 0;
const ProcessPropertyType INT = 1;
const ProcessPropertyType BOOL = 2;
const ProcessPropertyType FLOAT = 3;
const ProcessPropertyType STRING = 4;
const ProcessPropertyType POINTER = 5;
const ProcessPropertyType CUSTOM = 6;
const ProcessPropertyType STRUCT = 7;
const ProcessPropertyType ARRAY = 8; //Array properties are generally an array of something.
                                     //Their full type is ARRAY | something. Thankfully, 8 is a
                                     //power of two...

//This class provides a convenient way to parse process properties
class ProcessPropertiesParser {
  private:
    ProcessProperties parsed_copy;

    bool check_brackets();
    bool check_header();
    bool check_insulator_decls();
    bool check_property_assignments();
    bool is_valid_naming_char(char c);
    void remove_comments_and_spacing();
  public:
    //Parsing initialization
    bool open_and_check(ProcessProperties& properties);
    void close();

    //Basic file manipulation
    bool end_of_file();
    void reset();
    void skip_current_insulator();
    void skip_current_property();
    bool lookup_insulator(KString insulator_name);
    bool lookup_property(KString property_name);
    KString current_insulator();
    KString current_property();
    ProcessPropertyType current_property_type();

    //Value extraction : extracts a copy of something from the file, then moves to next item
    ProcessProperties extract_insulator_decl(); //For full insulator declarations (header copy included)
    int extract_int_value();
    bool extract_bool_value();
    float extract_float_value();
    KString extract_string_value();
    KString extract_pointed_property(); //For pointers
    KString extract_custom_value(); //For custom values (only removes brackets, content untouched)

    //Structure and array management
    void enter_struct(); //Enters currently selected structure, so that its contents can be parsed.
    bool end_of_struct(); //True after the last element of a structure has been parsed.
    void enter_array(); //Enters currently selected array, so that its contents can be parsed.
    bool end_of_array(); //True after the last element of an array has been parsed.
};

#endif
