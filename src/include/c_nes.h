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

#ifndef _CNES_H
#define _CNES_H

#define nes ((c_nes *)(o_machine))

#include "../blargg/blargg_common.h"
#include "../blargg/Nes_Apu.h"
#include "c_tracer.h"
#include "c_machine.h"
#include "c_label_holder.h"

class c_save_state;
class c_label_holder;
class c_nes_rom;
class c_nes_control;
class c_nes_cpu;
class c_nes_ppu;
class c_graphics;
class c_input;
class c_mem_block;
class c_mem_block;
class c_mapper;
class c_save_state;
class c_tracer;

class c_nes : public c_machine
{
	public:

        c_nes();

		void Open (int PAL, const char *FilePath);
		void close (void);

		void reset (void);
		void pause (void);
		void stop (void);

		void save_state (void);
		void load_state (void);
		void set_slot (__INT_32 Slot);

		void set_instruction_dumper (__BOOL o_state);
		void set_label_holder (__BOOL o_state);

		void set_sram (const char *Path);
		void dump_header (const char *Path);

		void load_config (void);

	public:

		c_nes_rom *o_rom;
		c_nes_control *o_control;
		c_nes_cpu *o_cpu;
		c_nes_ppu *o_ppu;
		c_graphics *o_gfx;
		c_input *o_input;
		c_mem_block *o_ram;
		c_mem_block *o_sram;
		c_mapper *o_mapper;
		c_save_state *o_state;
		c_label_holder *BankJMPList;

		Nes_Apu o_apu;
		Blip_Buffer o_blip;

		c_tracer general_log;
		c_tracer TJumpTableLog;
		
		int is_mmc2_vrom;
        s_label_node *prg_pages;
        s_label_node *chr_pages;

		char Labels_Name[1024];
		char Game_Name[1024];
		char Game_FileName[1024];

	private:

};

#endif
