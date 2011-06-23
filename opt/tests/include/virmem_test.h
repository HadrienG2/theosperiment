 /* Virtual memory management testing routines

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

#ifndef _VIRMEM_TEST_H_
#define _VIRMEM_TEST_H_

#include <physmem.h>
#include <virmem_test_arch.h>
#include <virtmem.h>

namespace Tests {
    const int VIRMEM_TEST_VERSION = 1;

    //Main tests
    bool meta_virmem(); //Check VirMemManager version
    VirMemManager* init_virmem(PhyMemManager& phymem);
    bool test_virmem(VirMemManager& virmem);

    //Auxiliary functions
    bool virinit_check_mapitems(VirMemState* virmem_state);
    bool virinit_check_listitems(VirMemState* virmem_state);
}

#endif