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
    //Visual hierarchy
    void test_beginning(const char* component_name); //Run at the beginning of a test.
                                                     //Sets text display to full screen and displays
                                                     //something like "Beginning <component_name>
                                                     //testing..."
    void reset_title(); //Reset title counter to 1
    void reset_sub_title(); //Reset sub-title counter to 1
    void test_title(const char* title); //Display test title (e.g. "I/  Mytest")
    void subtest_title(const char* title); //Display test sub-title (e.g. "1. Mysubtest")
    void item_title(const char* title); //Display test item title (e.g. "* A wonderful test")

    //Message display
    void test_success(); //Informs the user that all tests have been completed successfuly.
    void test_failure(const char* message); //Displays an error message
    void fail_notimpl(); //Warns the test user that a feature has not yet been implemented

    //Benchmarking
    void bench_start(); //To be run in the beginning of a benchmark-type test. Starts the system
                        //chronometer, if there's one, or else asks the tester to prepare himself for
                        //hand measurements and give him some time.
    void bench_stop(); //To be run in the end of a benchmark. Displays the measured time, if the
                       //system could measure it, or else asks the tester to stop his measurement.
}

#endif
