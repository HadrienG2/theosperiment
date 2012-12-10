/*  A dirty workaround for current G++ missing stdint.h
    WARNING : THIS ONLY WORKS BECAUSE x86_64 G++ USES LP64, ANOTHER COMPILER MIGHT FAIL !
    THIS SHOULD NOT, UNDER ANY CIRCUMSTANCES, BE USED VOLUNTARILY AS
    AN STDINT.H REPLACEMENT. INCOMPATILITIES *WILL* OCCUR. YOU ARE WARNED.

        Copyright (C) 2010     Hadrien Grasland

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef _HACK_STDINT_H_
#define _HACK_STDINT_H_

#ifndef __cplusplus
    #include <stdint.h>
#else
    //I wish I could just write #include <stdint.h> here...
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef signed short int16_t;
    typedef unsigned short uint16_t;
    typedef signed int int32_t;
    typedef unsigned int uint32_t;
    typedef signed long int64_t;
    typedef unsigned long uint64_t;
#endif

#endif
