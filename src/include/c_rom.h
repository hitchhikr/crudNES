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

#ifndef _CNESROM_H
#define _CNESROM_H

#include <io.h>
#include <stdio.h>

#include "datatypes.h"

struct SRominformation {

	char *filename;
	__UINT_32 Size;
	void *handle;

	__UINT_32 mapper,
			prg_pages,
            chr_pages,
            trainer,
            mirroring,
            o_sram,
            pal;
};

enum EFileType { __REGULAR = 0, __ZIP = 1 };

class c_nes_rom {

	public:

		c_nes_rom (const char *Path);
		~c_nes_rom (void);

		__BOOL check_header (int PAL);

		__UINT_8 read_byte (__UINT_32);
		void transfer_block (__UINT_8 *, __UINT_32, __UINT_32);
		void close (void);

		SRominformation & information (void) { return info; }

	private:

		EFileType FileType;
		SRominformation info;
};

#endif
