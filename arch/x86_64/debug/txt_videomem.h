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

#include <hack_stdint.h>

//Text attributes
#define DBG_TXT_BLACK 0x00
#define DBG_TXT_BLUE 0x01
#define DBG_TXT_GREEN 0x02
#define DBG_TXT_TURQUOISE 0x03
#define DBG_TXT_RED 0x04
#define DBG_TXT_PURPLE 0x05
#define DBG_TXT_MARROON 0x06
#define DBG_TXT_LIGHTGRAY 0x07
#define DBG_TXT_DARKGRAY 0x08
#define DBG_TXT_LIGHTBLUE 0x09
#define DBG_TXT_LIGHTGREEN 0x0A
#define DBG_TXT_LIGHTCOBALT 0x0B
#define DBG_TXT_LIGHTRED 0x0C
#define DBG_TXT_LIGHTPURPLE 0x0D
#define DBG_TXT_YELLOW 0x0E
#define DBG_TXT_WHITE 0x0F
#define DBG_BLINK 0x80
//Background attributes
#define DBG_BKG_BLACK 0x00
#define DBG_BKG_BLUE 0x10
#define DBG_BKG_GREEN 0x20
#define DBG_BKG_TURQUOISE 0x30
#define DBG_BKG_RED 0x40
#define DBG_BKG_PURPLE 0x50
#define DBG_BKG_MARROON 0x60
#define DBG_BKG_LIGHTGRAY 0x70

//Fill the screen with blank spaces
void dbg_clear_screen();
//Fill a rectangle on screen with blank spaces
void dbg_clear_rect(const int left, const int top, const int right, const int bottom);
//Init video memory (hides silly blinking cursor)
void dbg_init_videomem();
//Move the cursor to an absolute or relative position
void dbg_movecur_abs(const int col, const int row);
void dbg_movecur_rel(const int col_offset, const int row_offset);
//Print numbers and text
void dbg_print_bin8(const uint8_t integer);
void dbg_print_bin32(const uint32_t integer);
void dbg_print_bin64(const uint64_t integer);
void dbg_print_chr(const char chr);
void dbg_print_hex8(const uint8_t integer);
void dbg_print_hex32(const uint32_t integer);
void dbg_print_hex64(const uint64_t integer);
void dbg_print_int8(const int8_t integer);
void dbg_print_int32(const int32_t integer);
//void dbg_print_int64(const int64_t integer); ==> Not supported as of GCC 4.4.3
void dbg_print_str(const char* str);
void dbg_print_uint8(const uint8_t integer);
void dbg_print_uint32(const uint32_t integer);
//void dbg_print_uint64(const uint64_t integer); ==> Not supported as of GCC 4.4.3
/* Scroll text from "offset" lines (positive offset means scroll upper)
WARNING : Currently includes a temporisation */
void dbg_scroll(const int offset);
//Change text attributes
void dbg_set_attr(const char new_attr);

#endif
