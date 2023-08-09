// Nes_Snd_Emu 0.1.4. http://www.slack.net/~ant/nes-emu/

#include "Nes_Apu.h"

#include <stdio.h>

/* Library Copyright (C) 2003-2004 Shay Green. Nes_Snd_Emu is free
software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.
Nes_Snd_Emu is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details. You should have received a copy of the GNU General Public
License along with Nes_Snd_Emu; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

Nes_Apu::Nes_Apu() {
	square1.synth = &square_synth;
	square2.synth = &square_synth;
	dmc.apu = this;
	oscs [0] = &square1;
	oscs [1] = &square2;
	oscs [2] = &triangle;
	oscs [3] = &noise;
	oscs [4] = &dmc;
	
	irq_notifier_ = NULL;
	
	volume( 1.0 );
	reset( FALSE );
}

Nes_Apu::~Nes_Apu() {
}

void Nes_Apu::treble_eq( const blip_eq_t& eq ) {
	square_synth.treble_eq( eq );
	triangle.synth.treble_eq( eq );
	noise.synth.treble_eq( eq );
	dmc.synth.treble_eq( eq );
}

void Nes_Apu::volume( double v ) {
	square_synth.volume( 0.1128 * v );
	triangle.synth.volume( 0.12765 * v );
	noise.synth.volume( 0.0741 * v );
	dmc.synth.volume( 0.42545 * 0.90 * v );
}

void Nes_Apu::output( Blip_Buffer* buffer ) {
	for ( int i = 0; i < osc_count; i++ )
		osc_output( i, buffer );
}

void Nes_Apu::reset( int pal_mode, int initial_dmc_dac )
{
	frame_period = pal_mode ? 8313 * 2 : (7457 * 2 + 1); // frame_period has 1 fraction bit
	
	next_irq = no_irq;
	earliest_irq_ = no_irq;
	irq_enabled = FALSE;
	irq_flag = FALSE;
	frame_max = 3;
	frame = 4;
	next_frame_time = 0;
	last_time = 0;
	
	square1.reset();
	square2.reset();
	triangle.reset();
	noise.reset();
	dmc.reset();
	dmc.pal_mode = pal_mode;
	
	dmc.dac = initial_dmc_dac;
	dmc.last_amp = initial_dmc_dac; // prevent any output transition
}

void Nes_Apu::irq_changed()
{
	nes_time_t new_irq = next_irq;
	if ( dmc.irq_flag | (irq_flag & irq_enabled) )
		new_irq = 0;
	else if ( dmc.next_irq < new_irq )
		new_irq = dmc.next_irq;
	
	if ( new_irq != earliest_irq_ ) {
		earliest_irq_ = new_irq;
		if ( irq_notifier_ )
			irq_notifier_( irq_data );
	}
}

void Nes_Apu::state_restored()
{
	noise.was_playing = TRUE;
	dmc.last_amp = dmc.dac; // avoid click
	dmc.recalc_irq();
	if ( irq_enabled )
		next_irq = ((next_frame_time + ((frame - 1) & 3) * frame_period) >> 1) + 1;
	irq_changed();
}

#ifndef GAME_SND_EMU_DEBUG
	#define GAME_SND_EMU_DEBUG 0
#endif

static FILE* log_file;

static int log_time( int t ) {
	if ( !GAME_SND_EMU_DEBUG )
		return 0;
	static nes_time_t last_time;
	if ( last_time > t ) {
		if ( log_file ) {
			static int counter;
//			fprintf( log_file, "\n%d -- ", ++counter );
		}
		last_time = 0;
	}
	int diff = t - last_time;
	last_time = t;
	return diff;
}

void Nes_Apu::begin_debug_log() {
	if ( GAME_SND_EMU_DEBUG && !log_file )
		log_file = fopen( "log", "w" );
}

// frames

void Nes_Apu::run_until( nes_time_t end_time )
{
	if ( end_time == last_time )
		return; // might be called very often in an emulator
	assert(( "Nes_Apu::run_until(): End time is before current time", last_time < end_time ));
	
	while ( TRUE ) {
		nes_time_t time = next_frame_time >> 1;
		if ( time > end_time )
			time = end_time;
		
		// run oscillators
		for ( int i = 0; i < osc_count; ++i ) {
			Nes_Osc& osc = *oscs [i];
			if ( osc.output )
				osc.run( last_time, time );
		}
		
		last_time = time;
		
		if ( time == end_time )
			break; // no more frames to run
		
/*		if ( GAME_SND_EMU_DEBUG && log_file )
			fprintf( log_file, "%d\n\t\t\t\tframe %d  ", log_time( time ), frame );
*/		
		// run frame
		next_frame_time += frame_period;
		
		if ( frame < 4 ) {
			if ( frame == 1 && frame_max == 3 ) {
	 			if ( irq_enabled )
				{
	 				next_irq = time + frame_period * (4 / 2) + 1;
					irq_flag = TRUE;
					irq_changed ();
				}
			}
			
			if ( frame == 1 || frame == 3 ) {
				square1.clock_sweep( -1 );
				square2.clock_sweep( 0 );
				
				square1.clock_duration();
				square2.clock_duration();
				noise.clock_duration();
				triangle.clock_duration();
			}
			
			square1.clock_envelope();
			square2.clock_envelope();
			noise.clock_envelope();
			triangle.clock_lin_counter();
		}
		
		if ( --frame < 0 )
			frame = frame_max;
	}
}

