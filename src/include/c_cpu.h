/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    CPU Wrapper
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

#ifndef _CNESCPU_H
#define _CNESCPU_H

#include <math.h>

#include "c_mem_block.h"
#include "c_save_state.h"
#include "2A03.h"

#include "c_machine.h"
#include "c_nes.h"

#define CPU_INT_NONE 0
#define CPU_INT_IRQ  1
#define CPU_INT_NMI  2
#define CPU_INT_QUIT 3

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;
extern void irq_notifier (void *);

class c_nes_cpu
{
	public:

		c_nes_cpu (void);
		~c_nes_cpu (void);

		void reset (void);

		void run_accurate (void);

		void set_irq_line (__BOOL status) { _2A03_set_irq_line (status); }
		void request_nmi (void) { _2A03_request_nmi (); }
		void request_secondary_nmi (void) { _2A03_request_secondary_nmi (); }
		void request_irq (void) { _2A03_request_irq (); }
        
		void kill_cycles (__INT_32 cycles)
		{
			_2A03_kill_cycles (cycles*16);
		}

		__INT_32 current_time (void)
		{
			return _2A03_get_current_time () / (3*16);
		}

		__INT_32 earliest_event_before (__INT_32 iEndTime);

		void run_cycles (__INT_32 cycles);
		void apu_catch_up (void) { nes->o_apu.run_until (_2A03_get_current_time () / (3*16)); }

		void end_time_frame (__INT_32 end_time)
		{
			nes->o_apu.end_frame (_2A03_get_current_time () / (3*16));					
			nes->o_blip.end_frame (_2A03_get_current_time () / (3*16));
			_2A03_set_current_time (_2A03_get_current_time () - end_time);
			ideal_time -= end_time;
		}

		void toggle_logtracer (void) { _2A03_toggle_logtracer (); }
		void toggle_tracer (void) { _2A03_toggle_tracer (); }
		void toggle_label_holder (void) { _2A03_toggle_label_holder (); }
		void set_instruction_dumper (__BOOL status) { _2A03_set_tracer (status); }
		void set_label_holder (__BOOL status) { _2A03_set_label_holder (status); }
		void set_logtracer(__BOOL status) { _2A03_set_logtracer (status); }
		__BOOL is_tracer_on (void) { return _2A03_get_tracer (); }
		__BOOL is_logtracer_on (void) { return _2A03_get_logtracer (); }

		__UINT_8 read_byte (__UINT_16 address)
		{
			return PRGRAM [(address >> 12) & 7] [address & 0xfff];
		}
		__UINT_16 read_word (__UINT_16 address)
		{
			return *((__UINT_16 *)(PRGRAM [(address >> 12) & 7] + (address & 0xfff)));
		}
		int get_rom_offset (__UINT_16 address)
		{
			return (int) (((int) PRGRAM [(address >> 12) & 7] + (address & 0xfff)) - (int) &PRGROM[0]);
		}

		void swap_page (c_mem_block *uiDest, __UINT_16 dest_where, __UINT_8 page_number, e_page_sizes size)
		{
			uiDest->load_from (&PRGROM, page_number * size, dest_where, size);
		}

		void swap_page (__UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size);

		void output_video_sound (void);

		void save_state (c_tracer &o_writer, e_save_state type);
		void load_state (c_tracer &o_reader, e_save_state type);

		void SetEventTakingPlace (__BOOL status) { event_taking_place = status; }
		__BOOL is_pal (void) { return pal_console; }
		__UINT_32 get_cycle_multiplier (void) { return cycle_multiplier; }
		__UINT_32 get_last_line (void) { return last_line; }

		__BOOL event_taking_place;
		__INT_32 ideal_time;
		__UINT_16 Height;

		void request_config_load (void) { is_config_requested = TRUE; }
		void load_config (void);

		c_mem_block PRGROM;

	private:

		__UINT_8 is_frame_even;
		__UINT_8 *PRGRAM [8];
		__UINT_16 pages [8];
		__BOOL apu_irqs_enabled, is_config_requested, sound_enabled, is_sound_stereo, pal_console;
		__INT_32 sampling_rate, bits_per_sample, frame_rate;
		__UINT_32 last_line;
		__UINT_32 cycle_multiplier;

		AUDIOSTREAM *audio_stream;
};

#endif
