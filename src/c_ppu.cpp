/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Portable 2C02 PPU Core
    Copyright (C) 2003-2004 Sadai Sarmiento
    Copyright (C) 2023-2024 Franck "hitchhikr" Charlet

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

#pragma warning (disable : 4267)

#include <math.h>
#include <process.h>
#include <stdio.h>
#include <string.h>

#include "include/c_nes.h"
#include "include/c_mem_block.h"
#include "include/c_control.h"
#include "include/c_tracer.h"
#include "include/c_graphics.h"
#include "include/mappers/c_mapper.h"
#include "include/mappers/c_mapper_005.h"
#include "include/mappers/c_mapper_009.h"
#include "include/mappers/c_mapper_010.h"
#include "include/c_ppu.h"
#include "include/c_cpu.h"
#include "include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Local Data                                                               **/
/******************************************************************************/

__UINT_16 mirroring_modes [][4] =
{
	{ 0x2000, 0x2000, 0x2400, 0x2400 }, //Horizontal
	{ 0x2000, 0x2400, 0x2000, 0x2400 }, //Vertical
	{ 0x2000, 0x2000, 0x2000, 0x2000 }, //$2000
	{ 0x2000, 0x2400, 0x2800, 0x2c00 }, //Four-screen
	{ 0x2400, 0x2400, 0x2400, 0x2400 }  //$2400

};

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

extern "C"
{
	void RenderTileline (__UINT_8 *destination, __UINT_8 *Solid, __UINT_16 *Palette, __UINT_8 *VRam,  s_rendering_information *information, __UINT_16 Pattern, __UINT_16 Attribute);
}

