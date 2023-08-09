
// Nes_Snd_Emu 0.1.4. http://www.slack.net/~ant/nes-emu/

#include "Nes_Apu.h"

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

void Nes_Osc::reset() {
	delay = 0;
	enabled = FALSE;
	duration = 0;
	period = 1;
	last_amp = 0;
	duration_enabled = FALSE;
}

Nes_Osc::Nes_Osc() {
	output = NULL;
	reset();
}

void Nes_Osc::clock_duration() {
	if ( duration && duration_enabled )
		--duration;
}

static const unsigned char duration_table [0x20] = {
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 
	0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E
};
	
static inline int calc_duration( int v ) {
	return duration_table [(v >> 3) & 0x1F];
}

void Nes_Envelope::reset() {
	env_vol = 0;
	env_period = 1;
	env_count = 1;
	env_loop = 0;
	env_enabled = FALSE;
	env_reset = FALSE;
	volume = 0;
	Nes_Osc::reset();
}

Nes_Envelope::Nes_Envelope() {
	reset();
}

void Nes_Envelope::write_envelope( int reg, int value ) {
	switch ( reg & 3 ) {
		case 0:
			duration_enabled = !(value & 0x20);
			env_enabled = !(value & 0x10);
			
			env_loop = (value & 0x20) != 0;
			env_period = (value & 15) + 1;
			
			volume = env_enabled ? env_vol : value & 15;
			break;
		
		case 3:
			env_reset = TRUE;
			if ( enabled )
				duration = calc_duration( value );
			break;
	}
}

void Nes_Envelope::clock_envelope() {
	if ( env_reset ) {
		env_reset = FALSE;
		env_vol = 15;
		env_count = env_period;
	}
	else if ( !--env_count ) {
		if ( env_vol || env_loop )
			env_vol = (env_vol - 1) & 15;
		env_count = env_period;
	}
	
	if ( env_enabled )
		volume = env_vol;
}

// Nes_Square

const unsigned square_duty_range = 8;

void Nes_Square::reset() {
	sweep_count = 1;
	sweep_period = 0;
	sweep_shift = 0;
	sweep_negate = 0;
	phase_offset = 0;
	sweep_reload = FALSE;
	duty = square_duty_range / 2;
	phase = 0;
	
	Nes_Envelope::reset();
}

Nes_Square::Nes_Square() {
	synth = NULL;
	reset();
}

void Nes_Square::clock_sweep( int negative_adjust ) {
	if ( sweep_period && !--sweep_count ) {
		sweep_count = sweep_period;
		
		if ( period >= 8 ) {
			int offset = period >> sweep_shift;
			if ( sweep_negate )
				period += negative_adjust - offset;
			else if ( period + offset < 0x800 )
				period += offset;
		}
	}
	
	if ( sweep_reload ) {
		sweep_reload = FALSE;
		sweep_count = sweep_period;
	}
}

static const char duty_table [4] = { 1, 2, 4, 6 };

void Nes_Square::write_register( int addr, int value )
{
	switch ( addr & 3 ) {
		case 0x00:
			duty = duty_table [(value >> 6) & 3];
			phase_offset = (duty == 6 ? 6 : 0);
			break;
		
		case 0x01:
			sweep_negate = 0 - ((value >> 3) & 1);
			sweep_shift = value & 7;
			
			sweep_period = 0;
			if ( (value & 0x80) && sweep_shift ) {
				sweep_period = ((value >> 4) & 7) + 1;
				sweep_reload = TRUE;
			}
			break;
		
		case 0x02:
			period = (period & ~0xFF) | value;
			break;
		
		case 0x03:
			period = ((value & 7) << 8) | (period & 0xFF);
			phase = square_duty_range - 1;
			break;
	}
	
	write_envelope( addr, value );
}

void Nes_Square::run( nes_time_t time, nes_time_t end_time )
{
	const int sweep_overflows =
			period + (sweep_negate ^ (period >> sweep_shift)) >= 0x800;
	
	const int period_plus_one = (this->period + 1) * 2;
	
	if ( !duration || period < 8 || sweep_overflows || !volume )
	{
		if ( last_amp ) {
			synth->offset( time, -last_amp, output );
			last_amp = 0;
		}
		
		time += delay;
		if ( time < end_time ) {
			// keep calculating phase
			unsigned count = (end_time - time + period_plus_one - 1) / period_plus_one;
			phase = (phase + count) % square_duty_range;
			time += count * period_plus_one;
		}
	}
	else {
		unsigned phase = (this->phase + phase_offset) % square_duty_range;
		int amp = (((int) phase < duty) ? volume : 0);
		int diff = amp - last_amp;
		if ( diff ) {
			last_amp = amp;
			synth->offset( time, diff, output );
		}
		
		time += delay;
		if ( time < end_time )
		{
			Blip_Buffer* const output = this->output;
			const Synth* synth = this->synth;
			const int duty = this->duty;
			
			int delta = amp * 2 - volume;
			
			do {
				phase = (phase + 1) % square_duty_range;
				if ( !phase || phase == duty ) {
					delta = -delta;
					synth->offset_inline( time, delta, output );
				}
				time += period_plus_one;
			}
			while ( time < end_time );
			
			this->phase = (phase + square_duty_range - phase_offset) % square_duty_range;
			last_amp = (delta + volume) >> 1;
		}
	}
	
	delay = time - end_time;
}

