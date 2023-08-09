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
#include "include/c_mem_block.h"
#include "include/c_tracer.h"

c_mem_block :: c_mem_block (void)
{
	valueBlock = NULL;
}

c_mem_block :: c_mem_block (const __UINT_8 *bSource, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size)
{
	default_construction (size);
	load_from (bSource, where_in_source, dest_where, size);
}

c_mem_block :: c_mem_block (__UINT_32 size)
{
	default_construction (size);
}

c_mem_block :: c_mem_block (c_mem_block *source, __UINT_32 where_in_source, __UINT_32 dest_where)
{
	default_construction (source->size);
	load_from (source, where_in_source, dest_where, size);
}

c_mem_block :: ~c_mem_block (void)
{
	__DELETE_MEM_BLOCK (valueBlock);
}

void c_mem_block :: default_construction (__UINT_32 size)
{
	this->size = size;
	uiAccessMask = size - 1;
	__NEW_MEM_BLOCK (valueBlock, __UINT_8, size);
}

void c_mem_block :: load_from (const __UINT_8 *bSource, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size)
{
	memcpy (valueBlock + dest_where, bSource + where_in_source, size);
}

void c_mem_block :: load_from (c_mem_block *source, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size)
{
	memcpy (valueBlock + dest_where, source->valueBlock + where_in_source, size);
}

void c_mem_block :: load_from (const char *filename, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size)
{
	FILE *handle = fopen (filename, "rb");
	if (!handle) { return; }

	fseek (handle, where_in_source, SEEK_SET);
	fread (valueBlock + dest_where, 1, size, handle);
	
	fclose (handle); 
}

void c_mem_block :: dump_to (char *filename, char *dump_title, __UINT_32 where_in_source, __UINT_32 size, EDumpType type)
{
	c_tracer TDump;
	TDump.set_output_file (filename);
	dump_to (TDump, dump_title, where_in_source, size, type);
}

void c_mem_block :: dump_to (c_tracer &TDump, char *dump_title, __UINT_32 where_in_source, __UINT_32 size, EDumpType type)
{
	switch (type)
	{
		case BINARY:
			TDump.write (valueBlock + where_in_source, size);
		break;

		case BYTE_HEX:
			TDump.f_write ("ss", dump_title, ":\r\n\r\n");

			__UINT_8 valueOnLine = 0x00;
			__UINT_32 uiEndingAddr = where_in_source + size;

			for (;where_in_source < uiEndingAddr; where_in_source ++)
			{
				if (0 == valueOnLine)
				{
					TDump.f_write ("s", "\t.byte ");
					TDump.f_write ("b", valueBlock [where_in_source]);
					valueOnLine ++;
				}
				else if (valueOnLine == 0x10) { valueOnLine = 0; where_in_source --; TDump.f_write ("s", "\r\n"); }
				else 
				{
					TDump.f_write ("sb", ",", valueBlock [where_in_source]);
					valueOnLine ++;
				}
			}
			break;
	}
}