c_nes_ppu :: c_nes_ppu (void)
{
	__DBG_INSTALLING ("PPU");

	uiCacheIndex = 0;
	lat.contents = 0;
	reg.contents = 0;
	reg_backup.contents = 0;
	reg.fh = 0;
	info.po_flag = FALSE;
	info.sp_limit_flag = FALSE;
	info.po_collision_cycle = 0;
	info.sp_limit_reached_cycle = 0;
	info.scanline = 0;
	info.monochrome = 0xff;
	info.color_emphasis = 0;
	info.is_collision_event_pending = FALSE;
	
	srand(time(NULL));
	bwriteSwitch = FALSE;
	memset (registers, 0, sizeof(registers));

	//Used by the cycle-accurate engine
	is_frame_even = TRUE;
	state_changed = TRUE;

	accurate_tile_index = 0;
	memset (nt_set, 0x00, sizeof(nt_set));
	memset (at_set, 0x00, sizeof(at_set));
	memset (low_pattern_set, 0x00, sizeof(low_pattern_set));
	memset (high_pattern_set, 0x00, sizeof(high_pattern_set));

	//VRAM -> 16K bytes
	//CHRROM -> Size is determined by the ROM header.
	VRAM.resize (_16K_);
	CHRROM.resize (nes->o_rom->information ().chr_pages * _8K_);

	VRAM.clear_to (0x0000, 0x0d);
	background_palette = &VRAM [0x3f00];
	sprite_palette = &VRAM [0x3f10];
	emulator_palette = &nes_palette [0];

	//OAM -> 256 bytes 
	//TMPOAM -> 32 bytes (enough space for eight objects)
	OAM.resize (0x100);
	TMPOAM.resize (32);
	BUFOAM.resize (64);

	OAM.clear_to (0x000, 0x00);
	TMPOAM.clear_to (0x00, 0x00);
	BUFOAM.clear_to (0x00, 0x00);

	oam_inrange_sp = oam_address = 0;
	po_present = FALSE;

	//mirroring set-up
	switch (nes->o_rom->information ().mirroring)
	{
		case _2C02_HORIZONTAL_MIRRORING: 
		case _2C02_VERTICAL_MIRRORING:
		case _2C02_2000_MIRRORING:
		case _2C02_FOURSCREEN_MIRRORING:
			set_mirroring (nes->o_rom->information ().mirroring);
			break;
		default: set_mirroring (_2C02_HORIZONTAL_MIRRORING);
	}

	if (nes->o_rom->information().chr_pages)
	{
		//load CHR-ROM pages into CHRROM (if there are any).
		nes->o_rom->transfer_block (&CHRROM [0], 0x10 + (nes->o_rom->information().prg_pages * _16K_),
		                            nes->o_rom->information ().chr_pages * _8K_);
		//Decodes CHR-ROM pages into a more accessible format for faster rendering.
		is_chr_rom = TRUE;
	    for (__UINT_8 pageIndex = 0; pageIndex < 8; pageIndex ++)
	    {
		    pattern_pages [pageIndex] = &CHRROM [pageIndex * _1K_];
		    pattern_pages_backup [pageIndex] = pageIndex;
        }
	}
	else
	{
		is_chr_rom = FALSE;
	    for (__UINT_8 pageIndex = 0; pageIndex < 8; pageIndex ++)
	    {
		    pattern_pages [pageIndex] = &VRAM [pageIndex * _1K_];
		    pattern_pages_backup [pageIndex] = pageIndex;
        }
	}

    nes->is_mmc2_vrom = FALSE;
    switch(nes->o_rom->information().mapper)
    {
        case 9:
        case 10:
            nes->is_mmc2_vrom = TRUE;
            break;
    }    

	__DBG_INSTALLED ();	
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_nes_ppu :: ~c_nes_ppu (void)
{
	__DBG_UNINSTALLING ("PPU");

	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** run_accurate ()                                                          **/
/**                                                                          **/
/** runs the accurate engine for a whole Scanline.                           **/
/******************************************************************************/

void c_nes_ppu :: run_accurate ()
{
	clear_solid_pixel_lut ();

	info.x_offset = 0;
	info.is_h_blank = FALSE;
	spr_tmp_index = 0;
	spr_ram_index = 0;
	po_present = FALSE;
	oam_inrange_sp = 0;

	__BOOL bSpritesRendered = FALSE;
	
	//Sprite Rendering.
	if (oam_inrange_sp_backup)
	{
		__UINT_8 sprite_data [4];

		for (__UINT_16 spr_tmp_index = 0; spr_tmp_index < (oam_inrange_sp_backup << 3); spr_tmp_index += 8)
		{
			*((__UINT_32 *) (sprite_data)) = BUFOAM.read_dword (spr_tmp_index);
			tileline = BUFOAM.read_word (spr_tmp_index + 4);
            if (nes->is_mmc2_vrom)
            {
                if((tileline & 0xf00) == 0xf00) nes->o_mapper->update (&tileline);
            }

			if (!spr_tmp_index && is_primary_backup)
			{
				render_po_tileline_16 (sprite_data [SPR_X], info.scanline, tileline, sprite_data [SPR_ATTRIBUTE] & 3, sprite_data [SPR_ATTRIBUTE] & BIT_6, sprite_data [SPR_ATTRIBUTE] & BIT_5);
			}
			else
			{
				render_sp_tileline_16 (sprite_data [SPR_X], tileline, sprite_data [SPR_ATTRIBUTE] & 3, sprite_data [SPR_ATTRIBUTE] & BIT_6, sprite_data [SPR_ATTRIBUTE] & BIT_5);
			}
		}

		bSpritesRendered = TRUE;
	}

	if (!info.scanline) update_all_counters ();

	is_mmc5_vrom = FALSE;

	//Surprisingly fast
	//if ((nes->o_rom->information ().mapper == 5) && nes->o_ppu->get_flag (CTL_1, BIT_5) && nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4))
	//	is_mmc5_vrom = TRUE;

	//Background rendering
	bTile = 0;

	if (!get_flag (CTL_2, BIT_1))
	{
		nes->o_gfx->clear (info.scanline, 0, 8, _2C02_NES_COLOR (__UINT_16, background_palette, 0));
	}

    if (nes->is_mmc2_vrom)
    {
	    int changeline = info.bg_pattern_base | (nt_set[0] << 4);
        if((changeline & 0xf00) == 0xf00)
        {
            nes->o_mapper->update (&changeline);
        }
    }
	render_bg_tileline16();

	accurate_tile_index ++;

	//Memory fetch phase #2-128
	for (bTile = 1; bTile < 32; bTile ++, accurate_tile_index ++)
	{
        if (nes->is_mmc2_vrom)
        {
	        int changeline = info.bg_pattern_base |
	                            (nt_set[bTile + 1] << 4);
            if((changeline & 0xf00) == 0xf00)
            {
                nes->o_mapper->update (&changeline);
            }
        }
		render_bg_tileline16 ();
    }
        
	//Memory fetch phase #129-160
	//PPU CC #256 is when the PPU's scroll/address counters have their 
	//horizontal values automatically updated
	//if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("s", "--------------------- Horizontal Counter Updated ----------------------\r\n");
	update_h_counter ();
	
	//This would nicely explain the dummy fetches. Regardless of the value stored in the fine
	//horizontal scroll counter, the PPU must render at least 256 pixels, thus it has got to render
	//an extra tileline. Notice that 8 PPU cycles will be executed, and nametable data corresponding
	//to the first two tiles of the next scanline is fetched.
    if (nes->is_mmc2_vrom)
    {
	    int changeline = info.bg_pattern_base | (nt_set[31] << 4);
        if((changeline & 0xf00) == 0xf00)
        {
            nes->o_mapper->update (&changeline);
        }
    }
	render_bg_tileline16 ();

	tmp_oam_index = 0;
	buf_oam_index = 0;
	
	oam_inrange_sp_backup = oam_inrange_sp;
	is_primary_backup = po_present;
	FetchObjectData ();

	for (__UINT_32 uiIndex = 0; uiIndex < 7; uiIndex ++)
	{
		nes->o_cpu->run_cycles (8);
		FetchObjectData ();
	}

	//Memory fetch phase #161-168
	//Tile #1
	accurate_tile_index = 0;

	Setnt_address ();
	nes->o_cpu->run_cycles (1);
	FetchnametableDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchAttributeData ();
	nes->o_cpu->run_cycles (2);
	SetPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchLowPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	SetPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchHighPatternDataAddress ();
	nes->o_cpu->run_cycles (1);

	if (get_flag (CTL_2, BIT_3 | BIT_4))
	{
		if ((reg.contents & 0x1f) == 0x1f) reg.contents ^= 0x41f;
		else reg.contents ++;
	}

	accurate_tile_index ++;

	//Tile #2
	Setnt_address ();
	nes->o_cpu->run_cycles (1);
	FetchnametableDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchAttributeData ();
	nes->o_cpu->run_cycles (2);
	SetPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchLowPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	SetPatternDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchHighPatternDataAddress ();
	nes->o_cpu->run_cycles (1);

	if (get_flag (CTL_2, BIT_3 | BIT_4))
	{
		if ((reg.contents & 0x1f) == 0x1f) reg.contents ^= 0x41f;
		else reg.contents ++;
	}

	accurate_tile_index ++;

	//Memory fetch phase #169-170
	nes->o_cpu->run_cycles (4);

	//Dead cycle
	if (info.scanline || nes->o_cpu->is_pal ()) nes->o_cpu->run_cycles (1);
	else 
	{
		if (is_frame_even) nes->o_cpu->run_cycles (1);
		is_frame_even = !is_frame_even;
	}	

	if (5 == nes->o_rom->information ().mapper) nes->o_mapper->h_blank ();
}

void c_nes_ppu :: render_bg_tileline16 (void)
{
	if (!bTile) unpacked_pixel_index = 0;

	register __UINT_16 *destination = nes->o_gfx->get_pointer16 (info.x_offset, info.scanline);
	register __UINT_8 *solid = solid_pixel_lut + info.x_offset;
	register __UINT_8 * const pixel_data = unpacked_pixel_data + unpacked_pixel_index;

	if (bTile) 
	{
		_2C02_UNPACK_TILELINE_SLOW (low_pattern_set [accurate_tile_index - 1], high_pattern_set [accurate_tile_index - 1], (unpacked_pixel_data + ((accurate_tile_index - 1) << 3)));
	}
	else
	{
		unpacked_pixel_index = reg.fh;
		_2C02_UNPACK_TILELINE_SLOW (low_pattern_set [0], high_pattern_set [0], pixel_data);
		_2C02_UNPACK_TILELINE_SLOW (low_pattern_set [1], high_pattern_set [1], (pixel_data + 8));
	}

	__UINT_16 final_color;
	__UINT_8 attribute = at_set [(unpacked_pixel_index >> 3)];

	if (bTile != 32) EvaluateOAMEntry ();

	_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
	Setnt_address ();
	nes->o_cpu->run_cycles (1);
	attribute = at_set [(unpacked_pixel_index >> 3)];
	_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
    FetchnametableDataAddress ();
	nes->o_cpu->run_cycles (1);
	FetchAttributeData ();
	attribute = at_set [(unpacked_pixel_index >> 3)];
	_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
	nes->o_cpu->run_cycles (1);
	attribute = at_set [(unpacked_pixel_index >> 3)];
	_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
	if (bTile != 31)
	{
		nes->o_cpu->run_cycles (1);
		if (bTile == 32 && nes->o_rom->information ().mapper != 5) nes->o_mapper->h_blank ();
	}
	else
	{
		nes->o_cpu->run_cycles (1);
		update_v_counter ();
	}
	if (bTile != 32) EvaluateOAMEntry ();
	if (bTile != 31)
	{
		//FetchLowPatternData ();
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		SetPatternDataAddress ();
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		is_mmc5_vrom = ((nes->o_rom->information ().mapper == 5) && nes->o_ppu->get_flag (CTL_1, BIT_5) && nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4)) ? TRUE : FALSE;
		FetchLowPatternDataAddress ();
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		SetPatternDataAddress ();
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		is_mmc5_vrom = ((nes->o_rom->information ().mapper == 5) && nes->o_ppu->get_flag (CTL_1, BIT_5) && nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4)) ? TRUE : FALSE;
		FetchHighPatternDataAddress ();
		nes->o_cpu->run_cycles (1);
	}
	else 
	{
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		nes->o_cpu->run_cycles (1);
		attribute = at_set [(unpacked_pixel_index >> 3)];
		_2C02_DRAW_ACCURATE_PIXEL (__UINT_16, unpacked_pixel_data, final_color, attribute, destination, solid, unpacked_pixel_index);
		nes->o_cpu->run_cycles (1);
	}

	if (bTile != 32 && get_flag (CTL_2, BIT_3 | BIT_4))
	{
		if ((reg.contents & 0x1f) == 0x1f) reg.contents ^= 0x41f;
		else reg.contents ++;
	}

	info.x_offset += 8;
}

