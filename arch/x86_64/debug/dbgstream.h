 /* Iostream-like objects for debugging purposes

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

#ifndef _DBGSTREAM_H_
#define _DBGSTREAM_H_

#include <dbgsupport.h>
#include <hack_stdint.h>
#include <kernel_information.h>
#include <memory_support.h>
#include <physmem.h>


//*******************************************************
//***************** TEXT ATTRIBUTE BITS *****************
//*******************************************************

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
#define TXT_BLINK 0x80

//Background attributes
#define BKG_BLACK 0x00
#define BKG_BLUE 0x10
#define BKG_GREEN 0x20
#define BKG_TURQUOISE 0x30
#define BKG_RED 0x40
#define BKG_PURPLE 0x50
#define BKG_MARROON 0x60
#define BKG_LIGHTGRAY 0x70

//*******************************************************
//******************* OTHER CONSTANTS *******************
//*******************************************************

//Video memory caracteristics
#define NUMBER_OF_COLS 80
#define NUMBER_OF_ROWS 25

//*******************************************************
//***************** STREAM MANIPULATORS *****************
//*******************************************************

//Main manipulator class
class DebugManipulator {};

//This manipulator is used to change text attributes
class DebugAttributeChanger : DebugManipulator {
  public:
    uint8_t new_attr; //New attribute being set
    uint8_t mask; //Allows one to specify which bits in the new attribute are effective
};
DebugAttributeChanger& attrset(const uint8_t attribute);
DebugAttributeChanger& blink(const bool blink_status);
DebugAttributeChanger& bkgcolor(const uint8_t attribute);
DebugAttributeChanger& txtcolor(const uint8_t attribute);

//This manipulator is used to move the text cursor on screen
enum DebugCursorMoverType {ABSOLUTE, RELATIVE};
class DebugCursorMover : DebugManipulator {
  public:
    DebugCursorMoverType type;
    int col_off;
    int row_off;
};
DebugCursorMover& movxy(const unsigned int x, const unsigned int y);

//This manipulator is used to trigger a breakpoint from Bochs, while maybe leaving some interesting
//information in the registers
class DebugBreakpoint : DebugManipulator {
  public:
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
};
DebugBreakpoint& bp();
DebugBreakpoint& bp_streg(const uint64_t rax, const uint64_t rbx = 0, const uint64_t rcx = 0, const uint64_t rdx = 0);

//This manipulator is used to change the base in which number are displayed (ie binary, decimal...)
class DebugNumberBaseChanger : DebugManipulator {
  public:
    DebugNumberBase new_base;
};
DebugNumberBaseChanger& numberbase(const DebugNumberBase new_base);

//This manipulator is used to restrict debug I/O to a specific
//region of the screen (called a window). Content will be kept
//around as long as resizing the window does not make it move out.
class DebugWindower : DebugManipulator {
  public:
    DebugWindow window;
};
DebugWindower& set_window(const DebugWindow window);

//This manipulator allows one to turn on and off the zero-extending
//capabilities of the number printing function...
class DebugZeroExtender : DebugManipulator {
  public:
    bool zero_extend;
};
DebugZeroExtender& zero_extending(const bool zeroext_status);

//*******************************************************
//****************** MAIN DEBUG OUTPUT ******************
//*******************************************************

//The main debug output class...
class DebugOutput {
  private:
    //Current state
    uint8_t attribute;
    char* buffer;
    int col;
    int row;
    DebugNumberBase number_base;
    unsigned int tab_size; //Tabulation alignment in characters
    DebugWindow window;
    bool zero_extend; //Whether numbers must be zero-extended to reach their maximum number of digits.
    //Internal functions
    void check_boundaries() { if(col>window.endx-window.startx) {++row; col=0;}
      if(row>window.endy-window.starty) scroll(row+window.starty-window.endy); }
    void clear_border(const DebugWindow window);
    void clear_oldwindow(const DebugWindow old_window, const DebugWindow new_window); //Clear what remains of an old window after a new one has been drawn
    void clear_rect(const DebugRect rect);
    void clear_window() {clear_rect(window); col=0; row=0;}
    void copy_rect(const DebugRect rect, const int dest_col, const int dest_row);
    void cursor_movabs(const unsigned int acol, const unsigned int arow) {col = acol; row = arow; check_boundaries();}
    void fill_border(const DebugWindow window);
    void fill_rect(const DebugRect rect, const char character);
    unsigned int get_offset() const { //Returns the offset in character buffer corresponding to current col/row
      return get_offset(col+window.startx, row+window.starty);}
    unsigned int get_offset(const int col, const int row) const {return 2*col+2*NUMBER_OF_COLS*row;}
    void scroll(const int amount);
  public:
    DebugOutput(const DebugWindow& window);
    //Functions displaying standard types
    DebugOutput& operator<<(const bool input);
    DebugOutput& operator<<(const char input);
    DebugOutput& operator<<(const char* input);
    DebugOutput& operator<<(const int input) {int64_t tmp=input; *this << tmp; return *this;}
    DebugOutput& operator<<(const int64_t input);
    DebugOutput& operator<<(const unsigned int input) {uint64_t tmp=input; *this << tmp; return *this;}
    DebugOutput& operator<<(const uint64_t input);
    DebugOutput& operator<<(const void* ptr); //This is just to make the compiler crash when trying to output a pointer
    //Function displaying home-made types
    DebugOutput& operator<<(const KernelInformation& input); //Displays only the memory map atm
    DebugOutput& operator<<(const KernelMemoryMap& input);
    DebugOutput& operator<<(const PhyMemMap& input);
    DebugOutput& operator<<(const VirMapList& input);
    DebugOutput& operator<<(const VirMemMap& input);
    //Manipulator functions
    DebugOutput& operator<<(const DebugAttributeChanger& manipulator);
    DebugOutput& operator<<(const DebugBreakpoint& manipulator);
    DebugOutput& operator<<(const DebugCursorMover& manipulator);
    DebugOutput& operator<<(const DebugNumberBaseChanger& manipulator);
    DebugOutput& operator<<(const DebugWindower& manipulator);
    DebugOutput& operator<<(const DebugZeroExtender& manipulator) {zero_extend = manipulator.zero_extend; return *this;}
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