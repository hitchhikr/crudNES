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

#ifndef _CMEMORYBLOCK_H
#define _CMEMORYBLOCK_H

#include <string.h>

#include "datatypes.h"

enum EDumpType { BINARY = 0, BYTE_HEX };

class c_tracer;

class c_mem_block {

	public:

        c_mem_block (void);
		c_mem_block (const __UINT_8 *bSource, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size);
		c_mem_block (__UINT_32 size);
		c_mem_block (c_mem_block *source, __UINT_32 where_in_source, __UINT_32 dest_where);

		~c_mem_block (void);

		void load_from (c_mem_block *source, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size);
		void load_from (const __UINT_8 *bSource, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size);
		void load_from (const char *filename, __UINT_32 where_in_source, __UINT_32 dest_where, __UINT_32 size);

		__UINT_8 read_byte (__UINT_32 address) { return valueBlock [address & uiAccessMask]; }
		__UINT_16 read_word (__UINT_32 address) { return *((__UINT_16 *)(valueBlock + (address & uiAccessMask))); }
		__UINT_32 read_dword (__UINT_32 address) { return *((__UINT_32 *)(valueBlock + (address & uiAccessMask))); }

		void write_byte (__UINT_32 address, __UINT_8 value) {	valueBlock [address & uiAccessMask] = value; }
		void write_word (__UINT_32 address, __UINT_16 uiData) { *((__UINT_16 *)(valueBlock + (address & uiAccessMask))) = uiData; }
		void write_dword (__UINT_32 address, __UINT_32 uiData) { *((__UINT_32 *)(valueBlock + (address & uiAccessMask))) = uiData; }
	
		__UINT_8 & operator [] (__UINT_32 address) {	return valueBlock [address & uiAccessMask]; }

		__UINT_32 get_size (void) { return size; }
		void resize (__UINT_32 size) { default_construction (size); }

		void reset_access_mask (__UINT_32 uiMask) { uiAccessMask = uiMask; }

		void clear_to (__UINT_32 dest_where, __UINT_32 uiPattern)
		{
			memset (valueBlock + dest_where, uiPattern, size);
		}

		void clear_to (__UINT_32 dest_where, __UINT_32 uiPattern, __UINT_32 size)
		{
			memset (valueBlock + dest_where, uiPattern, size);
		}

		void dump_to (char *filename, char *dump_title, __UINT_32 where_in_source, __UINT_32 size, EDumpType type = BYTE_HEX);
		void dump_to (c_tracer &TDump, char *dump_title, __UINT_32 where_in_source, __UINT_32 size, EDumpType type = BYTE_HEX);
		void dump_to (__UINT_16 address, __UINT_8 value);

	private:

		void default_construction (__UINT_32 size);

		__UINT_8 * valueBlock;
        __UINT_32 uiAccessMask;
		__UINT_32 size;
};

#endif