void c_nes_ppu :: render_bg_tileline_fh_16 (__UINT_16 y_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 x_start, __UINT_8 x_end)
{
	register __UINT_16 pattern = read_pattern_word (tileline);

	if (!pattern) { info.x_offset += x_end - x_start; return; }

	//Basic pattern matching. This speeds things up quite a bit...
	if ((pattern == 0xffff || pattern == 0x5555 || pattern == 0xaaaa)
		&& (info.x_offset || !reg.fh))
	{
		__UINT_32 *destination = (__UINT_32 *)(nes->o_gfx->get_pointer16 (info.x_offset, y_offset));
		__UINT_32 *solid = (__UINT_32 *)(solid_pixel_lut + info.x_offset);
		register __UINT_32 color = _2C02_NES_COLOR (__UINT_16, background_palette, attribute | (pattern & 3));
		color |= color << 16;
        *(destination) = color;			
		*(destination + 1) = color;
		*(destination + 2) = color;
		*(destination + 3) = color;
		*(solid) = 0x01010101;
		*(solid + 1) = 0x01010101;				
		info.x_offset += 8;
	}
	else
	{
		if (uiPatternCache [0] != pattern)
		{
			_2C02_UNPACK_TILELINE_FAST (pattern, bg_pixel_data);
			uiPatternCache [0] = pattern;
		}

		__UINT_16 *destination = nes->o_gfx->get_pointer16 (info.x_offset, y_offset);
		__UINT_8 *solid = solid_pixel_lut + info.x_offset;

		info.x_offset += x_end - x_start;

		for (; x_start < x_end; x_start ++, destination++, solid ++)
		{
			if (!bg_pixel_data [x_start]) continue;

			*(destination) =  _2C02_NES_COLOR (__UINT_16, background_palette, attribute | bg_pixel_data [x_start]);
			*(solid) = BIT_0;
		}
	}
}

