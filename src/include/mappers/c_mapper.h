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

#ifndef _CMAPPER_H
#define _CMAPPER_H

#include "../c_nes.h"
#include "../c_label_holder.h"
#include "../c_tracer.h"
#include "../c_rom.h"

#include "../datatypes.h"

extern c_machine *o_machine;

class c_mapper {

	public:
		c_mapper (void);
		virtual ~c_mapper (void);

		virtual void reset (void);
		virtual void write_byte (__UINT_16, __UINT_8);
		virtual __UINT_8 read_byte (__UINT_16);
		virtual void update (void *vData);
		virtual void h_blank (void);

		virtual void create_label (s_label_node *o_label, __UINT_16 address, __UINT_8 type, __UINT_8 sub_type, int base)
		{
			o_label->contents = address;
			o_label->bank = get_bank_number(address);
			o_label->type = type;
			o_label->sub_type = sub_type;
			o_label->jump_base_table = base;
		}

		virtual __UINT_8 get_bank_number (__UINT_16 address) {	return 0; }
		__UINT_8 get_last_page_switched (void) { return last_page_switched; }
		virtual int get_bank_alias(int bank, int address)
		{
		    return last_page_switched;
        }

        virtual int get_max_pages(void)
        {
            return(max_pages - 1);
        }

		virtual void save_state (c_tracer &o_writer);
		virtual void load_state (c_tracer &o_reader);

        virtual void set_vectors();
        int vectors_address;
        s_label_node *constants;

	protected:
		__UINT_16 _1K_chr_mask, _2K_chr_mask, _4K_chr_mask, _8K_chr_mask;
		__UINT_16 _8K_prg_mask, _16K_prg_mask, _32K_prg_mask;

		__UINT_8 last_page_switched;
        int max_pages;
};

#endif