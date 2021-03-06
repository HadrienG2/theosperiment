 /* Text-mode video memory manipulation routines

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

#ifndef _VIDEOMEM_H_
#define _VIDEOMEM_H_

#include <stdint.h>

typedef uint8_t TxtAttr;
//Text color attributes
extern const TxtAttr TXT_BLACK;
extern const TxtAttr TXT_BLUE;
extern const TxtAttr TXT_GREEN;
extern const TxtAttr TXT_TURQUOISE;
extern const TxtAttr TXT_RED;
extern const TxtAttr TXT_PURPLE;
extern const TxtAttr TXT_MARROON;
extern const TxtAttr TXT_LIGHTGRAY;
extern const TxtAttr TXT_DARKGRAY;
extern const TxtAttr TXT_LIGHTBLUE;
extern const TxtAttr TXT_LIGHTGREEN;
extern const TxtAttr TXT_LIGHTCOBALT;
extern const TxtAttr TXT_LIGHTRED;
extern const TxtAttr TXT_LIGHTPURPLE;
extern const TxtAttr TXT_YELLOW;
extern const TxtAttr TXT_WHITE;

extern const TxtAttr TXT_BLINK; //Blinking text

//Background color attributes
extern const TxtAttr BKG_BLACK;
extern const TxtAttr BKG_BLUE;
extern const TxtAttr BKG_GREEN;
extern const TxtAttr BKG_TURQUOISE;
extern const TxtAttr BKG_RED;
extern const TxtAttr BKG_PURPLE;
extern const TxtAttr BKG_MARROON;
extern const TxtAttr BKG_LIGHTGRAY;

//Fill the screen with blank spaces
void clear_screen();
//Fill a rectangle on screen with blank spaces
void clear_rect(const unsigned int left, const unsigned int top, const unsigned int right, const unsigned int bottom);
//Init video memory (hides silly blinking cursor)
void init_videomem();
//Move the cursor to an absolute or relative position
void movecur_abs(const unsigned int col, const unsigned int row);
void movecur_rel(const int col_offset, const int row_offset);
//Print numbers and text
void print_bin8(const uint8_t integer);
void print_bin32(const uint32_t integer);
void print_bin64(const uint64_t integer);
void print_chr(const char chr);
void print_hex8(const uint8_t integer);
void print_hex32(const uint32_t integer);
void print_hex64(const uint64_t integer);
void print_int8(const int8_t integer);
void print_int32(const int32_t integer);
//void print_int64(const int64_t integer); ==> Not supported as of GCC 4.5.0
void print_str(const char* str);
void print_uint8(const uint8_t integer);
void print_uint32(const uint32_t integer);
//void print_uint64(const uint64_t integer); ==> Not supported as of GCC 4.5.0
/* Scroll text from "offset" lines (positive offset means scroll upper) */
void scroll(const int offset);
//Change text attributes
void set_attr(const TxtAttr new_attr);

#endif
