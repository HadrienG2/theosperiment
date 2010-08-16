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

//*******************************************************
//********************* SOME MACROS *********************
//*******************************************************

#define max(a,b) ((a)<(b))?(b):(a)
#define min(a,b) ((a)>(b))?(b):(a)

//*******************************************************
//****************** CLASSES AND ENUMS ******************
//*******************************************************

//Available numerical bases
enum DebugNumberBase {BINARY=2, OCTAL=8, DECIMAL=10, HEXADECIMAL=16};

//A simple rectangle class
struct DebugRect {
  int startx;
  int starty;
  int endx;
  int endy;
  DebugRect() : startx(0), starty(0), endx(0), endy(0) {}
  DebugRect(int x1, int y1, int x2, int y2) : startx(min(x1,x2)), starty(min(y1,y2)), endx(max(x1,x2)), endy(max(y1,y2)) {};
};

//A window is basically a rectangle with an optional border
enum DebugWindowBorder {NONE=0, SINGLE, DOUBLE, THICK, BLOCK};
struct DebugWindow : DebugRect {
  //For some reason, attributes are not inherited, so I rewrite them here
  int startx;
  int starty;
  int endx;
  int endy;
  //New things
  DebugWindowBorder border;
  DebugWindow() : startx(0), starty(0), endx(0), endy(0), border(NONE) {}
  DebugWindow(DebugRect rect, DebugWindowBorder init_border) : startx(rect.startx), starty(rect.starty), endx(rect.endx), endy(rect.endy), border(init_border) {};
  DebugWindow(int x1, int y1, int x2, int y2, DebugWindowBorder init_border) : startx(min(x1,x2)), starty(min(y1,y2)), endx(max(x1,x2)), endy(max(y1,y2)), border(init_border) {};
};

//*******************************************************
//************** CONSTANTS AND GLOBAL VARS **************
//*******************************************************

//Border characters arrays
#define BOR_LEFT 0
#define BOR_TOP 1
#define BOR_RIGHT 2
#define BOR_BOTTOM 3
#define BOR_TOPLEFT 4
#define BOR_TOPRIGHT 5
#define BOR_BOTTOMLEFT 6
#define BOR_BOTTOMRIGHT 7
extern const char BOR_SINGLE[8];
extern const char BOR_DOUBLE[8];
extern const char BOR_THICK[8];
extern const char BOR_BLOCK[8];
const char* border_array(DebugWindowBorder border); //Find which array you want

//Other litteral constants
#define endl '\n'
#define tab '\t'

#endif