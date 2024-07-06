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

#ifndef _CMAPPER_021_H
#define _CMAPPER_021_H

#include "c_mapper.h"

class c_mapper_021 : public c_mapper {

	public:
		c_mapper_021 (void);
		~c_mapper_021 (void);

		void reset (void);
		void write_byte (__UINT_16, __UINT_8);
		void h_blank (void);
		void updatePPUPage (__UINT_8 bArea, __UINT_8 bHalf, __UINT_8 page);
        void swap_banks ();
        void swap_chr();

	private:

        __UINT_8 PRGBanks[2];
        __UINT_8 CHRBanks[8];
		__BOOL bIRQEnabled, bPRGBankSwitch;
		__UINT_8 bIRQCounter, bIRQReload;
		__UINT_8 bPortLow [8], bPortHigh [8], bPortData [8];

};

#endif