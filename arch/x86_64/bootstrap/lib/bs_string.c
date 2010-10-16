/*  Bootstrap kernel's string manipulation routines

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

#include "bs_string.h"

int strlen(const char* str) {
  int i;
  for(i=0; str[i]; ++i);
  return i;
}

int strcmp(const char* str1, const char* str2) {
  int i;
  
  for(i=0; str1[i] && str2[i]; ++i) {
    if(str1[i]>str2[i]) return 1;
    else if(str1[i]<str2[i]) return -1;
  }
  if(str1[i]==str2[i]) return 0;
  
  if(str1[i]>0) return 1;
  else return -1;
}

void* memcpy(void* destination, const void* source, const unsigned int num) {
  unsigned int i;
  char *dest = (char*) destination, *sourc = (char*) source;
  
  for(i=0; i<num; ++i) {
    dest[i]=sourc[i];
  }
  
  return destination;
}

void* memset(void* ptr, int value, const unsigned int num) {
  unsigned int i;
  char* dest = (char*) ptr;
  char val;
  
  if((value>255) || (value<0)) return 0;
  else val = (char) value;
  
  for(i=0; i<num; ++i) {
    dest[i]=val;
  }
  
  return ptr;
}