void Nes_Apu::end_frame( nes_time_t offset )
{
	run_until( offset );
	
	// make times relative to new frame
	last_time = 0;
	next_frame_time -= offset * 2;
	assert( next_frame_time >= 0 );
	if ( next_irq != no_irq ) {
		next_irq -= offset;
		assert( next_irq >= 0 );
	}
	if ( dmc.next_irq != no_irq ) {
		dmc.next_irq -= offset;
		assert( dmc.next_irq >= 0 );
	}
	if ( earliest_irq_ != no_irq ) {
		earliest_irq_ -= offset;
		if ( earliest_irq_ < 0 )
			earliest_irq_ = 0;
	}
}

// registers

#pragma optimize ("a", off)

void Nes_Apu::write_register( nes_time_t time, nes_addr_t addr, int data )
{
	assert(( "Nes_Apu::write_register() now takes actual address (i.e. 0x40xx)", addr > 0x20 ));
	assert(( "Nes_Apu::write_register(): Data out of range",  0 <= data && data <= 0xFF ));
	
	int reg = addr - start_addr;
	if ( unsigned (reg) > end_addr - start_addr )
		return;
	
	run_until( time );
	
	int osc_index = reg >> 2;
	
/*	if ( GAME_SND_EMU_DEBUG && log_file &&
			(osc_index >= osc_count || oscs [osc_index]->output) )
		fprintf( log_file, "%d\n%.2X %.2X  ", log_time( time ), reg, data );
*/	
	if ( osc_index < osc_count )
	{
		// osc-specific
		oscs [osc_index]->write_register( reg, data );
	}
	else if ( addr == 0x4015 )
	{
		// enable/disable channels
		
		// volatile works around MSVC++ optimizer bug
		volatile int dmc_was_enabled = dmc.enabled;

		for ( int i = 0; i < osc_count; ++i )
		{
			Nes_Osc& osc = *oscs [i];
			osc.enabled = (data >> i) & 1;
			if ( !osc.enabled )
				osc.duration = 0;
		}

		int recalc_irq = dmc.irq_flag;
		dmc.irq_flag = FALSE;
		
		if ( !dmc.enabled ) {
			dmc.next_irq = no_irq;
			recalc_irq = TRUE;
		}
		else if ( !dmc_was_enabled ) {
			dmc.start(); // dmc just enabled
		}
		
		//if ( recalc_irq ) //Modified by Hyde
			irq_changed();
	}
	else if ( addr == 0x4017 )
	{
		// configure number of frames in cycle; reset frame and phase counters
		next_frame_time = time * 2;
		next_irq = no_irq;
		irq_enabled = FALSE;
		frame_max = 4;
		frame = 3;
		
		if ( !(data & 0x80) )
		{
			frame = 0;
			frame_max = 3;
			next_frame_time += frame_period;
			if ( !(data & 0x40) )
			{
				irq_enabled = TRUE;
				next_irq = time + frame_period * (4 / 2) + 1;
			}
		}
		
		irq_changed();
	}
}

int Nes_Apu::read_status( nes_time_t time )
{
	run_until( time );
	
/*	if ( GAME_SND_EMU_DEBUG && log_file )
		fprintf( log_file, "%d\nread APU ", log_time( time ), frame );
*/	
	int result = 0;
	if ( irq_flag )
	{
		irq_flag = FALSE;
		result = 0x40;
		irq_changed();
	}
	
	result |= dmc.irq_flag << 7;
	
	for ( int i = 0; i < osc_count; i++ )
		if ( oscs [i]->duration )
			result |= 1 << i;
	
	return result;
}

#pragma optimize ("a", on)
