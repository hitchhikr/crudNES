
// Private oscillators used by Nes_Apu

// Nes_Snd_Emu 0.1.4. Copyright (C) 2003-2004 Shay Green. GNU GPL license.

#ifndef NES_OSCS_H
#define NES_OSCS_H

#include "Blip_Buffer.h"

class Nes_Apu_Reflector;
class Nes_Apu;

struct Nes_Osc
{
	int duration;   // duration counter (0 if unused by oscillator)
	int period;     // cycles between (potential) transition points
	int delay;      // delay until next (potential) transition
	int last_amp;   // last amplitude oscillator was outputting
	int enabled;   // master channel enable
	int duration_enabled;
	
	Blip_Buffer* output;
	
	Nes_Osc();
	
	void reset();
	
	virtual void run( nes_time_t begin, nes_time_t end ) = 0;
	virtual void write_register( int reg, int value ) = 0;
	
	void clock_duration();
};

// Nes_Dmc
struct Nes_Dmc : Nes_Osc
{
	int sample;     // sample's starting address
	int length;     // sample's length
	int address;    // address of next byte to read
	//int duration; // bytes remaining to play (already defined in Nes_Osc)
	int dac;
	int buf;
	int bits_remain;
	int bits;
	int buf_empty;
	int silence;
	int loop;
	
	nes_time_t next_irq;
	
	int irq_flag;
	int irq_enabled;
	int pal_mode;
	
	int (*rom_reader)( void*, nes_addr_t ); // needs to be initialized to rom read function
	void* rom_reader_data;
	
	Nes_Apu* apu;
	
	Blip_Synth<blip_low_quality,127> synth;
	
	Nes_Dmc();
	void reset();
	void start();
	void write_register( int, int );
	void run( nes_time_t, nes_time_t );
	
	void recalc_irq();
	void fill_buffer();
};

// Nes_Triangle
struct Nes_Triangle : Nes_Osc {
	int lin_count;
	int lin_reg;
	int lin_load;
	int lin_control;
	
	enum { phase_range = 16 };
	
	int phase;
	
	Blip_Synth<blip_good_quality,15> synth;
	
	Nes_Triangle();
	void reset();
	void write_register( int, int );
	void run( nes_time_t, nes_time_t );
	void clock_lin_counter();
};

class Nes_Envelope : public Nes_Osc {
	int env_vol;
	int env_period;
	int env_count;
	int env_loop;
	int env_enabled;
	int env_reset;
	
	friend class Nes_Apu_Reflector;
public:
	
	int volume;
	
	Nes_Envelope();
	void reset();
	void clock_envelope();
	void write_envelope( int reg, int value );
};

// Nes_Square
class Nes_Square : public Nes_Envelope {
	int sweep_count;
	int sweep_period; // zero if sweep is disabled
	int sweep_shift;
	int sweep_negate;
	int phase_offset;
	int sweep_reload;
	
	friend class Nes_Apu_Reflector;
public:
	
	int duty;
	unsigned phase;
	
	typedef Blip_Synth<blip_good_quality,15> Synth;
	const Synth* synth; // shared between squares
	
	Nes_Square();
	void reset();
	void clock_sweep( int adjust );
	void write_register( int, int );
	void run( nes_time_t, nes_time_t );
};

// Nes_Noise
struct Nes_Noise : Nes_Envelope {
	int tap;
	int noise;
	int was_playing;
	
	Blip_Synth<blip_med_quality,15> synth;
	
	Nes_Noise();
	void reset();
	void write_register( int, int );
	void run( nes_time_t, nes_time_t );
};

#endif

