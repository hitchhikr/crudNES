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

#ifndef _CMAPPER_007_H
#define _CMAPPER_007_H

#include "c_mapper.h"

class c_mapper_007 : public c_mapper
{
	public:

		c_mapper_007 (void);
		~c_mapper_007 (void);

		void reset (void);
		void write_byte (__UINT_16, __UINT_8);

private:
		int last_prg_page;
};

#endif