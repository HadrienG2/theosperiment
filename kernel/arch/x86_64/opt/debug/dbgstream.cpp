 /* Iostream-like objects for debugging purposes

    Copyright (C) 2010-2013  Hadrien Grasland

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

#include "dbgstream.h"
#include <kmaths.h>

//A rectangle of the size of the screen
const DebugRect screen_rect(0, 0, NUMBER_OF_COLS-1, NUMBER_OF_ROWS-1);

//Various windows
const DebugWindow bottom_oneline_win(0,NUMBER_OF_ROWS-1,NUMBER_OF_COLS-1,NUMBER_OF_ROWS-1,SINGLE);
const DebugWindow bottom_fivelines_win(0,NUMBER_OF_ROWS-5,NUMBER_OF_COLS-1,NUMBER_OF_ROWS-1,SINGLE);
const DebugWindow bottom_tenlines_win(0,NUMBER_OF_ROWS-11,NUMBER_OF_COLS-1,NUMBER_OF_ROWS-1,SINGLE);
const DebugWindow top_oneline_win(0,0,NUMBER_OF_COLS-1,0,SINGLE);
const DebugWindow top_fivelines_win(0,0,NUMBER_OF_COLS-1,4,SINGLE);
const DebugWindow top_tenlines_win(0,0,NUMBER_OF_COLS-1,10,SINGLE);
static const DebugWindow topleftcorner_win(0,0,0,0,NONE); //The smallest possible window ;)
const DebugWindow screen_win(screen_rect, NONE);

//"Standard" debugging output
DebugOutput dbgout(bottom_tenlines_win);

//Buffers
static DebugAttributeChanger attrchg_buff;
static DebugBreakpoint bp_buff;
static DebugCursorMover cursmov_buff;
static DebugNumberBaseChanger numchg_buff;
static DebugPadder padder_buff;
static DebugScroller scroller_buff;
static DebugWindower windower_buff;

DebugAttributeChanger attrset(const uint8_t attribute) {
  attrchg_buff.new_attr = (uint8_t) attribute;
  attrchg_buff.mask = 0xff;

  return attrchg_buff;
}

DebugAttributeChanger blink(const bool blink_status) {
  if(blink_status) attrchg_buff.new_attr = TXT_BLINK;
  else attrchg_buff.new_attr = 0;
  attrchg_buff.mask = TXT_BLINK;

  return attrchg_buff;
}

DebugAttributeChanger bkgcolor(const uint8_t attribute) {
  attrchg_buff.new_attr = (uint8_t) attribute;
  attrchg_buff.mask = 0x70;

  return attrchg_buff;
}

DebugAttributeChanger txtcolor(const uint8_t attribute) {
  attrchg_buff.new_attr = (uint8_t) attribute;
  attrchg_buff.mask = 0x0f;

  return attrchg_buff;
}

DebugBreakpoint bp() {
  return bp_buff;
}

DebugBreakpoint bp_streg(const uint64_t rax, const uint64_t rbx, const uint64_t rcx, const uint64_t rdx) {
  bp_buff.rax = (uint64_t) rax;
  bp_buff.rbx = (uint64_t) rbx;
  bp_buff.rcx = (uint64_t) rcx;
  bp_buff.rdx = (uint64_t) rdx;

  return bp_buff;
}

DebugCursorMover move_to(const int x, const int y) {
  cursmov_buff.type = ABSOLUTE;
  cursmov_buff.col_off = (int) x;
  cursmov_buff.row_off = (int) y;

  return cursmov_buff;
}

DebugCursorMover move_rel(const int x, const int y) {
  cursmov_buff.type = RELATIVE;
  cursmov_buff.col_off = (int) x;
  cursmov_buff.row_off = (int) y;

  return cursmov_buff;
}

DebugNumberBaseChanger numberbase(const DebugNumberBase new_base) {
  numchg_buff.new_base = (DebugNumberBase) new_base;

  return numchg_buff;
}

DebugPadder pad_status(const bool padding_status) {
  padder_buff.padding_on = (bool) padding_status;
  padder_buff.padsize = -1; //Do not change padding

  return padder_buff;
}

DebugPadder pad_size(unsigned int padsize) {
  if(padsize > NUMBER_OF_COLS) padsize = NUMBER_OF_COLS; //More digits than the number of cols is probably an error.
  padder_buff.padsize = padsize;

  return padder_buff;
}

DebugScroller cls() {
    scroller_buff.amount = NUMBER_OF_ROWS;

    return scroller_buff;
}

DebugScroller scroll(unsigned int amount) {
    scroller_buff.amount = amount;

    return scroller_buff;
}

DebugWindower set_window(const DebugWindow window) {
  windower_buff.window.startx = min(max(0, min(window.startx, window.endx)), NUMBER_OF_COLS-1);
  windower_buff.window.starty = min(max(0, min(window.starty, window.endy)), NUMBER_OF_COLS-1);
  windower_buff.window.endx = min(max(0, max(window.startx, window.endx)), NUMBER_OF_COLS-1);
  windower_buff.window.endy = min(max(0, max(window.starty, window.endy)), NUMBER_OF_COLS-1);
  windower_buff.window.border = window.border;

  return windower_buff;
}

void DebugOutput::check_boundaries() {
    if(col>window.endx-window.startx) {
        ++row;
        col=0;
    }
    if(row>window.endy-window.starty) scroll(row+window.starty-window.endy);
}


void DebugOutput::clear_border(const DebugWindow window) {
  DebugRect border_line;

  if(window.border) {
    //Left border
    if(window.startx>0) {
      border_line.startx=window.startx-1;
      border_line.starty=window.starty;
      if(window.starty>0) --border_line.starty; //Remove topleft corner too
      border_line.endx=window.startx-1;
      border_line.endy=window.endy;
      if(window.endy<NUMBER_OF_ROWS-1) ++border_line.endy; //Remove bottomleft corner too
      clear_rect(border_line);
    }

    //Top border
    if(window.starty>0) {
      border_line.startx=window.startx;
      border_line.starty=window.starty-1;
      border_line.endx=window.endx;
      if(window.endx<NUMBER_OF_COLS-1) ++border_line.endx; //Remove topright corner too
      border_line.endy=window.starty-1;
      clear_rect(border_line);
    }

    //Right border
    if(window.endx<NUMBER_OF_COLS-1) {
      border_line.startx=window.endx+1;
      border_line.starty=window.starty;
      border_line.endx=window.endx+1;
      border_line.endy=window.endy;
      if(window.endy<NUMBER_OF_ROWS-1) ++border_line.endy; //Remove bottomright corner too
      clear_rect(border_line);
    }

    //Bottom border
    if(window.endy<NUMBER_OF_ROWS-1) {
      border_line.startx=window.startx;
      border_line.starty=window.endy+1;
      border_line.endx=window.endx;
      border_line.endy=window.endy+1;
      clear_rect(border_line);
    }
  }
}

void DebugOutput::clear_oldwindow(const DebugWindow old_window, const DebugWindow new_window) {
  DebugRect rect;
  uint8_t overlap = 0, horz_overlap = 0, vert_overlap = 0;

  //Detect if there is overlap between both windows
  if((old_window.startx>new_window.startx-1 && old_window.startx<new_window.endx+1)
      || (old_window.endx>new_window.startx-1 && old_window.endx<new_window.endx+1)) {
    horz_overlap=1;
  }
  if((old_window.starty>new_window.starty-1 && old_window.starty<new_window.endy+1)
      || (old_window.endy>new_window.starty-1 && old_window.endy<new_window.endy+1)) {
    vert_overlap=1;
  }
  overlap = horz_overlap && vert_overlap;

  if(!overlap) {
    //If there's no overlap, just fill the old window.
    rect.startx = old_window.startx;
    rect.starty = old_window.starty;
    rect.endx = old_window.endx;
    rect.endy = old_window.endy;
    clear_rect(rect);
  } else {
    //If there's overlap, erase the non-overlapping parts of the old window
    //  * Left + Topleft + Bottomleft parts
    rect.startx = old_window.startx;
    rect.starty = old_window.starty;
    rect.endx = new_window.startx-1;
    rect.endy = old_window.endy;
    clear_rect(rect);

    //  * Top + Topright parts
    rect.startx = new_window.startx;
    rect.starty = old_window.starty;
    rect.endx = old_window.endx;
    rect.endy = new_window.starty-1;
    clear_rect(rect);

    //  * Right + Bottomright parts
    rect.startx = new_window.endx+1;
    rect.starty = new_window.starty;
    rect.endx = old_window.endx;
    rect.endy = old_window.endy;
    clear_rect(rect);

    //  * Bottom part
    rect.startx = new_window.startx;
    rect.starty = new_window.endy+1;
    rect.endx = new_window.endx;
    rect.endy = old_window.endy;
    clear_rect(rect);
  }
}

void DebugOutput::clear_rect(const DebugRect rect) {
  unsigned int col, row, offset;

  for(row=rect.starty; row <= rect.endy; ++row) {
    for(col=rect.startx; col<=rect.endx; ++col) {
      offset = get_offset(col,row);
      buffer[offset]=' ';
      buffer[offset+1]=attribute;
    }
  }
}

void DebugOutput::copy_rect(const DebugRect rect,
                            unsigned int dest_col,
                            unsigned int dest_row) {
  unsigned int off_col, off_row, source_offset, dest_offset;
  const unsigned int col_length = rect.endx-rect.startx+1;
  const unsigned int row_length = rect.endy-rect.starty+1;

  if((dest_row<rect.starty) || ((dest_col<rect.startx) && (dest_row==rect.starty))) {
    //Fill destination region row by row, from top to bottom
    for(off_row=0; off_row < row_length; ++off_row) {
      for(off_col=0; off_col < col_length; ++off_col) {
        source_offset = get_offset(rect.startx+off_col, rect.starty+off_row);
        dest_offset = get_offset(dest_col+off_col, dest_row+off_row);
        buffer[dest_offset] = buffer[source_offset];
        buffer[dest_offset+1] = buffer[source_offset+1];
      }
    }
  }

  if(dest_row>rect.starty) {
    //Fill destination region row by row, from bottom to top
    for(off_row=row_length-1; off_row<row_length; --off_row) {
      for(off_col=0; off_col < col_length; ++off_col) {
        source_offset = get_offset(rect.startx+off_col, rect.starty+off_row);
        dest_offset = get_offset(dest_col+off_col, dest_row+off_row);
        buffer[dest_offset] = buffer[source_offset];
        buffer[dest_offset+1] = buffer[source_offset+1];
      }
    }
  }

  if((dest_col>rect.startx) && (dest_row == rect.starty)) {
    //Fill destination region column by column, from right to left
    for(off_col=col_length-1; off_col<col_length; --off_col) {
      for(off_row=0; off_row<col_length; ++off_row) {
        source_offset = get_offset(rect.startx+off_col, rect.starty+off_row);
        dest_offset = get_offset(dest_col+off_col, dest_row+off_row);
        buffer[dest_offset] = buffer[source_offset];
        buffer[dest_offset+1] = buffer[source_offset+1];
      }
    }
  }
}

void DebugOutput::cursor_movabs(const unsigned int acol, const unsigned int arow) {
    col = acol;
    row = arow;
    check_boundaries();
}

void DebugOutput::fill_border(const DebugWindow window) {
  DebugRect border_line;
  const unsigned char* border_characters = border_array(window.border);
  int offset;

  if(window.border) {
    //Left border
    if(window.startx>0) {
      border_line.startx=window.startx-1;
      border_line.starty=window.starty;
      border_line.endx=window.startx-1;
      border_line.endy=window.endy;
      fill_rect(border_line, border_characters[BOR_LEFT]);

      //Topleft corner
      if(window.starty>0) {
        offset = get_offset(window.startx-1, window.starty-1);
        buffer[offset] = border_characters[BOR_TOPLEFT];
        buffer[offset+1] = attribute;
      }

      //Bottomleft corner
      if(window.endy<NUMBER_OF_ROWS-1) {
        offset = get_offset(window.startx-1, window.endy+1);
        buffer[offset] = border_characters[BOR_BOTTOMLEFT];
        buffer[offset+1] = attribute;
      }
    }

    //Top border
    if(window.starty>0) {
      border_line.startx=window.startx;
      border_line.starty=window.starty-1;
      border_line.endx=window.endx;
      border_line.endy=window.starty-1;
      fill_rect(border_line, border_characters[BOR_TOP]);

      //Topright corner
      if(window.endx<NUMBER_OF_COLS-1) {
        offset = get_offset(window.endx+1, window.starty-1);
        buffer[offset] = border_characters[BOR_TOPRIGHT];
        buffer[offset+1] = attribute;
      }
    }

    //Right border
    if(window.endx<NUMBER_OF_COLS-1) {
      border_line.startx=window.endx+1;
      border_line.starty=window.starty;
      border_line.endx=window.endx+1;
      border_line.endy=window.endy;
      fill_rect(border_line, border_characters[BOR_RIGHT]);

      //Bottomright corner
      if(window.endy<NUMBER_OF_ROWS-1) {
        offset = get_offset(window.endx+1, window.endy+1);
        buffer[offset] = border_characters[BOR_BOTTOMRIGHT];
        buffer[offset+1] = attribute;
      }
    }

    //Bottom border
    if(window.endy<NUMBER_OF_ROWS-1) {
      border_line.startx=window.startx;
      border_line.starty=window.endy+1;
      border_line.endx=window.endx;
      border_line.endy=window.endy+1;
      fill_rect(border_line, border_characters[BOR_BOTTOM]);
    }
  }
}

void DebugOutput::fill_rect(const DebugRect rect, const char character) {
    unsigned int col, row, offset;

    for(row=rect.starty; row <= rect.endy; ++row) {
        for(col=rect.startx; col<=rect.endx; ++col) {
            offset=get_offset(col,row);
            buffer[offset]=character;
            buffer[offset+1]=attribute;
        }
    }
}

void DebugOutput::scroll(const unsigned int amount) {
    if(amount < window.endy-window.starty+1) {
        DebugRect rect(window.startx, window.starty+amount, window.endx, window.endy);
        copy_rect(rect, window.startx, window.starty);
        rect.starty = window.endy-amount+1;
        clear_rect(rect);
        row-=amount;
    } else {
        clear_window();
    }
}

DebugOutput::DebugOutput(const DebugWindow& using_window):
  attribute(TXT_DEFAULT),
  buffer((char*) 0xb8000),
  col(0),
  row(0),
  number_base(DECIMAL),
  padsize(0),
  tab_size(8),
  window(topleftcorner_win),
  padding_on(false)
{
  *this << set_window((DebugWindow) using_window);
}

DebugOutput& DebugOutput::operator<<(const bool input) {
    if(input) *this << "true";
    else *this << "false";

    return *this;
}

DebugOutput& DebugOutput::operator<<(const char input) {
    unsigned int offset;

    if(input!=0 && input!='\n' && input!='\t') { //Some characters are not printed
        check_boundaries();
        offset = get_offset();
        buffer[offset] = input;
        buffer[offset+1] = attribute;
        ++col;
    }

    if(input=='\n') {
        check_boundaries();
        do {
            offset = get_offset();
            buffer[offset] = ' ';
            buffer[offset+1] = attribute;
            ++col;
            check_boundaries();
        }   while(col!=0);
    }

    if(input=='\t') {
        check_boundaries();
        while(col%tab_size!=0) {
            offset = get_offset();
            buffer[offset] = ' ';
            buffer[offset+1] = attribute;
            ++col;
            check_boundaries();
        }
    }

    return *this;
}


DebugOutput& DebugOutput::operator<<(const char* input) {
    for(unsigned int index=0; input[index]; ++index) {
        *this << input[index];
    }

    return *this;
}

DebugOutput& DebugOutput::operator<<(const double input) {
    uint64_t truncated;
    double decimals;

    //First display the integer part of the number
    truncated = input;
    *this << truncated << ".";

    //Then display the decimals
    decimals = input - truncated;
    while(decimals) {
        decimals*= 10;
        truncated = decimals;
        *this << truncated;
        decimals-=truncated;
    }

    return *this;
}

DebugOutput& DebugOutput::operator<<(const int64_t input) {
    uint64_t absolute_value;

    if(input<0 && number_base==DECIMAL) {
    absolute_value = (int64_t) -input;
    *this << '-' << absolute_value;
    } else {
    absolute_value = (int64_t) input;
    *this << absolute_value;
    }

    return *this;
}

DebugOutput& DebugOutput::operator<<(const KString& input) {
    for(uint32_t index = 0; index < input.length(); ++index) {
        *this << input[index];
    }

    return *this;
}

DebugOutput& DebugOutput::operator<<(const uint64_t input) {
    static char buffer_reverse[64];
    static char buffer_direct[67];
    uint64_t tmp;
    unsigned int index, offset, digit, length;

    //Performing int->str transformation, with digits in reverse order
    for(tmp=input, index=0; tmp; tmp/=number_base, ++index) {
        digit = tmp%number_base;
        if(digit<10) {
            buffer_reverse[index]='0'+digit;
        } else {
            buffer_reverse[index]='a'+(digit-10);
        }
    }
    length = index;
    if(length==0) {
        buffer_reverse[0]='0';
        length=1;
    }

    //Add a prefix to the final string if needed
    offset = 0;
    switch(number_base) {
        case BINARY:
            buffer_direct[0]='b';
            buffer_direct[1]='i';
            offset=2;
            break;
        case OCTAL:
            buffer_direct[0]='o';
            buffer_direct[1]='c';
            offset=2;
            break;
        case HEXADECIMAL:
            buffer_direct[0]='0';
            buffer_direct[1]='x';
            offset=2;
            break;
        default:
            break;
    }

    //Get the string in the right order
    for(index=offset; index<length+offset; ++index) {
        buffer_direct[index]=buffer_reverse[length+offset-index-1];
    }

    //Add up blank spaces if required
    if(padding_on) {
        unsigned int limit = offset;
        if((padsize > 0) && (padsize < offset+64)) {
            limit = padsize;
        } else {
            switch(number_base) {
                case BINARY:
                    limit+= 64;
                    break;
                case OCTAL:
                    limit+= 22;
                    break;
                case DECIMAL:
                    limit+= 20;
                    break;
                case HEXADECIMAL:
                    limit+= 16;
                    break;
                default:
                    limit = 0;
            }
        }
        for(; index<limit; ++index) buffer_direct[index]=' ';
    }

    //Add the terminating NULL
    buffer_direct[index]=0;

    //Display the string
    *this << buffer_direct;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const KernelInformation& input) {
    unsigned int index;
    DebugNumberBase tmp = number_base;

    *this << numberbase(HEXADECIMAL) << endl;
    *this << "Location           | Size               | Nat  | Misc" << endl;
    *this << "-------------------+--------------------+------+------------------------------";

    for(index=0; index<input.kmmap_length; ++index) {
        *this << endl << input.kmmap[index];
    }
    *this << numberbase(tmp);

    return *this;
}

DebugOutput& DebugOutput::operator<<(const KernelMMapItem& input) {
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << input.location << " | " << input.size;

    switch(input.nature) {
        case NATURE_FRE:
            *this << " | FREE | ";
            break;
        case NATURE_RES:
            *this << " | RES  | ";
            break;
        case NATURE_BSK:
            *this << " | BSK  | ";
            break;
        case NATURE_KNL:
            *this << " | KNL  | ";
            break;
        case NATURE_MOD:
            *this << " | MOD  | ";
            break;
        default:
            *this << " | ???  | ";
    }
    *this << input.name;

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    return *this;
}

DebugOutput& DebugOutput::operator<<(const RAMChunk& input) {
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    RAMChunk* map = (RAMChunk*) &input;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "Location           | Size               | Own. | Misc" << endl;
    *this << "-------------------+--------------------+------+------------------------------";

    do {
        *this << endl << map->location << " | " << map->size;
        if(map->has_owner(PID_KERNEL)) {
            *this << " | KERN | ";
        } else {
            if(map->has_owner(PID_INVALID)) {
                *this << " | none | ";
            } else {
                *this << " | OTH. | ";
            }
        }
        if(map->allocatable) *this << "ALLOCA ";
        if(map->next_buddy) {
            *this << "BUDDY-" << pad_status(false) << map->next_buddy->location;
            *this << pad_status(true);
        }
        map = map->next_mapitem;
    } while(map);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;
    return *this;
}

DebugOutput& DebugOutput::operator<<(const RAMManagerProcess& input) {
    RAMManagerProcess* list = (RAMManagerProcess*) &input;
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "PID        | RAM usage          | Maximum RAM usage  | Mutex" << endl;
    *this << "-----------+--------------------+--------------------+-------------------------";

    do {
        *this << endl << pad_size(10) << list->identifier;
        *this << pad_size(18) << " | " << list->memory_usage << " | ";
        *this << list->memory_cap << " | ";
        if(list->mutex.state()) {
            *this << "available";
        } else {
            *this << "BUSY";
        }
        list = list->next_item;
    } while(list);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const PageChunk& input) {
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    PageChunk* map = (PageChunk*) &input;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "Location           | Size               | Perm. | Target&Misc" << endl;
    *this << "-------------------+--------------------+-------+------------------------------";

    do {
        *this << endl << map->location << " | " << map->size << " | ";

        if(map->flags & PAGE_FLAG_R) {
            *this << 'R';
        } else {
            *this << '-';
        }
        if(map->flags & PAGE_FLAG_W) {
            *this << 'W';
        } else {
            *this << '-';
        }
        if(map->flags & PAGE_FLAG_X) {
            *this << 'X';
        } else {
            *this << '-';
        }
        if(map->flags & PAGE_FLAG_A) {
            *this << 'A';
        } else {
            *this << '-';
        }
        if(map->flags & PAGE_FLAG_K) {
            *this << "K | ";
        } else {
            *this << "- | ";
        }

        if(map->points_to) {
            *this << pad_status(false) << (uint64_t) map->points_to;
            *this << pad_status(true);
        }
        if(map->next_buddy) {
            *this << " BUD-" << pad_status(false) << (uint64_t) map->next_buddy;
            *this << pad_status(true);
        }

        map = map->next_mapitem;
    } while(map);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const PagingManagerProcess& input) {
    PagingManagerProcess* list = (PagingManagerProcess*) &input;
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "PID        | Map location       | PML4T location     | Mutex" << endl;
    *this << "-----------+--------------------+--------------------+-------------------------";

    do {
        *this << endl << pad_size(10) << list->identifier;
        *this << pad_size(18) << " | " << (uint64_t) list->map_pointer << " | ";
        *this << list->pml4t_location << " | ";
        if(list->mutex.state()) {
            *this << "available";
        } else {
            *this << "BUSY";
        }
        list = list->next_item;
    } while(list);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const MemoryChunk& input) {
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    MemoryChunk* map = (MemoryChunk*) &input;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "Location           | Size               | Belongs to" << endl;
    *this << "-------------------+--------------------+-------------------------------------";

    do {
        *this << endl << map->location << " | " << map->size << " | ";
        if(map->belongs_to) {
            dbgout << "BEL-" << (uint64_t) map->belongs_to;
        }
        map = map->next_item;
    } while(map);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;
    return *this;
}

DebugOutput& DebugOutput::operator<<(const MallocProcess& input) {
    MallocProcess* list = (MallocProcess*) &input;
    bool tmp_padding = padding_on;
    unsigned int tmp_padsize = padsize;
    DebugNumberBase tmp = number_base;

    *this << pad_status(true);
    *this << pad_size(18);
    *this << numberbase(HEXADECIMAL) << endl;
    *this << "PID        | Busy map location  | Free map location  | Mutex" << endl;
    *this << "-----------+--------------------+--------------------+-------------------------";

    do {
        *this << endl << pad_size(10) << list->identifier;
        *this << pad_size(18) << " | " << (uint64_t) list->busy_map << " | ";
        *this << (uint64_t) list->free_map << " | ";
        if(list->mutex.state()) {
            *this << "Available";
        } else {
            *this << "BUSY";
        }
        list = list->next_item;
    } while(list);

    if(!tmp_padding) *this << pad_status(false);
    if(tmp_padsize) *this << pad_size(tmp_padsize);
    *this << numberbase(tmp) << endl;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugAttributeChanger& manipulator) {
    uint8_t final_attr = manipulator.new_attr & manipulator.mask;
    final_attr += attribute & ~(manipulator.mask);
    attribute = final_attr;
    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugCursorMover& manipulator) {
    switch(manipulator.type) {
        case ABSOLUTE:
            col=manipulator.col_off;
            row=manipulator.row_off;
            break;
        case RELATIVE:
            col+=manipulator.col_off;
            row+=manipulator.row_off;
            break;
    }
    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugBreakpoint& manipulator) {
    __asm__ volatile ("mov %0, %%rax; mov %1, %%rbx; mov %2, %%rcx; mov %3, %%rdx; xchg %%bx, %%bx"
                   :
                   :"m" (manipulator.rax),
                    "m" (manipulator.rbx),
                    "m" (manipulator.rcx),
                    "m" (manipulator.rdx)
                   :"%rax", "%rbx", "%rcx", "%rdx");
    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugNumberBaseChanger& manipulator) {
    number_base = manipulator.new_base;
    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugPadder& manipulator) {
    padding_on = manipulator.padding_on;
    if(manipulator.padsize>=0) padsize = manipulator.padsize;

    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugScroller& manipulator) {
    scroll(manipulator.amount);

    return *this;
}

DebugOutput& DebugOutput::operator<<(const DebugWindower& manipulator) {
    int col_size, row_size;

    //Erase old border (if any)
    clear_border(window);

    //Copy contents of the old window in the new one
    col_size = min(manipulator.window.endx-manipulator.window.startx, window.endx-window.startx);
    row_size = min(manipulator.window.endy-manipulator.window.starty, window.endy-window.starty);
    DebugRect rect(window.startx, window.starty, window.startx+col_size, window.starty+row_size);
    copy_rect(rect, manipulator.window.startx, manipulator.window.starty);

    //Clear old window and uninitialized parts of the new one
    clear_oldwindow(window, manipulator.window);
    rect.startx = manipulator.window.startx+col_size+1;
    rect.endx = manipulator.window.endx;
    rect.starty = manipulator.window.starty;
    rect.endy = manipulator.window.starty+row_size;
    clear_rect(rect);
    rect.startx = manipulator.window.startx;
    rect.starty = manipulator.window.starty+row_size+1;
    rect.endx = manipulator.window.endx;
    rect.endy = manipulator.window.endy;
    clear_rect(rect);

    //Draw the new border (if any)
    fill_border(manipulator.window);

    //Set new window
    window.startx = manipulator.window.startx;
    window.starty = manipulator.window.starty;
    window.endx = manipulator.window.endx;
    window.endy = manipulator.window.endy;
    window.border = manipulator.window.border;

    return *this;
}