void c_nes_ppu :: render_bg_tileline16 (__UINT_16 y_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 x_start, __UINT_8 x_end)
{
	register __UINT_16 pattern = read_pattern_word (tileline);

	if (!pattern)
	{
	    info.x_offset += 8; return;
	}

	//Basic pattern matching
	if ((pattern == 0xffff || pattern == 0x5555 || pattern == 0xaaaa))
	{
		__UINT_32 *destination = (__UINT_32 *)(nes->o_gfx->get_pointer16 (info.x_offset, y_offset));
		__UINT_32 *solid = (__UINT_32 *)(solid_pixel_lut + info.x_offset);
		register __UINT_32 color = _2C02_NES_COLOR (__UINT_16, background_palette, attribute | (pattern & 3));
		color |= color << 16;
        *(destination) = color;			
		*(destination + 1) = color;
		*(destination + 2) = color;
		*(destination + 3) = color;
		*(solid) = 0x01010101;
		*(solid + 1) = 0x01010101;				
		info.x_offset += 8;
	}
	else
	{
		//Pattern caching (Experimental)
		if (uiPatternCache [0] != pattern)
		{
			_2C02_UNPACK_TILELINE_FAST (pattern, bg_pixel_data);
			uiPatternCache [0] = pattern;
			//uiCacheIndex = (uiCacheIndex + 1) & 3;
		}

		__UINT_16 *destination = nes->o_gfx->get_pointer16 (info.x_offset, y_offset);
		__UINT_8 *solid = solid_pixel_lut + info.x_offset;
		info.x_offset += 8;

		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 0);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 1);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 2);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 3);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 4);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 5);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 6);
		_2C02_DRAW_FAST_PIXEL (__UINT_16, bg_pixel_data, destination, 7);
	}
}

