
// NES 2A03 APU sound chip emulator

// Nes_Snd_Emu 0.1.4. Copyright (C) 2003-2004 Shay Green. GNU GPL license.

#ifndef NES_APU_H
#define NES_APU_H

typedef long nes_time_t;     // clock cycle count
typedef unsigned nes_addr_t; // 16-bit address

#include "Nes_Oscs.h"

class Nes_Apu {
public:
	Nes_Apu();
	~Nes_Apu();
	
	// Overall volume of all oscillators, where 1.0 is full volume.
	void volume( double );
	
	// Treble equalization (see notes.txt).
	void treble_eq( const blip_eq_t& );
	
	// Assign all oscillator outputs to specified buffer. If buffer
	// is NULL, silence all oscillators.
	void output( Blip_Buffer* mono );
	
	// Assign oscillator output to buffer. Valid indicies are 0 to
	// osc_count - 1, which refer to Square 1, Square 2, Triangle, Noise,
	// and DMC, respectively. If buffer is NULL, silence oscillator.
	enum { osc_count = 5 };
	void osc_output( int index, Blip_Buffer* buffer );
	
	// reset internal frame counter and registers, and silence all
	// oscillators. Use PAL timing if pal_timing is TRUE, otherwise
	// use NTSC timing. Set the DMC oscillator's initial DAC value
	// to initial_dmc_dac without any audible click.
	void reset( int pal_timing = FALSE, int initial_dmc_dac = 0 );
	
	// Memory reader callback used by DMC oscillator to fetch samples.
	// When invoked, 'user_data' is passed unchanged as the first
	// parameter.
	void dmc_reader( int (*callback)( void* user_data, nes_addr_t ), void* user_data = NULL );
	
	// IRQ time callback that is invoked when the time of earliest IRQ
	// may have changed, or NULL to disable. When invoked, 'user_data'
	// is passed unchanged as the first parameter.
	void irq_notifier( void (*callback)( void* user_data ), void* user_data = NULL );
	
	// Time of the next APU-generated IRQ, assuming no further register reads
	// or writes occur. If time isn't later than the present, the IRQ has
	// occurred and is still pending.
	nes_time_t earliest_irq() const;
	
	// write 'data' to 'addr' at specified time. Previous writes and reads
	// within the current frame must not have specified a time later
	// than 't'. Address must satisfy start_addr <= addr <= end_addr.
	enum { start_addr = 0x4000 };
	enum { end_addr   = 0x4017 };
	void write_register( nes_time_t t, nes_addr_t addr, int value );
	
	// Read from the status register at specified time. Previous register
	// writes and status reads within the current frame must not have
	// specified a time later than the time specified.
	enum { status_addr = 0x4015 };
	int read_status( nes_time_t );
	
	// run APU until specified time, so that any DMC memory reads can be
	// accounted for (i.e. inserting CPU wait states).
	void run_until( nes_time_t );
	
	// run all oscillators up to specified time, end current frame, then
	// start a new frame at time 0.
	void end_frame( nes_time_t );
	
	static void begin_debug_log();
private:
	// noncopyable
	Nes_Apu( const Nes_Apu& );
	Nes_Apu& operator = ( const Nes_Apu& );
	
	Nes_Osc*            oscs [osc_count];
	Nes_Square          square1;
	Nes_Square          square2;
	Nes_Noise           noise;
	Nes_Triangle        triangle;
	Nes_Dmc             dmc;
	
	nes_time_t last_time; // has been run until this time in current frame
	nes_time_t earliest_irq_;
	nes_time_t next_irq;
	long next_frame_time; // kept in nes_time_t * 2 (to handle fraction)
	int frame_period;
	int frame;
	int frame_max;
	int irq_flag;
	int irq_enabled;
	void (*irq_notifier_)( void* user_data );
	void* irq_data;
	Nes_Square::Synth square_synth; // shared by squares
	
	enum { no_irq = 0xFFFFFFFF / 2 + 1 };
	
	void irq_changed();
	void state_restored();
	
	friend struct Nes_Dmc;
	friend class Nes_Apu_Reflector;
};

	inline void Nes_Apu::osc_output( int osc, Blip_Buffer* buf ) {
		assert(( "Nes_Apu::osc_output(): Index out of range", 0 <= osc && osc < osc_count ));
		oscs [osc]->output = buf;
	}

	inline nes_time_t Nes_Apu::earliest_irq() const {
		return earliest_irq_;
	}

	inline void Nes_Apu::dmc_reader( int (*func)( void*, nes_addr_t ), void* user_data ) {
		dmc.rom_reader_data = user_data;
		dmc.rom_reader = func;
	}

	inline void Nes_Apu::irq_notifier( void (*func)( void* user_data ), void* user_data ) {
		irq_notifier_ = func;
		irq_data = user_data;
	}
	
	typedef Blip_Buffer Nes_Channel; // deprecated
	typedef blip_sample_t nes_sample_t; // deprecated

#endif

