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

#ifndef _CLABELHOLDER_H
#define _CLABELHOLDER_H

#include "datatypes.h"
#include <string.h>

#define TYPE_CODE 0
#define TYPE_DATA 1
#define TYPE_BYTE 2
#define TYPE_WORD 3
#define TYPE_RELCODE 4
#define TYPE_UNK 5
#define TYPE_RAWWORD 6

struct s_label_node
{
    union
    {
    	__UINT_8 bank;
    	__UINT_16 address;
    };
	__UINT_8 type;
	__UINT_8 sub_type;
    union
    {
	    __UINT_16 contents;
	    __UINT_16 size;
    };
    int offset;
    int bank_lo;
    int bank_hi;
    int alias;
    __UINT_16 jump_base_table;
	s_label_node *Next;

    operator = (s_label_node *source)
    {
        this->address = source->address;
        this->contents = source->contents;
        this->Next = NULL;
    }

    s_label_node *Create(s_label_node *holder,
                         int address,
                         int size,
                         int offset,
                         int bank_lo,
                         int bank_hi,
                         int alias)
    {
		s_label_node *new_label;

		if(holder)
		{
		    __NEW(new_label, s_label_node);
            new_label->address = address;
            new_label->size = size;
            new_label->offset = offset;
            new_label->bank_lo = bank_lo;
            new_label->bank_hi = bank_hi;
            new_label->alias = alias;
            new_label->Next = 0;
    		holder->Next = new_label;
            return(new_label);
        }
        else
        {
		    __NEW(holder, s_label_node);
            holder->address = address;
            holder->size = size;
            holder->offset = offset;
            holder->bank_lo = bank_lo;
            holder->bank_hi = bank_hi;
            holder->alias = alias;
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

		void insert_label (__UINT_16 value, __UINT_8 type,
		                   __UINT_8 sub_type, int force, int base);
        int insert_label_bank (__UINT_8 bank, __UINT_16 value,
                               __UINT_8 type, __UINT_8 sub_type,
                               int force, int base);
        void dump_rom(void);
        s_label_node *search_label(int bank_lo, int bank_hi, int address);

	private:

		s_label_node *head;
		s_label_node unknown;

};

#endif