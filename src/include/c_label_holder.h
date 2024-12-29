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

#ifndef _CLABELHOLDER_H
#define _CLABELHOLDER_H

#include "datatypes.h"
#include <string.h>
#include <stdio.h>

enum e_dattype
{
	TYPE_DATA = 0,
	TYPE_CODE = 1,
	TYPE_BYTE = 2,
	TYPE_WORD = 3,
	TYPE_RELCODE = 4,
	TYPE_UNK = 5,
	TYPE_RAWWORD = 6,
	TYPE_DEAD = 7
};

struct s_label_node
{
    __UINT_8 bank;
    __UINT_16 address;
	e_dattype type;
	e_dattype sub_type;
    union
    {
	    __UINT_16 contents;
	    __UINT_16 size;
    };
    int offset;
    int bank_lo;
    int bank_hi;
    int alias;
	int real_bank;
	int rom_offset;
	int ref_bank;
	__UINT_16 jump_base_table;
	s_label_node *Next;

    void operator = (s_label_node *source)
    {
        this->address = source->address;
        this->contents = source->contents;
        this->Next = NULL;
    }

    s_label_node *create_page(s_label_node *holder,
                         int bank,
					     int address,
                         int size,
                         int offset,
                         int bank_lo,
                         int bank_hi,
                         int alias,
						 int rom_offset)
    {
		s_label_node *new_label;

		if(holder)
		{
		    __NEW(new_label, s_label_node);
            new_label->bank = bank;
            new_label->address = address;
            new_label->size = size;
            new_label->offset = offset;
            new_label->bank_lo = alias;
            new_label->bank_hi = alias;
            new_label->alias = alias;
            new_label->rom_offset = rom_offset;
			new_label->ref_bank = -1;
            new_label->Next = 0;
    		holder->Next = new_label;
            return(new_label);
        }
        else
        {
		    __NEW(holder, s_label_node);
            holder->bank = bank;
            holder->address = address;
            holder->size = size;
            holder->offset = offset;
            holder->bank_lo = alias;
            holder->bank_hi = alias;
            holder->alias = alias;
            holder->rom_offset = rom_offset;
			holder->ref_bank = -1;
            holder->Next = 0;
            return(holder);
        }
    }

};

class c_label_holder
{
	public:

		c_label_holder (void);
		~c_label_holder (void);

		void insert_label (__UINT_16 value, e_dattype type,
		                   e_dattype sub_type, int force, int base, int ref_bank = -1);
        int insert_label_bank (__UINT_8 bank, __UINT_16 value,
                               e_dattype type, e_dattype sub_type,
                               int force, int base, int ref_bank, int offset, int old_bank = -1, int jmp_pos = -1);
		s_label_node *search_page(int address);
		int is_current_page(int bank, int address);
		int fix_var_bank(int value, int ref_bank);
        int search_base(int alias, int address);
		int search_unknown_value(int address);
		int fix_rom_offset(int value, int ref_bank, int offset);
		s_label_node *get_page_from_bank(int bank);
		int get_bank_alias(int bank, int value);
		int get_real_bank(int alias);

		void dump_rom(void);
        s_label_node *search_label(int bank_lo,
								   int bank_hi,
								   int address,
								   int page_alias,
								   int real_ref = -1,
								   int all_refs = -1
								  );

	private:

		s_label_node *head;
		s_label_node unknown;

};

#endif