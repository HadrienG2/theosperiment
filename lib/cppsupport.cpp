 /* Some symbols which must be defined for C++ code to work, even though we don't need them

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

//Support code for shared libraries. We're not coding a shared library, so these are stubs
extern "C" {
    void *__dso_handle;
    void __cxa_atexit() {
        
    }
}

//What happens where a pure virtual function is called ? Nothing.
extern "C" {
    void __cxa_pure_virtual()
    {

    }
}
