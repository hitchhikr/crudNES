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

#pragma warning (disable : 4267 4311)

#ifdef __CRUDNES_ALLEGRO
	#pragma warning (disable : 4200)
	#include "include/allegro.h"
#elif defined CRUDNES_SDL
	#include <sdl.h>
#endif

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "include/2xSai.h"
#include "include/c_tracer.h"
#include "include/c_graphics.h"
#include "include/datatypes.h"
#include "include/c_ppu.h"
#include "include/c_nes.h"

__UINT_8 bPalette [64][3];
__UINT_16 nes_palette [512];

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;
extern c_nes_cpu *o_cpu;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_graphics :: c_graphics (void)
{
	__DBG_INSTALLING ("Graphics");

	active_page = NULL;
	filter_page = NULL;
	dummy = NULL;

	load_config ();

	Init_2xSaI (565);

	clear_to_color (screen, nes_palette [0x0e]);
	gui_bg_color = nes_palette [0x0e];
	gui_fg_color = nes_palette [0x1a];

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_graphics :: ~c_graphics (void)
{
//	__DBG_UNINSTALLING ("Graphics");

	if (active_page) { destroy_bitmap (active_page); }
	if (filter_page) { destroy_bitmap (filter_page); __DELETE_MEM_BLOCK (dummy); }

//	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** draw_frame ()                                                             **/
/******************************************************************************/

void c_graphics :: draw_frame (void)
{
	static __INT_32 ulEllapsedTime = (__INT_32)(time (0)), ulFPS = 0;

	ulFPS ++;    	

    if (ulEllapsedTime != time (0)) {

		char Buffer [10];
		sprintf (Buffer, "%i FPS", ulFPS);
		
		if (is_full_screen_mode);//textout (screen, font, Buffer, 0, 0, nes_palette [0x10]);
		else set_window_title (Buffer);
    	ulEllapsedTime = (__INT_32)(time (0));
		ulFPS = 0;
	}

	if (display_w == 256 && display_h == nes->o_cpu->Height)
		blit (active_page, screen, 0, 8, 0, 0, display_w, display_h);
	else
	{
		switch (using_filter)
		{
		    case 0: 
			    stretch_blit (active_page, screen, 0, 8, 256, nes->o_cpu->Height, 0, 0, display_w, display_h);
			    break;
		    case 1:
			    Super2xSaI (active_page->line[8], (unsigned int)(active_page->line[1] - active_page->line[0]), NULL, filter_page, 256, nes->o_cpu->Height);
			    blit (filter_page, screen, 0, 0, 0, 0, display_w, display_h);
			    break;
		    case 2:
			    SuperEagle (active_page->line[8], (unsigned int)(active_page->line[1] - active_page->line[0]), dummy, filter_page, 256, nes->o_cpu->Height);
			    blit (filter_page, screen, 0, 0, 0, 0, display_w, display_h);
			    break;
		}
	}
}

#define PDX_MIN(x_offset,y) ((x_offset) < (y) ? (x_offset) : (y))
#define PDX_MAX(x_offset,y) ((x_offset) < (y) ? (y) : (x_offset))

#define PDX_CLAMP(x_offset,min_,max_) PDX_MAX(PDX_MIN(x_offset,max_),min_)

static const double emphasis[8][3] =
{
	{1.000, 1.000, 1.000},
	{1.239, 0.915, 0.743},
	{0.794, 1.086, 0.882},
	{1.019, 0.980, 0.653},
	{0.905, 1.026, 1.277},
	{1.023, 0.908, 0.979},
	{0.741, 0.987, 1.001},
	{0.750, 0.750, 0.750}
};

void ToHSV(double r,double g,double b,double& h,double& s,double& v)
{
	const double min = PDX_MIN( r, PDX_MIN( g, b ));
	const double max = PDX_MAX( r, PDX_MAX( g, b ));

	v = max;

	if (max != 0)
	{
		const double delta = max - min;

		s = delta / max;

		     if (r == max) h = 0 + (g - b) / delta;
		else if (g == max) h = 2 + (b - r) / delta;
		else			   h = 4 + (r - g) / delta;

		h *= 60;

		if (h < 0)
			h += 360;
	}
	else
	{
		s =  0;
		h = -1;
	}
}

void ToRGB(double h,double s,double v,double& r,double& g,double& b)
{
	if (s == 0)
	{
		r = g = b = v;
	}
	else
	{
		h /= 60;
		
		const int i = (int)(floor (h));

		const double f = h - i;
		const double p = v * ( 1 - s );
		const double q = v * ( 1 - s * f );
		const double t = v * ( 1 - s * ( 1 - f ) );

		switch (i) 
		{
     		case 0:  r = v; g = t; b = p; return;
			case 1:  r = q; g = v; b = p; return;
			case 2:  r = p; g = v; b = t; return;
			case 3:  r = p; g = q; b = v; return;
			case 4:  r = t; g = p; b = v; return;
			default: r = v; g = p; b = q; return;
		}
	}
}

void c_graphics :: compute_palette (void)
{
	unsigned char brightness = 128,
				  saturation = 128,
				  hue = 128;

	const double bri = (brightness - 128) / 255.0;
	const double sat = (((saturation - 128) / 255.0) * 2) + 1;
	const int hof = (hue - 128) / 4;

	const unsigned char (*const from)[3] = bPalette;

	for (unsigned short int i=0; i < 8; ++i)
	{
		for (unsigned short int j=0; j < 64; ++j)
		{
			double r = from [j][0] / 255.0;
			double g = from [j][1] / 255.0;
			double b = from [j][2] / 255.0;

			double h,s,v;

			ToHSV (r,g,b,h,s,v);

			s *= sat;
			v += bri;
			h -= hof;

			if (h >= 360)
				h -= 360;
			else if (h < 0)
				h += 360;

			ToRGB (h,s,v,r,g,b);

			r *= emphasis[i][0];
			g *= emphasis[i][1]; 
			b *= emphasis[i][2]; 

			__UINT_8 rr = __UINT_8 (PDX_CLAMP(r * 255,0,255));
			__UINT_8 gg = __UINT_8 (PDX_CLAMP(g * 255,0,255));
			__UINT_8 bb = __UINT_8 (PDX_CLAMP(b * 255,0,255));

            nes_palette [(i * 64) + j] = makecol (rr, gg, bb);
		}
	}
}

void c_graphics :: load_config (void)
{
	config_requested = FALSE;

	int bValue;
    int iValue;

	using_filter = (__UINT_8) 0;

    iValue = 1;

	switch (iValue)
	{
		case 0:	if (!using_filter) color_depth = 8; else color_depth = 16; break;
		case 1: color_depth = 16; break;
	}
	
    v_sync_enabled = 1;

    iValue = 2;

	switch (iValue)
	{
		case 0: display_w = 256; display_h = nes->o_cpu->Height; break;
		case 2: display_w = 512; display_h = nes->o_cpu->Height * 2; break;
	}

    iValue = 0;

	if (!iValue) { iValue = GFX_AUTODETECT_WINDOWED; is_full_screen_mode = FALSE; }
	else { iValue = GFX_AUTODETECT_FULLSCREEN; is_full_screen_mode = TRUE; /*win_set_window (NULL);*/ }

	set_color_depth (color_depth);
	if (set_gfx_mode (iValue, display_w, display_h, 0, 0) < 0)
	{
		if (display_w == 512) set_gfx_mode (GFX_AUTODETECT_FULLSCREEN, 640, 480, 0, 0);
		else if (display_w == 768) set_gfx_mode (GFX_AUTODETECT_FULLSCREEN, 800, 600, 0, 0);
		else set_gfx_mode (GFX_AUTODETECT_FULLSCREEN, 320, 240, 0, 0);
	}

	if (active_page) destroy_bitmap (active_page);
	active_page = create_bitmap (256 + 8, nes->o_cpu->Height + 16);

	bValue = 1;
	
	if (bValue) set_display_switch_mode(SWITCH_BACKGROUND);
	else set_display_switch_mode(SWITCH_PAUSE);

	if (using_filter)
	{
		if (filter_page) { destroy_bitmap (filter_page); filter_page = NULL; }
		if (dummy) __DELETE_MEM_BLOCK (dummy);
		filter_page = create_bitmap (display_w, display_h);
		__NEW_MEM_BLOCK (dummy, __UINT_8, display_w * display_h << 1);
	}

	c_tracer o_reader;
    // either Matrixz.pal or Fce.pal
	if (o_reader.set_output_file("Matrixz.pal", __READ))
	{
	    o_reader.set_output_file ("Fce.pal", __READ);
	}
	o_reader.read ((__UINT_8 *)bPalette, 64 * 3);
	o_reader.close ();

	if (8 == color_depth)
	{
		bPalette [0x21] [0] = bPalette [0x2c] [0];
		bPalette [0x21] [1] = bPalette [0x2c] [1];
		bPalette [0x21] [2] = bPalette [0x2c] [2];
	}

	compute_palette ();

	remove_keyboard ();
	install_keyboard ();
}
