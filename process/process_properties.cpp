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

bool ProcessPropertiesParser::check_brackets() {
    KString current_line;
    size_t previous_index = parsed_copy.line_index();
    bool data_left, in_custom_brackets = false, in_string = false;
    int struct_nesting = 0;

    data_left = parsed_copy.read_line(current_line);
    while(data_left) {
        unsigned int i;
        bool in_array_brackets = false, escape_sequence = false;
        for(i = 0; i<current_line.length(); ++i) {
            //Inside custom values and array brackets, do not parse anything but the end.
            if(in_custom_brackets) {
                if(current_line[i] == '>') in_custom_brackets = false;
                continue;
            }
            if(in_array_brackets) {
                if(current_line[i] == ']') in_array_brackets = false;
                continue;
            }

            //Inside strings, take care of escaping sequences and do not parse anything else
            if(in_string) {
                if(!escape_sequence) {
                    if(current_line[i] == '"') in_string = false;
                    else if(current_line[i] == '\\') escape_sequence = true;
                } else {
                    escape_sequence = false;
                }
                continue;
            }

            //Take care of closing brackets for structs
            if((struct_nesting) && (current_line[i] == '}')) {
                --struct_nesting;
                continue;
            }

            //Parse opening brackets
            if(current_line[i] == '<') in_custom_brackets = true;
            else if(current_line[i] == '[') in_array_brackets = true;
            else if(current_line[i] == '"') in_string = true;
            else if(current_line[i] == '{') ++struct_nesting;

            //Abort if a closing bracket is found when no bracket is opened.
            if(current_line[i] == '>') {
                dbgout << "/!\\ Closing custom bracket without an opening bracket in : " << current_line << endl;
                return false;
            } else if(current_line[i] == ']') {
                dbgout << "/!\\ Closing array bracket without an opening bracket in : " << current_line << endl;
                return false;
            } else if(current_line[i] == '}') {
                dbgout << "/!\\ Closing struct bracket without an opening bracket in : " << current_line << endl;
                return false;
            }
        }

        //At the end of a line, all array brackets should be closed
        if(in_array_brackets) {
            dbgout << "/!\\ Array bracket not closed at end of line : " << current_line << endl;
            return false;
        }

        //Fetch next line of the process properties
        data_left = parsed_copy.read_line(current_line);
    }

    //At the end of a file, all special, string, and struct brackets should be closed.
    if(in_custom_brackets) {
        dbgout << "/!\\ Custom brackets not closed at end of file" << endl;
        return false;
    }
    if(in_string) {
        dbgout << "/!\\ String not closed at end of file" << endl;
        return false;
    }
    if(struct_nesting) {
        dbgout << "/!\\ Struct brackets not closed at end of file" << endl;
        return false;
    }

    //Reset line pointer
    parsed_copy.goto_index(previous_index);

    return true;
}

bool ProcessPropertiesParser::check_header() {
    KString header;
    const char* CURRENT_HEADER = "*** Process properties v1 ***";

    parsed_copy.goto_index(0);
    parsed_copy.read_line(header);
    if(header != CURRENT_HEADER) { //No need for backwards compatibility at revision 1 !
        dbgout << "/!\\ Invalid header : " << header << endl;
        return false;
    }

    return true;
}

bool ProcessPropertiesParser::check_insulator_decls() {
    KString current_line;
    size_t previous_index = parsed_copy.line_index();
    bool data_left;

    data_left = parsed_copy.read_line(current_line);
    while(data_left) {
        //Check if we are dealing with an insulator declaration
        if(current_line[current_line.length()-1] == ':') {
            //Insulator names should be at least one character long
            if(current_line.length() == 1) {
                dbgout << "/!\\ Empty insulator name" << endl;
                return false;
            }

            //Parse insulator name for forbidden characters
            unsigned int i;
            for(i=0; i<current_line.length()-1; ++i) {
                if(!is_valid_naming_char(current_line[i])) {
                    dbgout << "/!\\ Invalid character in insulator declaration : " << current_line << endl;
                    return false;
                }
            }
        }

        //Fetch next line of the process properties
        data_left = parsed_copy.read_line(current_line);
    }

    //Reset line pointer
    parsed_copy.goto_index(previous_index);

    return true;
}

