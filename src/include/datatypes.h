/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
	Data Types/Various Definitions
    Copyright (C) 2003-2004 Sadai Sarmiento
    Copyright (C) 2023-2024 Franck "hitchhikr" Charlet

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

#ifndef _CRUDNES_DEFINITIONS_H
#define _CRUDNES_DEFINITIONS_H

#define APPNAME "crudNES"
#define APPVERSION "v1.4"

#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

typedef signed char INT_8;
typedef unsigned char __UINT_8;
typedef short __INT_16;
typedef unsigned short int __UINT_16;
typedef int __INT_32;
typedef unsigned int __UINT_32;

typedef unsigned short int __BOOL;

#define TRUE 1
#define FALSE 0

typedef union
{
	struct { __UINT_8 L, H; } B;
	__UINT_16 W;
} AWORD;

typedef union
{
	struct { AWORD L, H; } W;
	__UINT_32 DW;
} ADWORD;

typedef struct
{
    AWORD address;
    __UINT_8 data;
} GENIE_6, *LPGENIE_6;

typedef struct
{
    AWORD address;
    __UINT_8 compare;
    __UINT_8 data;
} GENIE_8, *LPGENIE_8;

enum e_page_sizes
{
	_1K_ = 0x400, _2K_ = 0x800, _4K_ = 0x1000, _8K_ = 0x2000,
	_16K_ = 0x4000, _24K_ = 0x6000, _32K_ = 0x8000
};

#ifndef __NEW
	#define __NEW(var,type) \
		var = new type
#endif

#ifndef __NEW_MEM_BLOCK
	#define __NEW_MEM_BLOCK(var,type,size) \
		var = new type [size]
#endif

#ifndef __DELETE
	#define __DELETE(var) \
	{\
		if (var) delete var; \
		var = NULL; \
	}
#endif

#ifndef __DELETE_MEM_BLOCK
	#define __DELETE_MEM_BLOCK(var) \
		if (var) delete [] var
#endif

#endif
