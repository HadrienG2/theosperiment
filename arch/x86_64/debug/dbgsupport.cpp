/* Constants, macros, and classes potentially used by several debugging features

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
    
#include "dbgsupport.h"

//Border chars in left, top, right, bottom, topleft, topright, bottomleft, bottomright order
const char BOR_SINGLE[8] = {0xb3, 0xc4, 0xb3, 0xc4, 0xda, 0xbf, 0xc0, 0xd9};
const char BOR_DOUBLE[8] = {0xba, 0xcd, 0xba, 0xcd, 0xc9, 0xbb, 0xc8, 0xbc};
const char BOR_THICK[8] = {0xde, 0xdf, 0xdd, 0xdc, 0xde, 0xdd, 0xde, 0xdd};
const char BOR_BLOCK[8] = {0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb};

const char* border_array(DebugWindowBorder border) {
  switch(border) {
    case SINGLE:
      return BOR_SINGLE;
      break;
    case DOUBLE:
      return BOR_DOUBLE;
      break;
    case THICK:
      return BOR_THICK;
      break;
    case BLOCK:
      return BOR_BLOCK;
      break;
    default:
      return 0;
  }
}