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

#include "txt_videomem.h"
#include "wait.h"
#include <x86asm.h>
#include <bs_string.h>
 
//The beginning of the video ram
static char* const videoram = (char *) 0xb8000;
//Size of the video ram
static const int number_of_cols = 80;
static const int number_of_rows = 25;
//Position of the cursor
static int cursor_col = 0;
static int cursor_row = 0;

//Current text attribute
static char attr = DBG_TXT_LIGHTGRAY;

void dbg_clear_screen() {
  int i;
  
  for(i = 0; i<2*number_of_rows*number_of_cols; i+=2) {
    videoram[i]=' ';
    videoram[i+1]=attr;
  }
  
  cursor_col = 0;
  cursor_row = 0;
}

void dbg_clear_rect(const int left, const int top, const int right, const int bottom) {
  int x, y;
  int videoram_index;
  int eff_left = left, eff_top = top, eff_right = right, eff_bottom = bottom;
  
  //Make sure that left,right,top,bottom are valid coordinates.
  if(left < 0) eff_left = 0;
  else if(left >= number_of_cols) eff_left = number_of_cols-1;
  if(top < 0) eff_top = 0;
  else if(top >= number_of_rows) eff_top = number_of_rows-1;
  if(right < 0) eff_right = 0;
  else if(right >= number_of_cols) eff_right = number_of_cols-1;
  if(bottom < 0) eff_bottom = 0;
  else if(bottom >= number_of_rows) eff_bottom = number_of_rows-1;
  
  //Fill the rect
  for(y=top; y<=bottom; ++y) {
    for(x=left; x<=right; ++x) {
      videoram_index = 2*(x+number_of_cols*y);
      videoram[videoram_index]=' ';
      videoram[videoram_index+1]=attr;
    }
  }
}

void dbg_init_videomem() {
  //This code hides the silly blinking cursor
  /* CRT index port => ask for access to register 0xa ("cursor
     start") */
  outb(0x0a, 0x3d4);

  /* (RBIL Tables 708 & 654) CRT Register 0xa => bit 5 = cursor OFF */
  outb(1 << 5, 0x3d5);
}

void dbg_movecur_abs(const int col, const int row) {
  if((col >= 0) && (col < number_of_cols))
    cursor_col = col;
  if((row >= 0) && (row < number_of_rows))
    cursor_row = row;
}

void dbg_movecur_rel(const int col_offset, const int row_offset) {
  if((cursor_col + col_offset >= 0) && (cursor_col + col_offset < number_of_cols))
    cursor_col+=col_offset;
  if((cursor_row + row_offset >= 0) && (cursor_row + row_offset < number_of_rows))
    cursor_row+=row_offset;
}

void dbg_print_bin8(const uint8_t integer) {
  char buff[9];
  int i;
  uint8_t tmp = integer;
  
  for(i=7; i>=0; --i) {
    buff[i] = '0'+tmp%2;
    tmp = tmp/2;
  }
  buff[8]=0;
  
  dbg_print_str(buff);
}

void dbg_print_bin32(const uint32_t integer) {
  char buff[33];
  int i;
  uint32_t tmp = integer;
  
  for(i=31; i>=0; --i) {
    buff[i] = '0'+tmp%2;
    tmp = tmp/2;
  }
  buff[32]=0;
  
  dbg_print_str(buff);
}

void dbg_print_bin64(const uint64_t integer) {
  char buff[65];
  int i;
  uint64_t tmp = integer;
  
  for(i=63; i>=0; --i) {
    buff[i] = '0'+tmp%2;
    tmp = tmp/2;
  }
  buff[64]=0;
  
  dbg_print_str(buff);
}

void dbg_print_chr(const char chr) {
  char buff[2];
  
  buff[0]=chr;
  buff[1]=0;
  
  dbg_print_str(buff);
}

