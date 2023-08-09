
// Nes_Snd_Emu library is a portable Nintendo Entertainment System (NES) sound
// emulator for adding sound to a NES emulator or handling full emulation and
// synthesis in an NSF music file player.

// Nes_Snd_Emu 0.1.4. Copyright (C) 2003-2004 Shay Green. GNU GPL license.

#ifndef NES_APU_REFLECTOR_H
#define NES_APU_REFLECTOR_H

#include <stdio.h>

#include "Nes_Apu.h"

// Nes_Apu_Reflector allows saving and restoring APU state to/from a text file.
class Nes_Apu_Reflector {
public:
	// save complete APU state to text file.
	static void save( Nes_Apu const&, FILE* );
	
	// Restore complete APU state from text file. APU must have just been
	// reset.
	static void load( FILE*, Nes_Apu& );
	
private:
	static void reflect_apu( Nes_Apu&, FILE*, int load );
	static void reflect_osc( Nes_Osc&, FILE*, int load );
	static void reflect_env( Nes_Envelope&, FILE*, int load );
	static void reflect_sq( Nes_Square&, FILE*, int load );
};

#endif