// Nes_Triangle

void Nes_Triangle::reset() {
	lin_count = 0;
	lin_reg = 0;
	lin_load = FALSE;
	lin_control = FALSE;
	
	phase = phase_range / 2;
	Nes_Osc::reset();
}

Nes_Triangle::Nes_Triangle() {
	reset();
}

void Nes_Triangle::clock_lin_counter()
{
	if ( lin_load )
		lin_count = lin_reg;
	else if ( lin_count )
		lin_count--;
	
	if ( duration_enabled )
		lin_load = FALSE;
}

void Nes_Triangle::write_register( int addr, int value )
{
	switch ( addr ) {
		case 0x08:
			lin_reg = value & 0x7F;
			duration_enabled = !(value & 0x80);
			break;
		
		case 0x0A:
			period = (period & ~0xFF) | value;
			break;
		
		case 0x0B:
			period = ((value & 7) << 8) | (period & 0xFF);
			lin_load = TRUE;
			if ( enabled )
				duration = calc_duration( value );
			break;
	}
}

void Nes_Triangle::run( nes_time_t time, nes_time_t end_time )
{
	// to do: track phase when period < 2
	
	if ( duration && lin_count && period > 1 ) {
		time += delay;
		const int period_plus_one = this->period + 1;
		if ( time < end_time )
		{
			Blip_Buffer* const output = this->output;
			
			int phase = this->phase;
			int volume = 1;
			if ( phase > phase_range ) {
				phase -= phase_range;
				volume = -volume;
			}
			
			do {
				if ( !--phase ) {
					phase = phase_range;
					volume = -volume;
				}
				else {
					synth.offset_inline( time, volume, output );
				}
				
				time += period_plus_one;
			}
			while ( time < end_time );
			
			if ( volume < 0 )
				phase += phase_range;
			this->phase = phase;
		}
		
		delay = time - end_time;
	}
}

// Nes_Dmc

void Nes_Dmc::reset() {
	sample = 0x4000;
	length = 1;
	address = 0;
	dac = 0;
	buf = 0;
	bits_remain = 1;
	bits = 0;
	buf_empty = TRUE;
	silence = TRUE;
	loop = FALSE;
	next_irq = Nes_Apu::no_irq;
	irq_flag = FALSE;
	irq_enabled = FALSE;
	pal_mode = FALSE;
	
	Nes_Osc::reset();
	period = 0x036;
}

Nes_Dmc::Nes_Dmc() {
	rom_reader = NULL;
	apu = NULL;
	reset();
}

void Nes_Dmc::recalc_irq() {
	nes_time_t irq = Nes_Apu::no_irq;
	if ( irq_enabled && duration )
		irq = apu->last_time + delay +
				((duration - 1) * 8 + bits_remain - 1) * nes_time_t (period) + 1;
	if ( irq != next_irq ) {
		next_irq = irq;
		apu->irq_changed();
	}
}

static const short dmc_period_table [2] [16] = {
	0x1ac, 0x17c, 0x154, 0x140, 0x11e, 0x0fe, 0x0e2, 0x0d6, // NTSC
	0x0be, 0x0a0, 0x08e, 0x080, 0x06a, 0x054, 0x048, 0x036,
	
	0x18e, 0x161, 0x13c, 0x129, 0x10a, 0x0ec, 0x0d2, 0x0c7, // PAL
	0x0b1, 0x095, 0x084, 0x077, 0x062, 0x04e, 0x043, 0x032  // to do: verify PAL periods
};

void Nes_Dmc::write_register( int addr, int value )
{
	switch ( addr ) {
		case 0x10:
			loop = (value & 0x40) != 0;
			period = dmc_period_table [pal_mode] [value & 15];
			irq_enabled = !loop && (value & 0x80);
			irq_flag &= irq_enabled;
			recalc_irq();
			break;
		
		case 0x11:
			dac = value & 0x7F;
			break;
		
		case 0x12:
			sample = 0x4000 + value * 0x40;
			break;
		
		case 0x13:
			length = value * 0x10 + 1;
			break;
	}
}

