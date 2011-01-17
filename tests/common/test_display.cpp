 /* Some routines used to display test output in a consistent way

    Copyright (C) 2011  Hadrien Grasland

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

#include <test_display.h>

#include <dbgstream.h>

namespace Tests {
    static int title_count;
    static int sub_title_count;

    void reset_title() {
        title_count = 0;
    }

    void reset_sub_title() {
        sub_title_count = 0;
    }

    void test_title(const char* title) {
        ++title_count;
        switch(title_count) {
            case 1:
                dbgout << "I/  ";
                break;
            case 2:
                dbgout << "II/ ";
                break;
            case 3:
                dbgout << "III/";
                break;
            case 4:
                dbgout << "IV/ ";
                break;
            case 5:
                dbgout << "V/  ";
                break;
            case 6:
                dbgout << "VI/ ";
                break;
            case 7:
                dbgout << "VII/";
                break;
            default:
                dbgout << "?/  ";
        }
        dbgout << title << endl;
    }

    void subtest_title(const char* title) {
        ++sub_title_count;
        dbgout << "  " << sub_title_count << ". " << title << endl;
    }

    void item_title(const char* title) {
        dbgout << "    * " << title << endl;
    }

    void test_failure(const char* message) {
        dbgout << txtcolor(TXT_LIGHTRED) << "  Error : " << message;
        dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
    }

    void fail_notimpl() {
        dbgout << txtcolor(TXT_RED) << "  This test is not implemented yet !";
        dbgout << txtcolor(TXT_LIGHTGRAY) << endl;
    }
}