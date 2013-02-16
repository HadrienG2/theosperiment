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

#ifndef _DBGSUPPORT_H_
#define _DBGSUPPORT_H_

#include <kmath.h>

//*******************************************************
//****************** CLASSES AND ENUMS ******************
//*******************************************************

//Available numerical bases
enum DebugNumberBase {BINARY=2, OCTAL=8, DECIMAL=10, HEXADECIMAL=16};

//A simple rectangle class
struct DebugRect {
  unsigned int startx;
  unsigned int starty;
  unsigned int endx;
  unsigned int endy;
  DebugRect() : startx(0),
                starty(0),
                endx(0),
                endy(0) {}
  DebugRect(const unsigned int x1,
            const unsigned int y1,
            const unsigned int x2,
            const unsigned int y2) : startx(min(x1,x2)),
                                     starty(min(y1,y2)),
                                     endx(max(x1,x2)),
                                     endy(max(y1,y2)) {};
};

//A window is basically a rectangle with an optional border
enum DebugWindowBorder {NONE=0, SINGLE, DOUBLE, THICK, BLOCK};
struct DebugWindow {
  DebugRect frame;
  DebugWindowBorder border;
  DebugWindow() : border(NONE) {}
  DebugWindow(const DebugRect rect, const DebugWindowBorder init_border) : frame(rect),
                                                                            border(init_border) {};
  DebugWindow(const unsigned int x1,
              const unsigned int y1,
              const unsigned int x2,
              const unsigned int y2,
              const DebugWindowBorder init_border) : frame(min(x1,x2),min(y1,y2),max(x1,x2),max(y1,y2)),
                                                      border(init_border) {};
};

//*******************************************************
//************** CONSTANTS AND GLOBAL VARS **************
//*******************************************************

//Border characters arrays
const unsigned int BOR_LEFT = 0;
const unsigned int BOR_TOP = 1;
const unsigned int BOR_RIGHT = 2;
const unsigned int BOR_BOTTOM = 3;
const unsigned int BOR_TOPLEFT = 4;
const unsigned int BOR_TOPRIGHT = 5;
const unsigned int BOR_BOTTOMLEFT = 6;
const unsigned int BOR_BOTTOMRIGHT = 7;
extern const unsigned char BOR_SINGLE[8];
extern const unsigned char BOR_DOUBLE[8];
extern const unsigned char BOR_THICK[8];
extern const unsigned char BOR_BLOCK[8];
const unsigned char* border_array(const DebugWindowBorder border); //Find which array you want

//Other litteral constants
const char endl = '\n';
const char tab = '\t';

#endif
