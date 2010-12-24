 /* Physical memory management testing routines

      Copyright (C) 2010  Hadrien Grasland

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

#ifndef _PHYMEM_TEST_H_
#define _PHYMEM_TEST_H_

#include <kernel_information.h>
#include <physmem.h>

namespace MemTest {
    const int PHYMEM_TEST_VERSION = 1;
    
    //Main test
    PhyMemManager* test_phymem(const KernelInformation& kinfo);
    
    //PhyMemManager tests
    bool phy_test1_meta();
    PhyMemManager* phy_test2_init(const KernelInformation& kinfo);
    PhyMemManager* phy_test3_pagealloc(PhyMemManager& phymem);
}

#endif