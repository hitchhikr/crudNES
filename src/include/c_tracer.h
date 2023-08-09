/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
	o_state Saver/Loader
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

#ifndef _CTRACER_H
#define _CTRACER_H

#include <io.h>
#include <stdarg.h>
#include <stdio.h>

#include "datatypes.h"

#ifndef __DBG_INSTALLING
	#define __DBG_INSTALLING(str) \
		nes->general_log.f_write ("ss", str, ": Installing...")
#endif

#ifndef __DBG_INSTALLED
	#define __DBG_INSTALLED() \
		nes->general_log.f_write ("s", " OK.\r\n")
#endif

#ifndef __DBG_UNINSTALLING
	#define __DBG_UNINSTALLING(str) \
		nes->general_log.f_write ("ss", str, ": Uninstalling...")
#endif

#ifndef __DBG_UNINSTALLED
	#define __DBG_UNINSTALLED() \
		nes->general_log.f_write ("s", " OK.\r\n")
#endif

enum Edestination { __FILE = 0, __WINDOW };
enum EMode { __NEW = 0, __APPEND, __READ };

union label_dat
{
	char string[_1K_];
    int number;
};

class c_tracer {

	public:

		c_tracer ();
		c_tracer (const char *Path, EMode Mode = __NEW, Edestination Type = __FILE);
		~c_tracer ();

		void set_destination (Edestination Type) { destination = Type; }
		__INT_32 set_output_file (const char *, EMode Mode = __NEW);
		
		void close (void);
		void f_write (const char *, ...);
		label_dat *f_read (const char *, ...);

		void read (void *buffer, __UINT_32 size);
		void write (void *buffer, __UINT_32 size);

		FILE *get_handle (void) { return handle; }
        label_dat label_read;

	private:
		Edestination destination;
		__UINT_32 buffer_pos;
		FILE *handle;
		char Buffer [_1K_];
};

#endif