void c_nes_ppu :: render_po_tileline_16 (__UINT_16 x_offset, __UINT_16 y_offset, __UINT_16 line, __UINT_32 attribute, __BOOL SPXFlip, __BOOL priority)
{
	__UINT_8 pixel_data [8];

    register __UINT_8 lower_bits = read_pattern_byte (line),
					higher_bits = read_pattern_byte (line + 8);
	_2C02_UNPACK_TILELINE_SLOW (lower_bits, higher_bits, pixel_data);

	__UINT_8 flip_mask = (SPXFlip) ? 7 : 0;

	__UINT_8 tileline_offset = 0;
	if (!(registers [CTL_2] & BIT_2) && (x_offset < 8)) { tileline_offset = 8 - x_offset; x_offset = 8; }

	attribute <<= 2;

	for (; tileline_offset < 8 && x_offset < 256; tileline_offset ++, x_offset++)
	{
		if (!(pixel_data [tileline_offset ^ flip_mask])) continue;

		solid_pixel_lut [x_offset] |= (BIT_1 | BIT_2 | BIT_4);

		_2C02_DRAW_PIXEL (x_offset, y_offset, _2C02_NES_COLOR (__UINT_16, sprite_palette, attribute | pixel_data [tileline_offset ^ flip_mask]));
		
		if (priority) solid_pixel_lut [x_offset] |= BIT_3;
	}
}

void c_nes_ppu :: render_sp_tileline_16 (__UINT_16 x_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 flip_mask, __BOOL priority)
{
	__UINT_8 pixel_data [8];

    register __UINT_8 lower_bits = read_pattern_byte (tileline),
					higher_bits = read_pattern_byte (tileline + 8);
	_2C02_UNPACK_TILELINE_SLOW (lower_bits, higher_bits, pixel_data);

	flip_mask = (flip_mask) ? 7 : 0;

	__UINT_8 tileline_offset = 0;
	if ((x_offset < 8) && !(registers [CTL_2] & BIT_2)) { tileline_offset = 8 - x_offset; x_offset = 8; }

	for (; tileline_offset < 8; tileline_offset ++, x_offset++)
	{
		if (!pixel_data [tileline_offset ^ flip_mask]) continue;
		if (priority)
		{
			if (!(solid_pixel_lut [x_offset] & BIT_1))
			{
				_2C02_DRAW_PIXEL (x_offset, info.scanline, _2C02_NES_COLOR (__UINT_16, sprite_palette, (attribute << 2)| pixel_data [tileline_offset ^ flip_mask]));
				solid_pixel_lut [x_offset] |= (BIT_2 | BIT_3);
			}
			solid_pixel_lut [x_offset] |= BIT_1;
		}
		else if (!(solid_pixel_lut [x_offset] & BIT_1))
		{
			_2C02_DRAW_PIXEL (x_offset, info.scanline, _2C02_NES_COLOR (__UINT_16, sprite_palette, (attribute << 2) | pixel_data [tileline_offset ^ flip_mask]));
			solid_pixel_lut [x_offset] |= (BIT_1 | BIT_2);
		}
	}
}

