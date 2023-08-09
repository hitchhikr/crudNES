/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes

    Copyright (C) 2003-2004 Sadai Sarmiento
    Copyright (C) 2023 Franck "hitchhikr" Charlet

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

#ifndef _2XSAI_H

#include "allegro.h"

#define uint32 unsigned long
#define int32 signed long
#define uint16 unsigned short
#define uint8 unsigned char

extern int Init_2xSaI(uint32 BitFormat);

extern void Super2xSaI(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
         BITMAP *dstBitmap, int width, int height);
extern void _2xSaI(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
         BITMAP *dstBitmap, int width, int height);
extern void SuperEagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
         BITMAP *dstBitmap, int width, int height);
extern void Scale_2xSaI(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
         BITMAP *dstBitmap, int width, int height);

#endif