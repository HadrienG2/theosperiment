 /* Memory management testing routines

      Copyright (C) 2010-2011  Hadrien Grasland

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

#ifndef _MEMORY_TEST_H_
#define _MEMORY_TEST_H_

#include <kernel_information.h>
#include <kmem_allocator.h>
#include <physmem.h>
#include <virtmem.h>

namespace MemTest {
    //Main test
    void test_memory(const KernelInformation& kinfo);

    //Helper functions
    void reset_title();
    void reset_sub_title();
    void test_title(const char* title);
    void subtest_title(const char* title);
    void item_title(const char* title);
    void test_failure(const char* message); //Displays an error message
}

#endif