void c_nes_ppu :: FetchObjectData (void)
{
	if (!get_flag (CTL_2, BIT_3 | BIT_4) || !oam_inrange_sp_backup) return;

	__UINT_8 sprite_data [4];
	*((__UINT_32 *) (sprite_data)) = TMPOAM.read_dword (tmp_oam_index);

	if (sprite_data [SPR_ATTRIBUTE] & BIT_7) sprite_data [SPR_TILELINE] ^= (0xf >> (get_flag (CTL_1, BIT_5) ? 0 : 1));

	if (!get_flag (CTL_1, BIT_5))
	{
		tileline = (sprite_data [SPR_TILELINE]) + (((registers [CTL_1] & BIT_3) >> 3) << 12) + (sprite_data [SPR_TILENUM] << 4);
    }
	else
	{
		__UINT_16 sp_pattern_base = (sprite_data [SPR_TILENUM] & BIT_0) << 12;
		sprite_data [SPR_TILENUM] &= ~BIT_0;
		sprite_data [SPR_TILENUM] |= ((sprite_data [SPR_TILELINE] & BIT_3) >> 3);
		sprite_data [SPR_TILELINE] &= 7;
		tileline = (sprite_data [SPR_TILELINE]) + (sp_pattern_base + (sprite_data [SPR_TILENUM] << 4));
	}

	BUFOAM.write_dword (buf_oam_index, (*(__UINT_32 *)(sprite_data)));
	BUFOAM.write_word (buf_oam_index + 4, tileline);
	
	buf_oam_index += 8;
	tmp_oam_index += 4;
}

/******************************************************************************/
/** read_byte ()                                                             **/
/**                                                                          **/
/** Retrieves a byte from PPU registers / memory.                            **/
/******************************************************************************/

__UINT_8 c_nes_ppu :: read_byte (__UINT_16 address)
{
    register __UINT_8 value = 0x40;

	switch (address & 7)
    {
    	case 0x00:
			value = registers [CTL_1];
			break;

    	case 0x01:
			value = registers [CTL_2];
			break;

    	case 0x03:
			value = registers [SPRMEM];
			break;

    	case 0x05:
			value = registers [SCROLL];
			break;

    	case 0x06:
			value = registers [MEM];
			break;

    	case 0x02:
			if (IsEventPending (POC) && (_2A03_get_current_time () >= info.po_collision_cycle))
				AcknowledgeEvent (POC);

			value = registers [STAT];
			clear_flag (STAT, BIT_7);
			bwriteSwitch = FALSE;
			return value;

		case 0x07:
		{
			if (!info.is_v_blank
				&& get_flag (CTL_2, BIT_4 | BIT_3))
			{
				if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("s", "PPU: WARNING: Invalid read at $2007.\r\n");
				return 0x40;
			}

			if(nes->o_mapper->mapper_185 >= 1 && nes->o_mapper->mapper_185 < 3)
			{
				value = rand();
				nes->o_mapper->mapper_185++;
			}
			else
			{
				register __UINT_16 address = reg.contents & 0x3fff;
				value = read_read_buffer ();
			}

			if (address < 0x2000) set_read_buffer (read_chr_ram (address));
			else if (address < 0x3000) set_read_buffer (read_nt_byte (address));
			else if (address < 0x3f00) set_read_buffer (read_nt_byte (address - 0x1000));
			else if (address < 0x4000) value = background_palette [address & 0x1f];

			reg.contents += ((get_flag (CTL_1, BIT_2)) ? 32 : 1);
			break;
		}

		case 0x04: 
			value = read_oam_byte ();
			if (!info.is_v_blank && get_flag (CTL_2, BIT_4 | BIT_3))
				oam_address ++;
			break;
	}

	return value;
}

/******************************************************************************/
/** write_byte ()                                                            **/
/**                                                                          **/
/** writes a byte to PPU registers / memory.                                 **/
/******************************************************************************/

