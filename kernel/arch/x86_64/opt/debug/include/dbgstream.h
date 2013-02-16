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

#ifndef _DBGSTREAM_H_
#define _DBGSTREAM_H_

#include <dbgsupport.h>
#include <deprecated/KAsciiString.h>
#include <KernelInformation.h>
#include <mallocator_support.h>
#include <ram_support.h>
#include <paging_support.h>
#include <stdint.h>
#include <x86paging.h>


//*******************************************************
//***************** TEXT ATTRIBUTE BITS *****************
//*******************************************************

typedef uint8_t TxtAttr;
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

const TxtAttr TXT_DEFAULT = TXT_LIGHTGRAY;

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

//*******************************************************
//******************* OTHER CONSTANTS *******************
//*******************************************************

//Video memory caracteristics
const unsigned int NUMBER_OF_COLS = 80;
const unsigned int NUMBER_OF_ROWS = 25;

//*******************************************************
//***************** STREAM MANIPULATORS *****************
//*******************************************************

//Main manipulator class
class DebugManipulator {};

//This manipulator is used to change text attributes
class DebugAttributeChanger : DebugManipulator {
    public:
        TxtAttr new_attr; //New attribute being set
        TxtAttr mask; //Allows one to specify which bits in the new attribute are effective
};
DebugAttributeChanger attrset(const TxtAttr attribute);
DebugAttributeChanger blink(const bool blink_status);
DebugAttributeChanger bkgcolor(const TxtAttr attribute);
DebugAttributeChanger txtcolor(const TxtAttr attribute);

//This manipulator is used to move the text cursor on screen
enum DebugCursorMoverType {ABSOLUTE, RELATIVE};
class DebugCursorMover : DebugManipulator {
    public:
        DebugCursorMoverType type;
        int col_off;
        int row_off;
};
DebugCursorMover move_to(const int x, const int y);
DebugCursorMover move_rel(const int x, const int y);

//This manipulator is used to trigger a breakpoint from Bochs, while maybe leaving some interesting
//information in the registers
class DebugBreakpoint : DebugManipulator {
    public:
        uint64_t rax;
        uint64_t rbx;
        uint64_t rcx;
        uint64_t rdx;
};
DebugBreakpoint bp();
DebugBreakpoint bp_streg(const uint64_t rax,
                          const uint64_t rbx = 0,
                          const uint64_t rcx = 0,
                          const uint64_t rdx = 0);

//This manipulator is used to change the base in which number are displayed (ie binary, decimal...)
class DebugNumberBaseChanger : DebugManipulator {
    public:
        DebugNumberBase new_base;
};
DebugNumberBaseChanger numberbase(const DebugNumberBase new_base);

//This manipulator is used to set up padding when printing numbers.
//A padding of 5 means that if the final number is less than 5 digits long,
//spaces will be added to keep string length constant (ex : 512 -> "512  ").
//A padding of 0 means that maximum padding is used.
class DebugPadder : DebugManipulator {
    public:
        bool padding_on;
        int padsize;
};
DebugPadder pad_status(const bool padding_status);
DebugPadder pad_size(unsigned int padsize);

//This manipulator is used to scroll on-screen text
class DebugScroller : DebugManipulator {
    public:
        unsigned int amount;
};
DebugScroller cls();
DebugScroller scroll(unsigned int amount);

//This manipulator is used to restrict debug I/O to a specific
//region of the screen (called a window). Content will be kept
//around as long as resizing the window does not make it move out.
class DebugWindower : DebugManipulator {
    public:
        DebugWindow window;
};
DebugWindower set_window(const DebugWindow window);

//*******************************************************
//****************** MAIN DEBUG OUTPUT ******************
//*******************************************************

