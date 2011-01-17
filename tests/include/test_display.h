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

#ifndef _TEST_DISPLAY_H_
#define _TEST_DISPLAY_H_

namespace Tests {
    //Helper functions
    void reset_title(); //Reset title counter to 1
    void reset_sub_title(); //Reset sub-title counter to 1
    void test_title(const char* title); //Display test title (e.g. "I/  Mytest")
    void subtest_title(const char* title); //Display test sub-title (e.g. "1. Mysubtest")
    void item_title(const char* title); //Display test item title (e.g. "* A wonderful test")
    void test_failure(const char* message); //Displays an error message
    void fail_notimpl(); //Warns the test user that a feature has not yet been implemented
}

#endif
