 /* Text-mode video memory manipulation routines

        Copyright (C) 2010    Hadrien Grasland

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA    02110-1301    USA */

#include "txt_videomem.h"
#include <x86asm.h>
#include <bs_string.h>

//Text color attributes
const TxtAttr TXT_BLACK = 0x00;
const TxtAttr TXT_BLUE = 0x01;
const TxtAttr TXT_GREEN = 0x02;
const TxtAttr TXT_TURQUOISE = 0x03;
const TxtAttr TXT_RED = 0x04;
const TxtAttr TXT_PURPLE = 0x05;
const TxtAttr TXT_MARROON = 0x06;
const TxtAttr TXT_LIGHTGRAY = 0x07;
const TxtAttr TXT_DARKGRAY = 0x08;
const TxtAttr TXT_LIGHTBLUE = 0x09;
const TxtAttr TXT_LIGHTGREEN = 0x0A;
const TxtAttr TXT_LIGHTCOBALT = 0x0B;
const TxtAttr TXT_LIGHTRED = 0x0C;
const TxtAttr TXT_LIGHTPURPLE = 0x0D;
const TxtAttr TXT_YELLOW = 0x0E;
const TxtAttr TXT_WHITE = 0x0F;

const TxtAttr TXT_BLINK = 0x80; //Blinking text

//Background color attributes
const TxtAttr BKG_BLACK = 0x00;
const TxtAttr BKG_BLUE = 0x10;
const TxtAttr BKG_GREEN = 0x20;
const TxtAttr BKG_TURQUOISE = 0x30;
const TxtAttr BKG_RED = 0x40;
const TxtAttr BKG_PURPLE = 0x50;
const TxtAttr BKG_MARROON = 0x60;
const TxtAttr BKG_LIGHTGRAY = 0x70;

//The beginning of the video ram
static volatile char* const videoram = (char *) 0xb8000;
//Size of the video ram
static const unsigned int number_of_cols = 80;
static const unsigned int number_of_rows = 25;
//Position of the cursor
static unsigned int cursor_col = 0;
static unsigned int cursor_row = 0;

//Current text attribute
static TxtAttr attr = 0x07; //Initially light gray

void clear_screen() {
    unsigned int i;

    for(i = 0; i<2*number_of_rows*number_of_cols; i+=2) {
        videoram[i]=' ';
        videoram[i+1]=attr;
    }

    cursor_col = 0;
    cursor_row = 0;
}

void clear_rect(const unsigned int left, const unsigned int top, const unsigned int right, const unsigned int bottom) {
    unsigned int x, y;
    int videoram_index;
    unsigned int eff_left = (unsigned int) left,
                             eff_top = (unsigned int) top,
                             eff_right = (unsigned int) right,
                             eff_bottom = (unsigned int) bottom;

    //Make sure that left,right,top,bottom are valid coordinates.
    if(left >= number_of_cols) eff_left = number_of_cols-1;
    if(top >= number_of_rows) eff_top = number_of_rows-1;
    if(right >= number_of_cols) eff_right = number_of_cols-1;
    if(bottom >= number_of_rows) eff_bottom = number_of_rows-1;

    //Fill the rect
    for(y=eff_top; y<=eff_bottom; ++y) {
        for(x=eff_left; x<=eff_right; ++x) {
            videoram_index = 2*(x+number_of_cols*y);
            videoram[videoram_index]=' ';
            videoram[videoram_index+1]=attr;
        }
    }
}

void init_videomem() {
    //This code hides the silly blinking cursor
    /* CRT index port => ask for access to register 0xa ("cursor
         start") */
    outb(0x0a, 0x3d4);

    /* (RBIL Tables 708 & 654) CRT Register 0xa => bit 5 = cursor OFF */
    outb(1 << 5, 0x3d5);
}

void movecur_abs(const unsigned int col, const unsigned int row) {
    if(col < number_of_cols)
        cursor_col = col;
    if(row < number_of_rows)
        cursor_row = row;
}

void movecur_rel(const int col_offset, const int row_offset) {
    unsigned int dest_col = cursor_col + col_offset;
    unsigned int dest_row = cursor_col + col_offset;
    if(dest_col < number_of_cols)
        cursor_col+=col_offset;
    if(dest_row < number_of_rows)
        cursor_row+=row_offset;
}

void print_bin8(const uint8_t integer) {
    char buff[9];
    int i;
    uint8_t tmp = integer;

    for(i=7; i>=0; --i) {
        buff[i] = '0'+tmp%2;
        tmp = tmp/2;
    }
    buff[8]=0;

    print_str(buff);
}

