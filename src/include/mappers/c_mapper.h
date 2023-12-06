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
#include "../c_cpu.h"

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

		void create_label (s_label_node *o_label, __UINT_16 address, e_dattype type, e_dattype sub_type, int base, int offset, int ref_bank)
		{
            o_label->address = address;
			o_label->contents = address;
			o_label->bank = get_real_prg_bank_number (address);
			o_label->alias = nes->BankJMPList->get_bank_alias(o_label->bank, address);
			o_label->bank_lo = o_label->alias;
			o_label->bank_hi = o_label->alias;
			o_label->real_bank = o_label->bank;
			o_label->offset = offset;
			o_label->rom_offset = offset;
			o_label->type = type;
			o_label->sub_type = sub_type;
			o_label->jump_base_table = base;
			o_label->ref_bank = ref_bank;

		}

		virtual __UINT_8 get_bank_number (__UINT_16 address)
		{
			return 0;
		}

		__UINT_8 get_last_page_switched (void)
		{
			return last_page_switched;
		}

		virtual __UINT_8 get_prg_bank_number (__UINT_16 address)
		{
			return last_page_switched;
		}

		virtual __UINT_8 get_real_prg_bank_number (__UINT_16 address)
		{
			return last_page_switched;
		}

        virtual int get_max_pages(void)
        {
            return(max_alias);
        }

		virtual void save_state (c_tracer &o_writer);
		virtual void load_state (c_tracer &o_reader);

        virtual void set_vectors();
        int vectors_address;
        s_label_node *constants;
		__UINT_8 mapper_185;
		__UINT_8 last_page_switched;

	protected:
		__UINT_16 _1K_chr_mask, _2K_chr_mask, _4K_chr_mask, _8K_chr_mask;
		__UINT_16 _8K_prg_mask, _16K_prg_mask, _32K_prg_mask;
		int max_alias;
        int max_pages;
};

#endif