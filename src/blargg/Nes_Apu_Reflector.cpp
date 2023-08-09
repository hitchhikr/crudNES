
// Nes_Snd_Emu 0.1.4. http://www.slack.net/~ant/nes-emu/

// This is meant as a starting point for a state saving scheme in an emulator.
// Nes_Apu_Reflector is a friend of the various APU-related classes to allow
// access to private members.

#include "Nes_Apu_Reflector.h"

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

static int reflect_int( int i, FILE* file, int load ) {
	if ( load )
		fscanf( file, "%d ", &i );
	else
		fprintf( file, "%d ", i );
	return i;
}

// Cut down on template bloat by forwarding to helper function
template<class T>
void nes_apu_reflect( T& t, FILE* file, int load ) {
	t = reflect_int( t, file, load );
}

// Allow single function to do both directions of reflection.
// Much simpler and eliminates subtle errors due to different save/load functions.

#define REFLECT( var )  nes_apu_reflect( var, file, load )

void Nes_Apu_Reflector::reflect_apu( Nes_Apu& apu, FILE* file, int load )
{
	// APU
	// don't save/restore apu.last_time
	REFLECT( apu.next_frame_time );
	REFLECT( apu.frame );
	REFLECT( apu.frame_max );
	REFLECT( apu.irq_flag );
	REFLECT( apu.irq_enabled );
	if ( !load ) fputc( '\n', file ); // make file easier to debug
	
	// Squares
	reflect_sq( apu.square1, file, load );
	reflect_sq( apu.square2, file, load );
	if ( !load ) fputc( '\n', file );
	
	// Triangle
	reflect_osc( apu.triangle, file, load );
	REFLECT( apu.triangle.phase );
	REFLECT( apu.triangle.lin_count );
	REFLECT( apu.triangle.lin_reg );
	REFLECT( apu.triangle.lin_load );
	REFLECT( apu.triangle.lin_control );
	if ( !load ) fputc( '\n', file );
	
	// Noise
	reflect_env( apu.noise, file, load );
	REFLECT( apu.noise.noise);
	REFLECT( apu.noise.tap );
	if ( !load ) fputc( '\n', file );
	
	// DMC
	reflect_osc( apu.dmc, file, load );
	REFLECT( apu.dmc.dac );
	REFLECT( apu.dmc.silence );
	REFLECT( apu.dmc.bits );
	REFLECT( apu.dmc.bits_remain );
	REFLECT( apu.dmc.buf );
	REFLECT( apu.dmc.buf_empty );
	REFLECT( apu.dmc.address );
	REFLECT( apu.dmc.loop );
	REFLECT( apu.dmc.sample );
	REFLECT( apu.dmc.length );
	REFLECT( apu.dmc.irq_flag );
	REFLECT( apu.dmc.irq_enabled );
	if ( !load ) fputc( '\n', file );
	
	// After loading, allow APU to make any necessary adjustments to the loaded state
	if ( load )
		apu.state_restored();
}

void Nes_Apu_Reflector::reflect_osc( Nes_Osc& osc, FILE* file, int load ) {
	// don't save/restore osc.last_amp
	REFLECT( osc.enabled );
	REFLECT( osc.delay );
	REFLECT( osc.period );
	REFLECT( osc.duration );
	REFLECT( osc.duration_enabled );
	if ( !load ) fputc( '\n', file );
}

void Nes_Apu_Reflector::reflect_env( Nes_Envelope& env, FILE* file, int load ) {
	reflect_osc( env, file, load );
	REFLECT( env.volume );
	REFLECT( env.env_enabled );
	REFLECT( env.env_reset );
	REFLECT( env.env_vol );
	REFLECT( env.env_count );
	REFLECT( env.env_period );
	REFLECT( env.env_loop );
	if ( !load ) fputc( '\n', file );
}

void Nes_Apu_Reflector::reflect_sq( Nes_Square& sq, FILE* file, int load ) {
	reflect_env( sq, file, load );
	REFLECT( sq.phase );
	REFLECT( sq.phase_offset ); // format of this might change
	REFLECT( sq.duty );
	REFLECT( sq.sweep_count );
	REFLECT( sq.sweep_period );
	REFLECT( sq.sweep_shift );
	REFLECT( sq.sweep_negate );
	REFLECT( sq.sweep_reload );
	if ( !load ) fputc( '\n', file );
}

void Nes_Apu_Reflector::save( Nes_Apu const& apu, FILE* file ) {
	reflect_apu( (Nes_Apu&) apu, file, FALSE );
}

void Nes_Apu_Reflector::load( FILE* file, Nes_Apu& apu ) {
	reflect_apu( (Nes_Apu&) apu, file, TRUE );
}

