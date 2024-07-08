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

#pragma warning (disable : 4267 4311)

#ifdef __CRUDNES_ALLEGRO
	#pragma warning (disable : 4200)
	#include "include/allegro.h"
#elif defined CRUDNES_SDL
	#include <sdl.h>
#endif

#include <stdio.h>
#include <string.h>

#include "include/2a03.h"
#include "include/c_cpu.h"
#include "include/c_ppu.h"
#include "include/c_rom.h"
#include "include/mappers/c_mapper.h"
#include "include/c_input.h"
#include "include/c_graphics.h"
#include "include/c_machine.h"
#include "include/c_nes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** irq_notifier ()                                                            **/
/**                                                                          **/
/** Sound IRQ call-back procedure. Needed by Shay's library.                 **/
/******************************************************************************/

void irq_notifier (void *)
{
	_2A03_set_end_time (nes->o_cpu->earliest_event_before (_2A03_get_end_time ()));
}

/******************************************************************************/
/** dmc_reader ()                                                             **/
/**                                                                          **/
/** Reads DMC sound samples from the main memory. Needed by Shay's library.  **/
/******************************************************************************/

int dmc_reader (void *, unsigned address)
{
	_2A03_kill_cycles (4*3*(!nes->o_cpu->is_pal ()?16:15));
	return nes->o_cpu->read_byte (address);
}

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_nes_cpu :: c_nes_cpu (void)
{
	__DBG_INSTALLING ("CPU");

	PRGROM.resize (nes->o_rom->information ().prg_pages * _16K_);
	nes->o_rom->transfer_block (&PRGROM [0], 0x10, nes->o_rom->information ().prg_pages * _16K_);

	memset (pages, 0x00, sizeof(pages));

	nes->o_rom->HEADER.resize (0x10);
	nes->o_rom->ROM.resize ((nes->o_rom->information ().prg_pages * _16K_) +
							(nes->o_rom->information ().chr_pages * _8K_));
	nes->o_rom->load_ROM(nes->o_rom->info.filename, 0,
						(__UINT_32) &nes->o_rom->HEADER[0],
						(__UINT_32) &nes->o_rom->ROM[0],
						(nes->o_rom->information ().prg_pages * _16K_) +
						(nes->o_rom->information ().chr_pages * _8K_));

	audio_stream = NULL;
	load_config ();

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_nes_cpu :: ~c_nes_cpu (void)
{
	__DBG_UNINSTALLING ("CPU");

	if (audio_stream)
		stop_audio_stream (audio_stream);

	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset ()                                                                 **/
/******************************************************************************/

void c_nes_cpu :: reset (void)
{
	ideal_time = 0;
	event_taking_place = FALSE;
	is_frame_even = FALSE;

	nes->o_mapper->reset ();
	_2A03_reset ();

	nes->o_apu.output (&nes->o_blip);
	nes->o_apu.irq_notifier (irq_notifier, NULL);
	nes->o_apu.dmc_reader (dmc_reader, NULL);

	nes->o_apu.reset (pal_console?TRUE:FALSE);
	nes->o_blip.clear ();
}

/******************************************************************************/
/** Frame limiter stuff.                                                    **/
/******************************************************************************/

__INT_32 c_nes_cpu :: earliest_event_before (__INT_32 iEndTime)
{
	if (!_2A03_get_interrupt_flag ())
	{
		__INT_32 irq_time = nes->o_apu.earliest_irq ();
		if (irq_time < 0x2aaaaaaa && ((irq_time * 3 * ((!pal_console)?16:15)) <= iEndTime)) {iEndTime = irq_time * 3 * ((!pal_console)?16:15); }
	}

	return iEndTime;
}

void c_nes_cpu :: run_cycles (__INT_32 cycles)
{
	if (!pal_console) ideal_time += cycles * 16;
	else ideal_time += cycles * 15;

	while (_2A03_get_current_time () < ideal_time)
	{
		_2A03_set_end_time (earliest_event_before (ideal_time));

		if (_2A03_get_end_time () <= _2A03_get_current_time ())
		{
			if (apu_irqs_enabled) request_irq ();
			_2A03_set_end_time (ideal_time);
		}

		if (_2A03_has_enough_cycles ()) _2A03_run ();
	}
}

/******************************************************************************/
/** run_accurate ()                                                           **/
/******************************************************************************/

void c_nes_cpu :: run_accurate (void)
{
	if(audio_stream) voice_start (audio_stream->voice);

    nes->o_mapper->set_vectors();

	while (nes->is_running ())
	{
		if (is_config_requested) { load_config (); }
		if (nes->o_gfx->is_config_requested ()) { nes->o_gfx->load_config (); }

		if (nes->is_paused ())
		{
			voice_stop (audio_stream->voice);
			while (nes->is_paused ()) 
            {
                rest (10);
                nes->o_input->handle_input (0);
                nes->o_input->handle_input (1);
            }
			voice_start (audio_stream->voice);
		}

		//Scanline #0
		if (is_logtracer_on ()) nes->general_log.f_write ("s", "--------------------------------- PROCESSING *__NEW FRAME* -----------------------------------\r\n");

		//New frame. Adjust a few flags.
		nes->o_gfx->lock_buffer ();
		nes->o_ppu->begin_frame ();

		//Scanlines #1-239
		for (nes->o_ppu->information ().scanline = 0;
				(int) nes->o_ppu->information ().scanline <(nes->o_cpu->Height + 16);
				nes->o_ppu->information ().scanline ++)
		{
			//Clear the current Scanline if background rendering is disabled.
			if (is_logtracer_on ()) nes->general_log.f_write ("sds", "---------------------- PROCESSING *SCANLINE* #", nes->o_ppu->information ().scanline, " ---------------------\r\n");				

			nes->o_ppu->run_accurate ();

			apu_catch_up ();
		}

		//Scanline #240
		if (is_logtracer_on ()) nes->general_log.f_write ("sds", "--------------------------------- PROCESSING *HBLANK* #", nes->o_ppu->information ().scanline, " ---------------------------------\r\n");
		run_cycles (260);
		nes->o_mapper->h_blank ();
		run_cycles (81);

		apu_catch_up ();

		nes->o_ppu->information ().scanline++;

		//Scanline #241 - VBlank
		if (is_logtracer_on ()) nes->general_log.f_write ("s", "----------------------------------- PROCESSING *VBLANK* ------------------------------------\r\n");
		nes->o_ppu->information ().is_v_blank = TRUE;

		//TODO: Should there be a delay here? In theory, no.
		nes->o_ppu->set_flag (STAT, BIT_7);
//		nes->o_ppu->set_flag (CTL_1, BIT_7);
		run_cycles (6);
		if (nes->o_ppu->get_flag (CTL_1, BIT_7))
		{
		    request_nmi ();
		}
		run_cycles (341 - 6);

		apu_catch_up ();

		//Scanlines #242-262
		for (nes->o_ppu->information ().scanline = 242;
			 nes->o_ppu->information ().scanline < last_line;
			 nes->o_ppu->information ().scanline ++)
		{
			if (is_logtracer_on ()) nes->general_log.f_write ("sds", "------------------------------- PROCESSING *SCANLINE* #", nes->o_ppu->information ().scanline, " ---------------------------------\r\n");
			run_cycles (341);
			nes->o_mapper->h_blank ();

			apu_catch_up ();
		}

		if (is_logtracer_on ()) nes->general_log.f_write ("sds", "------------------------------- PROCESSING *SCANLINE* #", nes->o_ppu->information ().scanline, " ---------------------------------\r\n");
		nes->o_ppu->end_frame ();
		run_cycles (260);
		nes->o_mapper->h_blank ();
		run_cycles (81);

		apu_catch_up ();

		//Adjust timers
		end_time_frame (ideal_time);

		__BOOL skip_sync = FALSE;

		nes->o_gfx->unlock_buffer ();
		output_video_sound ();

		if (nes->o_input->handle_input (0) == CPU_INT_QUIT ||
            nes->o_input->handle_input (1) == CPU_INT_QUIT)
        {
            break;	
        }
		//handle input
	}	

	if (audio_stream) voice_stop (audio_stream->voice);
}

void c_nes_cpu :: output_video_sound (void)
{
	__BOOL skip_sync = FALSE;

	//handle input
	if (nes->o_blip.samples_avail () >= (((__UINT_32)(sampling_rate) / frame_rate) * 5))
	{
		blip_sample_t * sound_buffer = NULL;

		if (sound_enabled)
		{
			do
			{
				sound_buffer = (blip_sample_t *)(get_audio_stream_buffer (audio_stream));
				//if (!sound_buffer) rest (10);			
			} while (!sound_buffer);

			nes->o_blip.read_samples (sound_buffer, (sampling_rate / frame_rate) * 5, FALSE);
			free_audio_stream_buffer (audio_stream);
		} else
			nes->o_blip.remove_samples ((sampling_rate / frame_rate) * 5);
//		skip_sync = TRUE;
	}
//	if (sound_enabled)
  //  {
    //    get_audio_stream_buffer (audio_stream);
//        skip_sync = TRUE;
	//}

	//Graphics
	nes->o_gfx->draw_frame ();
	vsync ();
}

void c_nes_cpu :: swap_page (__UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size)
{
	__UINT_16 bank = (dest_where >> 12) & 7;

	PRGRAM [bank] = &PRGROM [page_number * size];
	
	if (_16K_ == size)
	{
		pages [bank] = page_number << 2;
		
		PRGRAM [bank + 1] = PRGRAM [bank] + _4K_;
		PRGRAM [bank + 2] = PRGRAM [bank + 1] + _4K_;
		PRGRAM [bank + 3] = PRGRAM [bank + 2] + _4K_;

		pages [bank + 1] = pages [bank] + 1;
		pages [bank + 2] = pages [bank] + 2;
		pages [bank + 3] = pages [bank] + 3;
	}

	else if (_8K_ == size)
	{
		pages [bank] = page_number << 1;
		pages [bank + 1] = pages [bank] + 1;
		PRGRAM [bank + 1] = PRGRAM [bank] + _4K_;
	}

	else if (_32K_ == size)
	{
		pages [bank] = page_number << 3;

		PRGRAM [1] = PRGRAM [0] + _4K_;
		PRGRAM [2] = PRGRAM [1] + _4K_;
		PRGRAM [3] = PRGRAM [2] + _4K_;
		PRGRAM [4] = PRGRAM [3] + _4K_;
		PRGRAM [5] = PRGRAM [4] + _4K_;
		PRGRAM [6] = PRGRAM [5] + _4K_;
		PRGRAM [7] = PRGRAM [6] + _4K_;
		pages [1] = pages [bank] + 1;
		pages [2] = pages [bank] + 2;
		pages [3] = pages [bank] + 3;
		pages [4] = pages [bank] + 4;
		pages [5] = pages [bank] + 5;
		pages [6] = pages [bank] + 6;
		pages [7] = pages [bank] + 7;
	}
}

/******************************************************************************/
/** save_state ()                                                             **/
/******************************************************************************/

void c_nes_cpu :: save_state (c_tracer &o_writer, e_save_state Type)
{
	switch (Type)
	{
		case MAIN:
			o_writer.write (pages, 16);
			o_writer.write (&is_frame_even, 1);
			_2A03_save_state (o_writer);
			break;
		case OTHER:
			nes->o_mapper->save_state (o_writer);
			break;
	}
}

/******************************************************************************/
/** load_state ()                                                             **/
/******************************************************************************/

void c_nes_cpu :: load_state (c_tracer &o_reader, e_save_state Type)
{
	switch (Type)
	{
		case MAIN:
			o_reader.read (pages, 16);
			o_reader.read (&is_frame_even, 1);

			for (__UINT_32 uiPage = 0; uiPage < 8; uiPage ++)
			{
				PRGRAM [uiPage] = &PRGROM [pages [uiPage] * _4K_];
            }

			_2A03_load_state (o_reader);
			break;
		case OTHER:
			nes->o_mapper->load_state (o_reader);
			break;
	}
}

void c_nes_cpu :: load_config (void)
{
	is_config_requested = FALSE; 

	apu_irqs_enabled = 0;   //!bValue;
	pal_console = nes->o_rom->information().pal;        //bValue;
    sampling_rate = 1;
    bits_per_sample = 0;
	set_label_holder(1);
	set_instruction_dumper(1);

	switch (sampling_rate)
	{
		case 0: sampling_rate = 22050; break;
		case 1: sampling_rate = 44100; break;
	}

	switch (bits_per_sample)
	{
		case 0: bits_per_sample = 16; break;
	}

	nes->o_blip.clock_rate (((pal_console) ? 1662607 : 1789773));
	nes->o_blip.sample_rate (sampling_rate);	

    Height = pal_console ? 232: 224;

	if (!pal_console)
	{
	    frame_rate = 60; last_line = 261;
	}
	else
	{
	    frame_rate = 50; last_line = 261 + 50;
	}

	if (sound_enabled && audio_stream)
	{
	    voice_stop (audio_stream->voice); stop_audio_stream (audio_stream);
	}

	sound_enabled = TRUE;

	if (sound_enabled)
	{
		audio_stream = play_audio_stream ((sampling_rate / frame_rate) * 5, bits_per_sample, 0, sampling_rate, 255, 127);
		voice_start (audio_stream->voice);
	}
}