void c_nes_ppu :: write_byte (__UINT_16 address, __UINT_8 value)
{
    switch (address & 7)
    {
		case 0x07:
		{
			if (!info.is_v_blank
				&& get_flag (CTL_2, BIT_4 | BIT_3))
			{
				if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("sbs", "PPU: WARNING: Ignoring invalid write at $2007 Data: ", value, ".\r\n");
				return;
			}
            
			register __UINT_16 address = reg.contents & 0x3fff;
			
			if (address < 0x2000) { write_chr_ram (address, value); }
			else if (address < 0x3000) { write_nt_byte (address, value); }
			else if (address < 0x3f00) { write_nt_byte ((reg.contents - 0x1000) & 0x3fff, value);}
			else if (address < 0x4000)
			{
				value &= 0x7f;
                
				if (!(address & 0xf))
				{
					background_palette [0] = value;
					sprite_palette [0] = value;
				}
				else background_palette [address & 0x1f] = value;
			}

			reg.contents += ((get_flag (CTL_1, BIT_2)) ? 32 : 1);
			return;
		}

		case 0x06:
			if (!bwriteSwitch)
			{
				lat.contents = ((lat.contents & 0xff) | ((((__UINT_16) value) & 0x3f) << 8));
			}
			else
			{
				lat.contents = ((lat.contents & 0xff00) | (__UINT_16) value);
				//if (((reg.contents & 0x3fff) >= 0x3f00) && ((reg.contents & 0x3fff) < 0x3f10))
				//	bLastColorSelected = reg.contents & 0xf;
				if (4 == nes->o_rom->information ().mapper)
				{
					if ((lat.contents & 0x1000) && !(reg.contents & 0x1000)) nes->o_mapper->update (0);
				}
				reg.contents = lat.contents;
			}
			bwriteSwitch = !bwriteSwitch;
			return;

		case 0x05:
			if (!bwriteSwitch)
			{
				lat.contents = ((lat.contents & 0xffe0) | (((__UINT_16) value) >> 3));
				reg.fh = value & 7;
			}
			else
			{
				lat.contents = ((lat.contents & 0x8c1f) | ((((__UINT_16) value) & ~0x7) << 2));
				lat.contents = ((lat.contents & 0x8fff) | ((((__UINT_16) value) & 0x7) << 12));
			}
			bwriteSwitch = !bwriteSwitch;
			return;

		case 0x01:
			info.monochrome = (value & 1) ? 0xf0 : 0xff;
			info.color_emphasis = (value >> 5) << 6;
			emulator_palette = &nes_palette [info.color_emphasis];
			registers [CTL_2] = value;
			return;

		case 0x03: 
			set_oam_address (value);
			return;

		case 0x04:
			if (!info.is_v_blank
				&& get_flag (CTL_2, BIT_4 | BIT_3))
			{
				if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("sb", "\r\nPPU: WARNING: Invalid write at $2004 Data: ", value);
				return;
			}
			write_oam_byte (value);
			return;

		case 0x00:
			lat.contents = ((lat.contents & 0xf3ff) | (((__UINT_16) value & 3) << 10));
			
			if (!get_flag (CTL_1, BIT_7) && (value & BIT_7) && get_flag (STAT, BIT_7))
			{
				if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("sds", "PPU: WARNING: Extra NMI requested at scanline #", info.scanline, ".\r\n");
				nes->o_cpu->request_secondary_nmi ();
			}

			registers [CTL_1] = value;
			info.bg_pattern_base = (value & BIT_4) << 8;
			info.sp_pattern_base = (value & BIT_3) << 9;
			return;
	}
}

void c_nes_ppu :: swap_page (__UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size)
{
	if (!nes->o_rom->information ().chr_pages) return;
//	if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("sbsws", "PPU: Page ", page_number, " swapped at ", dest_where, "\r\n");

    is_chr_rom = TRUE;
    // divide by 1024
	dest_where >>= 10;
	__UINT_32 uiRealSize = size >> 10;
	pattern_pages_backup [dest_where] = page_number * uiRealSize;
	pattern_pages [dest_where++] = &CHRROM [page_number * size];

	for (__UINT_8 page = 1; page < uiRealSize; page ++, dest_where ++)
	{
		pattern_pages_backup [dest_where] = pattern_pages_backup [dest_where - 1] + 1;
		pattern_pages [dest_where] = pattern_pages [dest_where - 1] + _1K_;
	}
}

void c_nes_ppu :: swap_page (__UINT_8 **destination, __UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size)
{
	if (!nes->o_rom->information ().chr_pages) return;
//	if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("sbsws", "PPU: Page ", page_number, " swapped at ", dest_where, "\r\n");

    is_chr_rom = TRUE;
	dest_where >>= 10;
	__UINT_32 uiRealSize = size >> 10;
	destination [dest_where ++] = &CHRROM [page_number * size];

	for (__UINT_8 page = 1; page < uiRealSize; page ++, dest_where ++)
	{
		destination [dest_where] = destination [dest_where - 1] + _1K_;
    }	

}

/******************************************************************************/
/** decode_chr_rom ()                                                        **/
/**                                                                          **/
/** Unpacks pattern data into a more accessible format (for speed purposes). **/
/******************************************************************************/

