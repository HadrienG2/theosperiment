 /* A way to make the kernel "die" when there's nothing else to do

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
 
#include "die.h"
#include "wait.h"
#include "txt_videomem.h"

void die(const char* issue) {
  dbg_set_attr(DBG_TXT_WHITE | DBG_BKG_PURPLE);
  dbg_clear_screen();
  dbg_print_str("Sorry, something went wrong, and the operating system just died.\n");
  dbg_print_str("Before dying, though, it left some information that may prove to be useful :\n\n");
  dbg_set_attr(DBG_TXT_YELLOW | DBG_BKG_PURPLE);
  dbg_print_str(issue);
  dbg_wait(-1);
}