void dbg_print_hex8(const uint8_t integer) {
  char buff[5];
  int i;
  uint8_t tmp = integer;
  
  buff[0]='0';
  buff[1]='x';
  for(i=3; i>=2; --i) {
    switch(tmp%16) {
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        buff[i] = 'a'+(tmp%16)-10;
        break;
      default:
        buff[i] = '0'+(tmp%16);
    }
    tmp = tmp/16;
  }
  buff[4]=0;
  
  dbg_print_str(buff);
}

void dbg_print_hex32(const uint32_t integer) {
  char buff[11];
  int i;
  uint32_t tmp = integer;
  
  buff[0]='0';
  buff[1]='x';
  for(i=9; i>=2; --i) {
    switch(tmp%16) {
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        buff[i] = 'a'+(tmp%16)-10;
        break;
      default:
        buff[i] = '0'+(tmp%16);
    }
    tmp = tmp/16;
  }
  buff[10]=0;
  
  dbg_print_str(buff);
}

void dbg_print_hex64(const uint64_t integer) {
  char buff[19];
  int i;
  uint64_t tmp = integer;
  
  buff[0]='0';
  buff[1]='x';
  for(i=17; i>=2; --i) {
    switch(tmp%16) {
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        buff[i] = 'a'+(tmp%16)-10;
        break;
      default:
        buff[i] = '0'+(tmp%16);
    }
    tmp = tmp/16;
  }
  buff[18]=0;
  
  dbg_print_str(buff);
}

void dbg_print_int8(const int8_t integer) {
  uint8_t tmp;
  
  if(integer<0) {
    dbg_print_chr('-');
    tmp = -integer;
  }
  else tmp = integer;
  
  dbg_print_uint8(tmp);
}

void dbg_print_int32(const int32_t integer) {
  uint32_t tmp;
  
  if(integer<0) {
    dbg_print_chr('-');
    tmp = -integer;
  }
  else tmp = integer;
  
  dbg_print_uint32(tmp);
}

/* void dbg_print_int64(const int64_t integer) {
  uint64_t tmp;
  
  if(integer<0) {
    dbg_print_chr('-');
    tmp = -integer;
  }
  else tmp = integer;
  
  dbg_print_uint64(tmp);
} */

void dbg_print_str(const char* str) {
  int str_index, videoram_index;
  
  for(str_index = 0; str_index < strlen(str); ++str_index) {
    videoram_index = 2*(cursor_row*number_of_cols + cursor_col);
    
    if(str[str_index] == '\n') {
      //Jump to a new line
      videoram[videoram_index] = ' ';
      videoram[videoram_index+1] = attr;
      ++cursor_col;
      videoram_index+=2;
      while(cursor_col < number_of_cols) {
        videoram[videoram_index] = ' ';
        videoram[videoram_index+1] = attr;
        ++cursor_col;
        videoram_index+=2;
      }
      ++cursor_row;
      cursor_col=0;
    }
    else {
      //Print a character
      videoram[videoram_index] = str[str_index];
      videoram[videoram_index+1] = attr;    
      ++cursor_col;
    }
    
    //Go to next character, check boundaries
    if(cursor_col>=number_of_cols) {
      cursor_col=0;
      ++cursor_row;
    }
    //Scroll if needed
    if(cursor_row>=number_of_rows) {
      dbg_scroll(cursor_row-number_of_rows+1);
    }
  }
}

void dbg_print_uint8(const uint8_t integer) {
  char buff[4];
  char reverse_buff[3];
  int i, j;
  uint32_t tmp = integer;
  
  /* First we get the digits in reverse order, along with number length.
     Then we write them down in reverse order after the sign */
  if(tmp==0) {
    reverse_buff[0]='0';
    i=1;
  } else {
    for(i=0; tmp!=0; ++i) {
      reverse_buff[i] = '0'+(tmp%10);
      tmp = tmp/10;
    }
  }
  //At this stage, i's value is the length of reverse_buff
  for(j=i-1; j>=0; --j) {
    buff[i-j-1]=reverse_buff[j];
  }
  buff[i]=0;
  
  dbg_print_str(buff);
}

