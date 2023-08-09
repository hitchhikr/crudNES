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

#include <io.h>
#include <stdio.h>
#include <string.h>
//#include <unzip.h>

#include "include/c_tracer.h"
#include "include/c_ppu.h"
#include "include/c_rom.h"
#include "include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_nes_rom :: c_nes_rom (const char *filename)
{
	info.filename = new char [strlen (filename) + 1];
	strcpy (info.filename, filename);
	//char *extension = strrchr (filename, '.');
	//if (!strcmp (extension, ".zip")) FileType = __ZIP;
	//else 
	FileType = __REGULAR;

	nes->general_log.f_write ("sss", "ROM: Loaded ", filename, "\r\n");
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_nes_rom :: ~c_nes_rom (void)
{
	close ();
	delete [] info.filename;
	nes->general_log.f_write ("s", "ROM: Uninstalled.\r\n");
}

void c_nes_rom :: close (void)
{
	if (info.handle)
	{
		//if (__REGULAR == FileType) 
		fclose ((FILE *)(info.handle));
		/*else
		{
			unzCloseCurrentFile (info.handle);
			unzClose (info.handle);
		}*/
		info.handle = NULL;
	}
}

/******************************************************************************/
/** check_header ()                                                          **/
/**                                                                          **/
/** Checks whether the file is really a game or it's something else.         **/
/**                                                                          **/
/** Supportted formats:                                                      **/
/** -iNES                                                                    **/
/******************************************************************************/

__BOOL c_nes_rom :: check_header (int PAL)
{
	char bNESHeader [16];

/*	if (__REGULAR == FileType)
	{*/
		info.handle = (FILE *)(fopen (info.filename, "rb"));
		if (!info.handle) return TRUE;
		info.Size = filelength (fileno ((FILE *)(info.handle)));
		fread (bNESHeader, 1, 8, (FILE *)(info.handle));
/*	}
	else {
		info.handle = unzOpen (info.filename);
		if (!info.handle) return TRUE;
		unzGoToFirstFile (info.handle);
		unzOpenCurrentFile (info.handle);
		unzReadCurrentFile (info.handle, bNESHeader, 16);
	}*/

	if (!strncmp (bNESHeader, "NES\x1a", 4))
	{
		nes->general_log.f_write ("s", "ROM: iNES format identified.\r\n");

		info.prg_pages	= bNESHeader [4];
		info.chr_pages	= bNESHeader [5];
		info.mirroring = bNESHeader [6] & 1;
		if (bNESHeader [6] & BIT_3) info.mirroring = _2C02_FOURSCREEN_MIRRORING;
		info.o_sram		= (bNESHeader [6] & 2) >> 1;
		info.trainer	= (bNESHeader [6] & 4) >> 2;
		if (bNESHeader [7] & 0xf) bNESHeader [7] = 0;
		info.mapper	= ((bNESHeader [6] >> 4) & 0xf) | (bNESHeader [7] & 0xf0);
        info.pal = PAL;

		nes->general_log.f_write ("s", "ROM: Cartridge information:\r\n");
		nes->general_log.f_write ("sbs", "PRG-ROM pages: ", info.prg_pages, "\r\n");
		nes->general_log.f_write ("sbs", "CHR-ROM pages: ", info.chr_pages, "\r\n");

		nes->general_log.f_write ("s", "mirroring: ");

		switch (info.mirroring)
		{
			case _2C02_HORIZONTAL_MIRRORING: nes->general_log.f_write ("s", "horizontal"); break;
			case _2C02_VERTICAL_MIRRORING: nes->general_log.f_write ("s", "vertical"); break;
			case _2C02_FOURSCREEN_MIRRORING: nes->general_log.f_write ("s", "four-screen"); break;
		}

		nes->general_log.f_write ("s", "\r\n");

		nes->general_log.f_write ("sss", "SRAM: ", (info.o_sram) ? "available" : "not available", "\r\n");
		nes->general_log.f_write ("sss", "Trainer: ", (info.trainer) ? "available" : "not available", ".\r\n");
		nes->general_log.f_write ("sbs", "Mapper: ", info.mapper, "\r\n");

	}
	else return TRUE;
	return FALSE;
}

/******************************************************************************/
/** read_byte ()                                                              **/
/******************************************************************************/

__UINT_8 c_nes_rom :: read_byte (__UINT_32 address)
{
	/*if (__REGULAR == FileType)
	{*/
		fseek ((FILE *)(info.handle), address, SEEK_SET);
		return fgetc ((FILE *)(info.handle));
//	}
}

/******************************************************************************/
/** transfer_block ()                                                         **/
/**                                                                          **/
/** Read a block of bytes into an array.                                     **/
/******************************************************************************/

void c_nes_rom :: transfer_block (__UINT_8 *destination, __UINT_32 where, __UINT_32 length)
{
/*	if (__REGULAR == FileType)
	{*/
		fseek ((FILE *)(info.handle), where, SEEK_SET);
		fread (destination, 1, length, (FILE *)(info.handle));
/*	}
	else
	{
		unzReadCurrentFile (info.handle, destination, length);
	}*/
}