void Nes_Dmc::start() {
	address = sample;
	duration = length;
	fill_buffer();
	recalc_irq();
}

void Nes_Dmc::fill_buffer() {
	if ( buf_empty && duration ) {
//		assert(( "Nes_Apu: Tried to use DMC without setting dmc_reader", rom_reader ));
		buf = rom_reader( rom_reader_data, 0x8000u + address );
		address = (address + 1) & 0x7FFF;
		buf_empty = FALSE;
		if ( !--duration )
		{
			if ( loop )
			{
				address = sample;
				duration = length;
			}
			else
			{
				enabled = FALSE;
				irq_flag = irq_enabled;
				next_irq = Nes_Apu::no_irq;
				apu->irq_changed();
			}
		}
	}
}

void Nes_Dmc::run( nes_time_t time, nes_time_t end_time )
{
	int dac = this->dac;
	
	int diff = dac - last_amp;
	if ( diff )
	{
		last_amp = dac;
		synth.offset( time, diff, output );
	}
	
	int bits = this->bits;
	int bits_remain = this->bits_remain;
	
	time += delay;
	
	if ( time < end_time )
	{
		if ( silence && buf_empty )
		{
			int count = (end_time - time + period - 1) / period;
			bits_remain = (bits_remain - 1 + 8 - (count % 8)) % 8 + 1;
			time += count * period;
		}
		else {
			Blip_Buffer* const output = this->output;
			const int period = this->period;
			
			do
			{
				if ( !silence )
				{
					const int step = (bits & 1) * 4 - 2;
					bits >>= 1;
					if ( unsigned (dac + step) <= 0x7F )
					{
						dac += step;
						synth.offset_inline( time, step, output );
					}
				}
				
				time += period;
				
				if ( !--bits_remain )
				{
					bits_remain = 8;
					if ( buf_empty )
					{
						silence = TRUE;
					}
					else
					{
						silence = FALSE;
						bits = buf;
						buf_empty = TRUE;
						fill_buffer();
					}
				}
			}
			while ( time < end_time );
			
			this->dac = dac;
			this->last_amp = dac;
			this->bits = bits;
		}
		this->bits_remain = bits_remain;
	}
	delay = time - end_time;
}

// Nes_Noise

void Nes_Noise::reset() {
	tap = 13;
	noise = 1 << 14;
	was_playing = FALSE;
	Nes_Envelope::reset();
}

Nes_Noise::Nes_Noise() {
	reset();
}

static const short noise_period_table [16] = {
	0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
	0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
};

void Nes_Noise::write_register( int addr, int value ) {
	if ( addr == 0x0E ) {
		period = noise_period_table [value & 15];
		tap = (value & 0x80 ? 8 : 13);
	}
	
	write_envelope( addr, value );
}

void Nes_Noise::run( nes_time_t time, nes_time_t end_time )
{
	int volume = this->volume;
	
	const int playing = duration && period > 3;
	
	// approximate noise cycling while muted
	// to do: precise muted noise cycling? 
	if ( (!playing || !volume) && tap == 13 ) {
		int feedback = (noise << 13) ^ (noise << 14);
		noise = (feedback & 0x4000) | (noise >> 1);
	}
	
	if ( !playing && !was_playing )
		return;
	
	was_playing = playing;
	
	if ( !playing )
		volume = 0;
	
	int amp = (noise & 1) ? volume : 0;
	int diff = amp - last_amp;
	if ( diff ) {
		last_amp = amp;
		synth.offset( time, diff, output );
	}
	
	time += delay;
	if ( time < end_time )
	{
		if ( !volume ) {
			// round to next multiple of period
			time += (end_time - time + period - 1) / period * period;
		}
		else {
			const int period = this->period;
			const int tap = this->tap;
			
			Blip_Buffer* const output = this->output;
			
			// keep parallel resampled time to eliminate multiplication in the loop
			const Blip_Buffer::resampled_time_t resampled_period =
					output->resampled_duration( period );
			Blip_Buffer::resampled_time_t resampled_time = output->resampled_time( time );
			
			int noise = this->noise;
			int delta = amp * 2 - volume;
			
			do {
				int changed = 1 & (noise ^ (noise >> 1));
				int feedback = (noise << tap) ^ (noise << 14);
				time += period;
				
				if ( changed ) {
					delta = -delta;
					synth.offset_resampled( resampled_time, delta, output );
				}
				
				resampled_time += resampled_period;
				noise = (feedback & 0x4000) | (noise >> 1);
			}
			while ( time < end_time );
			
			this->noise = noise;
			last_amp = (delta + volume) >> 1;
		}
	}
	
	delay = time - end_time;
}

