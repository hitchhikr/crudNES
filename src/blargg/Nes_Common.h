
// Nes_Snd_Emu library is a portable Nintendo Entertainment System (NES) sound
// emulator for adding sound to a NES emulator or handling full emulation and
// synthesis in an NSF music file player.

// Copyright (c) 2003-2004 Shay Green; licensed under GNU GPL (see source)

#ifndef NES_SND_EMU_NES_COMMON_H
#define NES_SND_EMU_NES_COMMON_H

#include "Blip_Buffer.h"

typedef long nes_time_t; // clock cycle count

// Quality varies from 0 to 2, the higher the better. Compared to
// quality 0, quality 2 uses four times the memory and gives about
// half the performance. The overall quality difference is only
// slight, but probably worth it on modern desktop PCs.
#ifndef NES_SND_EMU_QUALITY
	#define NES_SND_EMU_QUALITY 1
#endif

	typedef Blip_Buffer Nes_Channel; // deprecated
	typedef blip_sample_t nes_sample_t; // deprecated

#endif

