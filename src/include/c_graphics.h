/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes

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

#ifndef _CGFX_H
#define _CGFX_H

#ifdef __CRUDNES_ALLEGRO
	#pragma warning (disable : 4200)
	#include "allegro.h"
#elif defined CRUDNES_SDL
	#include <sdl.h>
#endif

#include <stdio.h>
#include <string.h>

#include "2xsai.h"
#include "datatypes.h"

#ifdef __CRUDNES_ALLEGRO_GUI
	#include "c_gui.h"
	extern NESGUIHandler NESGUI;
#endif

extern __UINT_16 nes_palette [512];

class c_graphics
{
	public:

		c_graphics (void);
		~c_graphics (void);

		void set_full_screen (__BOOL status) { is_full_screen_mode = status; }
		void toggle_full_screen (void) 
		{
			if (is_full_screen_mode) { set_gfx_mode (GFX_AUTODETECT_WINDOWED, display_w, display_h, 0, 0); is_full_screen_mode = FALSE; }
			else { set_gfx_mode (GFX_AUTODETECT_FULLSCREEN, 320, 240, 0, 0); is_full_screen_mode = TRUE; }	
		}
		void lock_buffer (void) { acquire_bitmap (active_page); }
		void unlock_buffer (void) { release_bitmap (active_page);	}

		void put_pixel (__UINT_32 x_offset, __UINT_32 y_offset, __UINT_16 color)
		{
			#ifdef __CRUDNES_ALLEGRO
				//if (8 == color_depth) ((__UINT_8 *)active_page->line[y_offset])[x_offset] = (__UINT_8)(color);
				//else 
				((__UINT_16 *)active_page->line[y_offset])[x_offset] = color;
			#elif defined CRUDNES_SDL
			#endif
		}

		__UINT_8 * get_pointer (__UINT_32 x_offset, __UINT_32 y_offset) { return ((__UINT_8 *)active_page->line[y_offset]) + x_offset; }
		__UINT_16 * get_pointer16 (__UINT_32 x_offset, __UINT_32 y_offset) { return ((__UINT_16 *)active_page->line[y_offset]) + x_offset; }

		__UINT_8 get_color_depth (void) { return color_depth; }

		void clear (__UINT_16 y_offset, __UINT_16 start, __UINT_16 end, __INT_32 color)
		{
			hline (active_page, start, y_offset, end, color);
		}

		void draw_frame (void);
		void compute_palette (void);

		void request_config_load (void) { config_requested = TRUE; }
		__BOOL is_config_requested (void) { return config_requested; }
		void load_config (void);

		__BOOL is_v_sync_enabled (void) { return v_sync_enabled; }

	private:
		#ifdef __CRUDNES_ALLEGRO
			BITMAP *active_page, *filter_page;
		#elif defined (CRUDNES_SDL)
			SDL_Surface *active_page;
		#endif

		__BOOL is_full_screen_mode, v_sync_enabled, config_requested;

		__UINT_8 *dummy;

		__UINT_8 color_depth;
		__UINT_8 using_filter;
		__UINT_32 display_w, display_h;
};

#endif