bool ProcessPropertiesParser::check_property_assignments() {
    KString current_line;
    size_t previous_index = parsed_copy.line_index();
    bool data_left;

    data_left = parsed_copy.read_line(current_line);
    while(data_left) {
        //Check if we are dealing with a property assignment
        if(current_line[current_line.length()-1] != ':') {
            //Find position of the equality sign
            unsigned int equal_position;
            for(equal_position = 0; equal_position < current_line.length(); ++equal_position) {
                if(current_line[equal_position] == '=') break;
            }

            //If no equality sign is found, abort
            if(equal_position == current_line.length()) {
                dbgout << "/!\\ Gibberish in configuration file : " << current_line << endl;
                return false;
            }

            //Check property name
            for(unsigned int i = 0; i < equal_position; ++i) {
                if(!is_valid_naming_char(current_line[i])) {
                    dbgout << "/!\\ Invalid character in property assignment : " << current_line << endl;
                    return false;
                }
            }

            //TODO : Check property value
            dbgout << "/!\\ TODO : Check property value" << endl;
        }

        //Fetch next line of the process properties
        data_left = parsed_copy.read_line(current_line);
    }

    //Reset line pointer
    parsed_copy.goto_index(previous_index);

    return true;
}

bool ProcessPropertiesParser::is_valid_naming_char(char c) {
    if((c=='=') || (c=='{') || (c=='}') || (c=='[') || (c==']') || (c=='<') || (c=='>') || (c=='"')) {
        return false;
    } else {
        return true;
    }
}

void ProcessPropertiesParser::remove_comments_and_spacing() {
    KString current_line;
    bool data_left, in_custom_value = false, in_string = false;
    size_t previous_index = parsed_copy.line_index(), writing_index = previous_index;

    data_left = parsed_copy.read_line(current_line);
    while(data_left) {
        unsigned int i, line_length;
        bool escape_sequence = false;
        for(i = 0, line_length = 0; i<current_line.length(); ++i, ++line_length) {
            //Keep a copy of any character by default, we will eliminate spaces and tabs later
            current_line[line_length] = current_line[i];

            //Inside custom values, do not parse anything but the end of the custom value
            if(in_custom_value) {
                if(current_line[i] == '>') in_custom_value = false;
                continue;
            }

            //Inside strings, take care of escaping sequences and do not parse anything else
            if(in_string) {
                if(!escape_sequence) {
                    if(current_line[i] == '"') in_string = false;
                    else if(current_line[i] == '\\') escape_sequence = true;
                } else {
                    escape_sequence = false;
                }
                continue;
            }

            //Otherwise, normally parse comments, string and custom value beginnings, and spaces
            if(current_line[i] == '#') break;
            else if(current_line[i] == '<') in_custom_value = true;
            else if(current_line[i] == '"') in_string = true;
            else if((current_line[i] == ' ') || (current_line[i] == '\t')) --line_length; //Eliminate spaces and tabs
        }
        current_line.set_length(line_length);

        //Commit modified current_line to parsed_copy
        if(current_line.length()) {
            parsed_copy.paste(current_line, writing_index);
            writing_index+=current_line.length();
            parsed_copy[writing_index]='\n';
            ++writing_index;
        }

        //Fetch next line of the process properties
        data_left = parsed_copy.read_line(current_line);
    }
    parsed_copy.set_length(writing_index-1);

    //Reset line pointer
    parsed_copy.goto_index(previous_index);
}

bool ProcessPropertiesParser::open_and_check(ProcessProperties& properties) {
    parsed_copy = properties;

    //Check header and spec revision, remove comments and spacing for speed, check that all brackets
    //are properly closed, check insulator declarations and property types and values.
    bool result = check_header();
    if(!result) return false;
    remove_comments_and_spacing();
    result = check_brackets();
    if(!result) return false;
    result = check_insulator_decls();
    if(!result) return false;
    result = check_property_assignments();
    if(!result) return false;

    return true;
}