void dbg_print_uint32(const uint32_t integer) {
  char buff[11];
  char reverse_buff[10];
  int i, j;
  uint32_t tmp = integer;
  
  /* First we get the digits in reverse order, along with number length.
     Then we write them down in reverse order after the sign */
  if(tmp==0) {
    reverse_buff[0]='0';
    i=1;
  } else {
    for(i=0; tmp!=0; ++i) {
      reverse_buff[i] = '0'+(tmp%10);
      tmp = tmp/10;
    }
  }
  //At this stage, i's value is the length of reverse_buff
  for(j=i-1; j>=0; --j) {
    buff[i-j-1]=reverse_buff[j];
  }
  buff[i]=0;
  
  dbg_print_str(buff);
}

/*
void dbg_print_uint64(const uint64_t integer) {
  char buff[21];
  char reverse_buff[20];
  int i, j;
  uint64_t tmp = integer;
  
  // First we get the digits in reverse order, along with number length.
  // Then we write them down in reverse order after the sign
  if(tmp==0) {
    reverse_buff[0]='0';
    i=1;
  } else {
    for(i=0; tmp!=0; ++i) {
      reverse_buff[i] = '0'+(tmp%10);
      tmp = tmp/10;
    }
  }
  //At this stage, i's value is the length of reverse_buff
  for(j=i-1; j>=0; --j) {
    buff[i-j-1]=reverse_buff[j];
  }
  buff[i]=0;
  
  dbg_print_str(buff);
}*/

void dbg_scroll(const int offset) {
  /* int x, y;
  int target_index, source_index; */
  
  //WARNING : REMOVE THIS AND UNCOMMENT THE REST IF USING FOR NON-DEBUGGING PURPOSES !!!
  dbg_wait(offset);
  __asm__("xchgw %bx, %bx");
  dbg_clear_screen();
  
  /*if(offset>=0) {
    // Scrolling more lines than screen height is useless,
    // In that case clearing the screen is just as effective
    if(offset > number_of_rows) {
      dbg_clear_screen();
    } else {
      //Make text move upwards
      for(y=0; y<number_of_rows-offset; ++y) {
        for(x=0; x<number_of_cols; ++x) {
          //Copy line y+offset to line y
          target_index = 2*(x+number_of_cols*y);
          source_index = 2*(x+number_of_cols*(y+offset));
          videoram[target_index] = videoram[source_index];
          videoram[target_index+1] = videoram[source_index+1];
        }
      }
      
      //Clear text at the bottom
      dbg_clear_rect(0, number_of_rows-offset, number_of_cols-1, number_of_rows-1);
      
      //Move cursor up, if it goes out of the screen make it move to the upper left corner
      if(cursor_row - offset >= 0) {
        dbg_movecur_rel(0, -offset);
      } else {
        dbg_movecur_abs(0, 0);
      }
    }
  } else {
    //Same as above, but downwards
    // Scrolling more lines than screen height is useless,
    // In that case clearing the screen is just as effective
    if(offset < -number_of_rows) {
      dbg_clear_screen();
    } else {      
      //Make text move downwards
      for(y=number_of_rows-1; y>=-offset; --y) {
        for(x=0; x<number_of_cols; ++x) {
          //Copy line y+offset to line y
          target_index = 2*(x+number_of_cols*y);
          source_index = 2*(x+number_of_cols*(y+offset));
          videoram[target_index] = videoram[source_index];
          videoram[target_index+1] = videoram[source_index+1];
        }
      }
      
      //Clear text at the top
      dbg_clear_rect(0, 0, number_of_cols-1, -offset-1);
      
      // Move cursor down, if it goes out of the screen make it move to
      // the bottom right corner
      if(cursor_row - offset >= 0) {
        dbg_movecur_rel(0, -offset);
      } else {
        dbg_movecur_abs(number_of_cols-1, number_of_rows-1);
      }
    }
  } */

}
void dbg_set_attr(const char new_attr) {
  attr = new_attr;
}