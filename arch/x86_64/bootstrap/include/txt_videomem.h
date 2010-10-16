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

//Text attributes
#define TXT_BLACK 0x00
#define TXT_BLUE 0x01
#define TXT_GREEN 0x02
#define TXT_TURQUOISE 0x03
#define TXT_RED 0x04
#define TXT_PURPLE 0x05
#define TXT_MARROON 0x06
#define TXT_LIGHTGRAY 0x07
#define TXT_DARKGRAY 0x08
#define TXT_LIGHTBLUE 0x09
#define TXT_LIGHTGREEN 0x0A
#define TXT_LIGHTCOBALT 0x0B
#define TXT_LIGHTRED 0x0C
#define TXT_LIGHTPURPLE 0x0D
#define TXT_YELLOW 0x0E
#define TXT_WHITE 0x0F
#define BLINK 0x80
//Background attributes
#define BKG_BLACK 0x00
#define BKG_BLUE 0x10
#define BKG_GREEN 0x20
#define BKG_TURQUOISE 0x30
#define BKG_RED 0x40
#define BKG_PURPLE 0x50
#define BKG_MARROON 0x60
#define BKG_LIGHTGRAY 0x70

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
void set_attr(const char new_attr);

#endif