void c_nes_ppu :: decode_chr_rom (void)
{
	__UINT_8 lower_bits, higher_bits;
	__UINT_16 t_tileline [8];

	for (__UINT_32 i = 0; i < CHRROM.get_size () - 8;)
	{
		for (__UINT_32 j = 0; j < 8; j ++)
		{
			lower_bits = CHRROM.read_byte (i + j);
			higher_bits = CHRROM.read_byte (i + j + 8);
			
			t_tileline [j] = ((higher_bits & BIT_7) << 8) | ((lower_bits & BIT_7) << 7);
			t_tileline [j] |= ((higher_bits & BIT_6) << 7) | ((lower_bits & BIT_6) << 6);
			t_tileline [j] |= ((higher_bits & BIT_5) << 6) | ((lower_bits & BIT_5) << 5);
			t_tileline [j] |= ((higher_bits & BIT_4) << 5) | ((lower_bits & BIT_4) << 4);
			t_tileline [j] |= ((higher_bits & BIT_3) << 4) | ((lower_bits & BIT_3) << 3);
			t_tileline [j] |= ((higher_bits & BIT_2) << 3) | ((lower_bits & BIT_2) << 2);
			t_tileline [j] |= ((higher_bits & BIT_1) << 2) | ((lower_bits & BIT_1) << 1);
			t_tileline [j] |= ((higher_bits & BIT_0) << 1) | ((lower_bits & BIT_0) << 0);
		}

		for (__UINT_8 k = 0; k < 8; k ++, i += 2)
		{
			CHRROM.write_word (i, t_tileline [k]);
        }
	}
}

void c_nes_ppu :: write_chr_ram (__UINT_16 address, __UINT_8 value)
{	
	is_chr_rom = FALSE;

	if (nes->o_rom->information ().chr_pages)
	{
		for (__UINT_8 page = 0; page < 8; page ++)
		{
			VRAM.load_from (&CHRROM, pattern_pages_backup [page] * _1K_, page << 10, _1K_);
        }
	}

	VRAM [address] = value;
}

__UINT_8 c_nes_ppu :: read_chr_ram (__UINT_16 address)
{
	if (is_chr_rom) return read_pattern_byte (address);
	else return VRAM [address];
}

void c_nes_ppu :: save_state (c_tracer &o_writer, e_save_state Type)
{
	switch (Type)
	{
		case MAIN:
			OAM.dump_to (o_writer, NULL, 0x000, 0x100, BINARY);
			VRAM.dump_to (o_writer, NULL, 0x0000, _16K_, BINARY);
            break;
		case OTHER:
			o_writer.write (&oam_address, 1);
			o_writer.write (&info.bg_pattern_base, 4);
			o_writer.write (&info.sp_pattern_base, 4);
			o_writer.write (&info.color_emphasis, 4);
			o_writer.write (&info.monochrome, 4);
			o_writer.write (&info.scanline, 4);
			o_writer.write (&reg.contents, 2);
			o_writer.write (&lat.contents, 2);
			o_writer.write (&reg.fh, 1);
			o_writer.write (&mirroring_mode, 1);
			o_writer.write (&is_chr_rom, 1);
			o_writer.write (&bwriteSwitch, 1);
			o_writer.write (&state_changed, 1);
			o_writer.write (registers, 8);
			o_writer.write (pattern_pages_backup, 16);
			break;
	}
}

void c_nes_ppu :: load_state (c_tracer &o_reader, e_save_state Type)
{
	switch (Type)
	{
		case MAIN:
			o_reader.read (&OAM [0], 0x100);
			o_reader.read (&VRAM [0], _16K_);
			break;
		case OTHER:
		{
			o_reader.read (&oam_address, 1);
			o_reader.read (&info.bg_pattern_base, 4);
			o_reader.read (&info.sp_pattern_base, 4);
			o_reader.read (&info.color_emphasis, 4);
			o_reader.read (&info.monochrome, 4);
			o_reader.read (&info.scanline, 4);
			o_reader.read (&reg.contents, 2);
			o_reader.read (&lat.contents, 2);
			o_reader.read (&reg.fh, 1);
			o_reader.read (&mirroring_mode, 1);
			o_reader.read (&is_chr_rom, 1);
			o_reader.read (&bwriteSwitch, 1);
			o_reader.read (&state_changed, 1);
			o_reader.read (registers, 8);
			o_reader.read (pattern_pages_backup, 16);

			if (is_chr_rom)
			{
				for (__UINT_8 index = 0; index < 8; index ++)
				{
					pattern_pages [index] = &CHRROM [pattern_pages_backup [index] * _1K_];
                }
            }
			
			if (nes->o_rom->information ().mapper != 5) set_mirroring (mirroring_mode);
			else ((c_mapper_005 *)(nes->o_mapper))->set_mirroring ();
		}
    	break;
	}
}