void print_bin32(const uint32_t integer) {
    char buff[33];
    int i;
    uint32_t tmp = integer;

    for(i=31; i>=0; --i) {
        buff[i] = '0'+tmp%2;
        tmp = tmp/2;
    }
    buff[32]=0;

    print_str(buff);
}

void print_bin64(const uint64_t integer) {
    char buff[65];
    int i;
    uint64_t tmp = integer;

    for(i=63; i>=0; --i) {
        buff[i] = '0'+tmp%2;
        tmp = tmp/2;
    }
    buff[64]=0;

    print_str(buff);
}

void print_chr(const char chr) {
    char buff[2];

    buff[0]=chr;
    buff[1]=0;

    print_str(buff);
}

void print_hex8(const uint8_t integer) {
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

    print_str(buff);
}

void print_hex32(const uint32_t integer) {
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

    print_str(buff);
}

void print_hex64(const uint64_t integer) {
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

    print_str(buff);
}

void print_int8(const int8_t integer) {
    uint8_t tmp;

    if(integer<0) {
        print_chr('-');
        tmp = -integer;
    }
    else tmp = integer;

    print_uint8(tmp);
}

void print_int32(const int32_t integer) {
    uint32_t tmp;

    if(integer<0) {
        print_chr('-');
        tmp = -integer;
    }
    else tmp = integer;

    print_uint32(tmp);
}

/* void print_int64(const int64_t integer) {
    uint64_t tmp;

    if(integer<0) {
        print_chr('-');
        tmp = -integer;
    }
    else tmp = integer;

    print_uint64(tmp);
} */

void print_str(const char* str) {
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
            scroll(cursor_row-number_of_rows+1);
        }
    }
}

void print_uint8(const uint8_t integer) {
    char buff[4];
    char reverse_buff[3];
    int i, j;
    uint32_t tmp = integer;

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

    print_str(buff);
}

void print_uint32(const uint32_t integer) {
    char buff[11];
    char reverse_buff[10];
    int i, j;
    uint32_t tmp = integer;

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

    print_str(buff);
}

/* void print_uint64(const uint64_t integer) {
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

    print_str(buff);
} */

void scroll(const int offset) {
    #ifdef DEBUG
        __asm__ volatile ("xchg %bx, %bx");
        clear_screen();
        //The following is used just to skip the "unused parameter" kind of error message
        int a = offset;
        a+=a;
    #else
        unsigned int x, y;
        unsigned int target_index, source_index, absolute_offset;

        if(offset>=0) {
            absolute_offset = (unsigned int) offset;
            // Scrolling more lines than screen height is useless,
            // In that case clearing the screen is just as effective
            if(absolute_offset > number_of_rows) {
                clear_screen();
            } else {
                //Make text move upwards
                for(y=0; y<number_of_rows-absolute_offset; ++y) {
                    for(x=0; x<number_of_cols; ++x) {
                        //Copy line y+offset to line y
                        target_index = 2*(x+number_of_cols*y);
                        source_index = 2*(x+number_of_cols*(y+absolute_offset));
                        videoram[target_index] = videoram[source_index];
                        videoram[target_index+1] = videoram[source_index+1];
                    }
                }

                //Clear text at the bottom
                clear_rect(0, number_of_rows-absolute_offset, number_of_cols-1, number_of_rows-1);

                //Move cursor up, if it goes out of the screen make it move to the upper left corner
                if(cursor_row - absolute_offset < number_of_rows) {
                    movecur_rel(0, -offset);
                } else {
                    movecur_abs(0, 0);
                }
            }
        } else {
            absolute_offset = (unsigned int) -offset;
            //Same as above, but downwards
            // Scrolling more lines than screen height is useless,
            // In that case clearing the screen is just as effective
            int neg_nb_row = number_of_rows;
            neg_nb_row = -neg_nb_row;
            if(offset < neg_nb_row) {
                clear_screen();
            } else {
                //Make text move downwards
                for(y=number_of_rows-1; y>=-absolute_offset; --y) {
                    for(x=0; x<number_of_cols; ++x) {
                        //Copy line y+offset to line y
                        target_index = 2*(x+number_of_cols*y);
                        source_index = 2*(x+number_of_cols*(y-absolute_offset));
                        videoram[target_index] = videoram[source_index];
                        videoram[target_index+1] = videoram[source_index+1];
                    }
                }

                //Clear text at the top
                clear_rect(0, 0, number_of_cols-1, absolute_offset-1);

                // Move cursor down, if it goes out of the screen make it move to
                // the bottom right corner
                if(cursor_row + absolute_offset < number_of_rows) {
                    movecur_rel(0, absolute_offset);
                } else {
                    movecur_abs(number_of_cols-1, number_of_rows-1);
                }
            }
        }
    #endif
}
void set_attr(const TxtAttr new_attr) {
    attr = new_attr;
}
