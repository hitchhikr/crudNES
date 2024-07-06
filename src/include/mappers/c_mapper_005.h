/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes

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

#ifndef _CMAPPER_005_H
#define _CMAPPER_005_H

#include "c_mapper.h"
#include "../c_mem_block.h"

class c_mapper_005 : public c_mapper {

	public:
		c_mapper_005 (void);
		~c_mapper_005 (void);

		void reset (void);
		void write_byte (__UINT_16, __UINT_8);
		__UINT_8 read_byte (__UINT_16);
		void h_blank (void);
		void set_mirroring (void);
		__UINT_8 **get_extra_bg (void) { return ExtraBackground; }

		void save_state (c_tracer &o_writer);
		void load_state (c_tracer &o_reader);

	private:

		__BOOL bIsIrqEnabled, bIsIrqGenerated, bIsSRamEnabled;
		__UINT_8 bBankSwitch, bChrBankSwitch, bSRamProtected [2], bExRamUsage, * ExtraBackground [4];
		__UINT_8 bMirroring [4];
		__INT_32 iIrqCounter, iMultipliers [2], iProduct;

		c_mem_block ExRam, Fillnametable;
};

#endif