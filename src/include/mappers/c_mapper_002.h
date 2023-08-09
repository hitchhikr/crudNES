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

#ifndef _CMAPPER_002_H
#define _CMAPPER_002_H

#include "c_mapper.h"

class c_mapper_002: public c_mapper {

	public:
		c_mapper_002(void);
		~c_mapper_002(void);

		void reset (void);
		void write_byte (__UINT_16, __UINT_8);

		void create_label (s_label_node *o_label, __UINT_16 address, __UINT_8 type, __UINT_8 sub_type, int base)
		{
			o_label->contents = address;
            o_label->bank = get_bank_number (address);
			o_label->type = type;
			o_label->sub_type = sub_type;
			o_label->jump_base_table = base;
		}

		__UINT_8 get_bank_number (__UINT_16 address)
		{
			if (address < 0xc000) return get_last_page_switched ();
			else return nes->o_rom->information ().prg_pages - 1;
		}

		int get_bank_alias(int bank, int address)
		{
            if(address < 0xc000)
            {
                // Case where there's jumps lower bank from the last one.
                if((nes->o_rom->information ().prg_pages - 1) == bank) return -1;
                return bank;
		    }
		    return nes->o_rom->information ().prg_pages - 1;
		}

};

#endif