//The main debug output class...
class DebugOutput {
    private:
        //Current state
        TxtAttr attribute; //Text attributes (see above)
        volatile char* buffer; //The buffer in which text is written
        unsigned int col; //Current cursor position : column...
        unsigned int row; //...and row
        DebugNumberBase number_base; //Base in which we count : octal, binary, hexa, decimal...
        unsigned int padsize; //Zero padding in digits (see DebugZeroExtender description above)
        unsigned int tab_size; //Tabulation alignment in characters
        DebugWindow window; //The window we use (see doc)
        bool padding_on; //Whether numbers must be kept above a certain length (see doc)
        //Internal functions
        void check_boundaries(); //Check that the text cursor has not scrolled beyond the limits
                                 //of the screen, correct it so that we're ready to write.
        void clear_border(const DebugWindow window); //Clear the border of a window.
        void clear_oldwindow(const DebugWindow old_window,  //Clear what remains of an old window
                             const DebugWindow new_window); //after a new one has been drawn
        void clear_rect(const DebugRect rect); //Clear the contents of a rectangle on screen
        void clear_window() {clear_rect((DebugRect&) window); col=0; row=0;} //Clear the current window
        void copy_rect(const DebugRect rect,    //Copy the contents of a specified rectangle
                       unsigned int dest_col,   //somewhere else on the screen.
                       unsigned int dest_row);  //Overlap allowed.
        void cursor_movabs(const unsigned int acol,  //Move the text cursor at a specified position
                           const unsigned int arow); //on screen.
        void fill_border(const DebugWindow window); //Draw the border of a window
        void fill_rect(const DebugRect rect,  //Fill a rectangle with the specified character
                       const char character);
        unsigned int get_offset() const { //Returns the offset in character buffer corresponding to current col/row
            return get_offset(col+window.frame.startx, row+window.frame.starty);}
        unsigned int get_offset(const int col, const int row) const {
            return 2*col+2*NUMBER_OF_COLS*row;}
        void scroll(const unsigned int amount);
  public:
    DebugOutput(const DebugWindow& window);
    //Functions displaying standard types
    DebugOutput& operator<<(const bool input);
    DebugOutput& operator<<(const char input);
    DebugOutput& operator<<(const char* input);
    DebugOutput& operator<<(const double input);
    DebugOutput& operator<<(const int input) {int64_t tmp=input; *this << tmp; return *this;}
    DebugOutput& operator<<(const int64_t input);
    DebugOutput& operator<<(const KAsciiString& input);
    DebugOutput& operator<<(const unsigned int input) {uint64_t tmp=input; *this << tmp; return *this;}
    DebugOutput& operator<<(const uint64_t input);
    DebugOutput& operator<<(const void* ptr); //This is just to make the compiler fail when trying
                                              //to output a pointer, instead of casting it to bool
    //Functions displaying memory management's custom types
    DebugOutput& operator<<(const KernelInformation& input); //Displays only the memory map atm
    DebugOutput& operator<<(const KernelMMapItem& input);
    DebugOutput& operator<<(const RamChunk& input);
    DebugOutput& operator<<(const RamManagerProcess& input);
    DebugOutput& operator<<(const PageChunk& input);
    DebugOutput& operator<<(const PagingManagerProcess& input);
    DebugOutput& operator<<(const MemoryChunk& input);
    DebugOutput& operator<<(const MallocProcess& input);
    //Manipulator functions
    DebugOutput& operator<<(const DebugAttributeChanger& manipulator);
    DebugOutput& operator<<(const DebugBreakpoint& manipulator);
    DebugOutput& operator<<(const DebugCursorMover& manipulator);
    DebugOutput& operator<<(const DebugNumberBaseChanger& manipulator);
    DebugOutput& operator<<(const DebugPadder& manipulator);
    DebugOutput& operator<<(const DebugScroller& manipulator);
    DebugOutput& operator<<(const DebugWindower& manipulator);
};

//*******************************************************
//********************* GLOBAL VARS *********************
//*******************************************************

extern const DebugRect screen_rect; //This rectangle represents the whole screen
extern const DebugWindow bottom_oneline_win; //A one-line debugging window in the bottom (for command prompts)
extern const DebugWindow bottom_fivelines_win; //An small debugging window
extern const DebugWindow bottom_fifteenlines_win; //A big debugging window
extern const DebugWindow top_oneline_win; //Same as above, but on top of the screen
extern const DebugWindow top_fivelines_win;
extern const DebugWindow top_fifteenlines_win;
extern const DebugWindow screen_win; //The whole screen, with a "none" border
extern DebugOutput dbgout; //An debug output targetting the whole screen

#endif
