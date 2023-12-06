/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Portable 2C02 PPU Core
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

#ifndef _CNESPPU_H
#define _CNESPPU_H

#include "c_mem_block.h"
#include "c_control.h"
#include "c_graphics.h"
#include "c_rom.h"
#include "c_cpu.h"
#include "c_save_state.h"
#include "datatypes.h"
#include "mappers/c_mapper_005.h"

//////////////////////////////////////////////////////////////////
// Hard-coded mirroring modes. This ought to be used within any 
// mapper-related context.
//////////////////////////////////////////////////////////////////

#define _2C02_HORIZONTAL_MIRRORING 0
#define _2C02_VERTICAL_MIRRORING 1
#define _2C02_2000_MIRRORING 2
#define _2C02_2400_MIRRORING 4
#define _2C02_FOURSCREEN_MIRRORING 3

#define SPR_TILENUM 0
#define SPR_X 2
#define SPR_ATTRIBUTE 1
#define SPR_TILELINE 3

#define _2C02_DRAW_PIXEL(x_offset,y_offset,iColor) \
{ \
	nes->o_gfx->put_pixel (x_offset, y_offset, iColor); \
}

#define _2C02_UNPACK_TILELINE_FAST(uiSrc,destinationArray) \
{ \
    destinationArray [0] = uiSrc >> 14; \
	destinationArray [1] = (uiSrc >> 12) & 3; \
	destinationArray [2] = (uiSrc >> 10) & 3; \
	destinationArray [3] = (uiSrc >> 8) & 3; \
	destinationArray [4] = (uiSrc >> 6) & 3; \
	destinationArray [5] = (uiSrc >> 4) & 3; \
	destinationArray [6] = (uiSrc >> 2) & 3; \
	destinationArray [7] = uiSrc & 3; \
}

#define _2C02_UNPACK_TILELINE_SLOW(bSrcLow,bSrcHigh,destinationArray) \
{ \
	destinationArray [0] = (((bSrcHigh & 128) >> 6) | ((bSrcLow & 128) >> 7)); \
	destinationArray [1] = (((bSrcHigh & 64) >> 5)  | ((bSrcLow & 64) >> 6) ); \
	destinationArray [2] = (((bSrcHigh & 32) >> 4)  | ((bSrcLow & 32) >> 5) ); \
	destinationArray [3] = (((bSrcHigh & 16) >> 3)  | ((bSrcLow & 16) >> 4) ); \
	destinationArray [4] = (((bSrcHigh & 8) >> 2)   | ((bSrcLow & 8) >> 3)  ); \
	destinationArray [5] = (((bSrcHigh & 4) >> 1)   | ((bSrcLow & 4) >> 2)  ); \
	destinationArray [6] = ((bSrcHigh  & 2)         | ((bSrcLow & 2) >> 1)  ); \
	destinationArray [7] = ((bSrcHigh & 1) << 1)    | ((bSrcLow & 1)        ); \
}

#define _2C02_NES_COLOR(Cast,Palette,uiIndex) ((Cast) (emulator_palette [Palette [uiIndex] & info.monochrome]))

#define _2C02_DRAW_FAST_PIXEL(Cast,Storage,destination,Index) \
{ \
	if (Storage [Index]) \
	{ \
		*(destination + Index) = _2C02_NES_COLOR (Cast, background_palette, attribute | Storage [Index]); \
		*(solid + Index) = BIT_0; \
	} \
}

#define _2C02_DRAW_ACCURATE_PIXEL(Cast,Storage,Color,Attribute,destination,Solid,Index)\
{ \
	if (!get_flag (CTL_2, BIT_4)) \
	{ \
		(*destination) = _2C02_NES_COLOR (Cast, background_palette, 0); \
		(*Solid) = 0; \
	} \
	if (get_flag (CTL_2, BIT_3)) \
	{ \
		if (Storage [Index]) \
		{ \
		Color = _2C02_NES_COLOR (Cast, background_palette, Attribute | Storage [Index]); \
\
		if (!(*Solid)) (*destination) = Color; \
		else \
		{ \
			if (!((*Solid) & BIT_2) || (((*Solid) & BIT_3))) (*destination) = final_color; \
\
			if (is_primary_backup && !info.po_flag) \
			{ \
				if ((*Solid) & BIT_4) \
				{ \
					info.is_collision_event_pending = TRUE; \
					info.po_flag = TRUE; \
					info.po_collision_cycle = (info.scanline * 341) + (Index - reg.fh) - ((!is_frame_even) ? 1 : 0); \
					info.po_collision_cycle *= (!nes->o_cpu->is_pal () ? 16 : 15); \
				} \
			} \
		} \
		} \
		else if (!(*Solid)) (*destination) = _2C02_NES_COLOR (Cast, background_palette, 0); \
	} \
	else if (!(*Solid)) { \
		(*destination) = _2C02_NES_COLOR (Cast, background_palette, bLastColorSelected); \
	} \
	destination ++; \
	Solid ++; \
	Index ++; \
}

extern __UINT_16 nes_palette [512];
extern __UINT_16 mirroring_modes [][4];

enum e_registers { CTL_1 = 0, CTL_2 = 1, STAT = 2, SPRMEM = 3, SPR = 4, SCROLL = 5, MEM = 6 };	

enum e_event_list { POC = 0 };

struct s_internal_register {
	__UINT_8 fh;
	__UINT_16 contents;
};

//////////////////////////////////////////////////////////////////
// s_rendering_information 
//
// This structure contains everything you need for figuring out
// the PPU's current state.
//////////////////////////////////////////////////////////////////

struct s_rendering_information {

	__BOOL po_flag,
		  sp_limit_flag,
		  is_v_blank,
		  is_h_blank,
		  is_collision_event_pending;

	__UINT_32 x_offset,
            color_emphasis,
			monochrome,
			bg_pattern_base,
			sp_pattern_base,
			scanline;

	__INT_32 po_collision_cycle,
		     sp_limit_reached_cycle;
};

//////////////////////////////////////////////////////////////////
// c_nes_ppu
//
// Lo and behold... The class.
//////////////////////////////////////////////////////////////////

class c_nes_ppu {

	public:

		c_nes_ppu (void);
		~c_nes_ppu (void);

//		void run (void);
		void run_accurate (void);

		s_internal_register & Registers (void) { return reg; }
		s_internal_register & Latches (void) { return lat; }
		s_rendering_information & information (void) { return info; }

		void begin_frame (void)
		{
			state_changed = TRUE;
			info.is_v_blank = FALSE;
			info.scanline = 0;
			oam_inrange_sp = 0;
			oam_inrange_sp_backup = 0;
			is_primary_backup = FALSE;
		}

		void end_frame (void)
		{
			registers [STAT] = 0;
			info.po_flag = info.sp_limit_flag = FALSE;
			info.is_collision_event_pending = FALSE;
			info.po_collision_cycle = info.sp_limit_reached_cycle = 0;
		}

        void update_all_counters (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4)) reg.contents = lat.contents;
		}

		void update_h_counter (void)
 		{
			info.is_h_blank = TRUE;

            if (get_flag (CTL_2, BIT_3 | BIT_4))
            {
				reg.contents = (reg.contents & 0xfbe0) | (lat.contents & 0x041f);
            }
		}

		void update_v_counter (void)
		{
			info.is_h_blank = TRUE;

			if (!get_flag (CTL_2, BIT_3 | BIT_4)) return;

			if ((reg.contents & 0x7000) == 0x7000)
			{
   				reg.contents &= 0x8fff;

				if ((reg.contents & 0x3e0) == 0x3a0)
				{
					reg.contents ^= 0x800;
					reg.contents &= 0xfc1f;
				}
				else if ((reg.contents & 0x3e0) == 0x3e0) reg.contents &= 0xfc1f;
				else reg.contents += 0x20;
			} else reg.contents += 0x1000;
		}

		void set_mirroring (__UINT_8 mode)
		{
			state_changed = TRUE;
			mirroring_mode = mode;

			nametables [0] = &VRAM [mirroring_modes [mode] [0]];
			nametables [1] = &VRAM [mirroring_modes [mode] [1]];
			nametables [2] = &VRAM [mirroring_modes [mode] [2]];
			nametables [3] = &VRAM [mirroring_modes [mode] [3]];
		}

		void set_mirroring (__UINT_32 nametable, __UINT_8 *Area)
		{
			state_changed = TRUE;
			
			nametables [nametable] = Area;
		}

		__UINT_8 * get_vram (__INT_32 address) { return &VRAM [address]; }
		__UINT_8 ** get_nametables () { return nametables; }

		void set_flag (e_registers Register, __UINT_8 bBits) { registers [Register] |= bBits; }
		void clear_flag (e_registers Register, __UINT_8 bBits) { registers [Register] &= ~bBits; }
		__UINT_8 get_flag (e_registers Register, __UINT_8 bBits) { return registers [Register] & bBits; }

		__UINT_8 read_byte (__UINT_16 address);
		void write_byte (__UINT_16 address, __UINT_8 value);

		void write_nt_byte (__UINT_16 address, __UINT_8 value)
		{
			nametables [(address >> 10) & 3] [address & 0x3ff] = value;
		}

		__UINT_8 read_nt_byte (__UINT_16 address)
		{
			return nametables [(address >> 10) & 3] [address & 0x3ff];
		}

		__UINT_8 read_read_buffer (void) { return read_buffer; }
		void set_read_buffer (__UINT_8 value) { read_buffer = value; }

		void swap_page (__UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size);
		void swap_page (__UINT_8 **destination, __UINT_16 dest_where, __UINT_16 page_number, e_page_sizes size);

		void oam_dma (c_mem_block *source, __UINT_16 where_in_source)
		{
			OAM.load_from (source, where_in_source, 0x00, 0x100);
		}

		__UINT_8 read_oam_byte (void) { return OAM.read_byte (oam_address); }
		void write_oam_byte (__UINT_8 value) { OAM.write_byte (oam_address ++, value); }
		void set_oam_address (__UINT_8 value) { oam_address = value; }

		void clear_solid_pixel_lut (void) { memset (solid_pixel_lut, 0, 256); }

		void clear_scanline (void)
		{
			if (nes->o_gfx->get_color_depth () == 8)
				nes->o_gfx->clear (info.scanline, 0, 256, _2C02_NES_COLOR (__UINT_8, background_palette, 0));
			else nes->o_gfx->clear (info.scanline, 0, 256, _2C02_NES_COLOR (__UINT_16, background_palette, 0));
		}

		void SetBgPatternAddress (__UINT_16 address) {	info.bg_pattern_base = address; }

		void FetchObjectData (void);

		void EvaluateOAMEntry (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
				__INT_16 y_difference = info.scanline - OAM [spr_ram_index];

				if (y_difference < 0 || (y_difference > ((!get_flag (CTL_1, BIT_5)) ? 7 : 15)))
				{
				    spr_ram_index += 4;
				    return;
				}

				if (8 == oam_inrange_sp)
				{
					set_flag (STAT, BIT_5);
				}
				else
				{
					oam_inrange_sp ++;

					TMPOAM [spr_tmp_index] = OAM [spr_ram_index + 1]; //Tile
					TMPOAM [spr_tmp_index + 1] = OAM [spr_ram_index + 2]; //X Coordinate
					TMPOAM [spr_tmp_index + 2] = OAM [spr_ram_index + 3]; //Attribute
					TMPOAM [spr_tmp_index + 3] = (__UINT_8) (y_difference); //Tileline #
			
					if (!spr_ram_index) po_present = TRUE;
					spr_ram_index += 4;
					spr_tmp_index += 4;
				}
			}
		}

		void write_chr_ram (__UINT_16 address, __UINT_8 value);
		__UINT_8 read_chr_ram (__UINT_16 address);
		__UINT_8 read_pattern_byte (__UINT_16 address)
		{
			if (is_chr_rom) return pattern_pages [(address >> 10) & 7] [address & 0x3ff];
			else return VRAM.read_byte (address);
		}

		__UINT_8 read_pattern_byte (__UINT_8 **source, __UINT_16 address)
		{
			if (source [(address >> 10) & 3])
				return source [(address >> 10) & 3] [address & 0x3ff];
			else return read_pattern_byte (address);
		}

		__UINT_16 read_pattern_word (__UINT_16 address)
		{
			return *((__UINT_16 *)(pattern_pages [(address >> 10) & 7] + (address & 0x3ff)));
		}

		__UINT_16 read_pattern_word (__UINT_8 **source, __UINT_16 address)
		{
			if (source [(address >> 10) & 3])
				return *((__UINT_16 *)(source [(address >> 10) & 3] + (address & 0x3ff)));
			else return *((__UINT_16 *)(pattern_pages [(address >> 10) & 7] + (address & 0x3ff)));
		}

		void decode_chr_rom (void);

		void save_state (c_tracer &o_writer, e_save_state Type);
		void load_state (c_tracer &o_reader, e_save_state Type);

		//Used by the cycle-accurate engine
		void Setnt_address (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4)) nt_address = 0x2000 | (reg.contents & 0xfff);
		}

		void FetchnametableDataAddress (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
  				nt_set [accurate_tile_index] = read_nt_byte (nt_address);
            }
		}

		void FetchAttributeData (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
				bHT = (reg.contents & 31);
				bVT = (reg.contents & 0x3e0) >> 5;
				at_address = 0x23c0 | (reg.contents & 0x0c00) | ((bVT & 0x1c) << 1) | (bHT >> 2);
				bAttributeShift = (bHT & 0x2) | ((bVT & 0x2) << 1);

				at_set [accurate_tile_index] = ((read_nt_byte (at_address) >> bAttributeShift) & 3) << 2;
			}
		}


		void SetPatternDataAddress (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
				fv = (reg.contents >> 12) & 7;
				PatternAddress = info.bg_pattern_base + (nt_set [accurate_tile_index] << 4) + (fv);
			}
		}

		void FetchLowPatternDataAddress (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
				if (!is_mmc5_vrom)
					low_pattern_set [accurate_tile_index] = read_pattern_byte (PatternAddress);
				else low_pattern_set [accurate_tile_index] = read_pattern_byte (((c_mapper_005 *) (nes->o_mapper))->get_extra_bg (), PatternAddress);
			}
		}

		void FetchHighPatternDataAddress (void)
		{
			if (get_flag (CTL_2, BIT_3 | BIT_4))
			{
				if (!is_mmc5_vrom)
					high_pattern_set [accurate_tile_index] = read_pattern_byte (PatternAddress + 8);
				else high_pattern_set [accurate_tile_index] = read_pattern_byte (((c_mapper_005 *) (nes->o_mapper))->get_extra_bg (), PatternAddress + 8);
			}
		}

		void AcknowledgeEvent (e_event_list Event)
		{
			switch (Event)
			{
				case POC:
					info.is_collision_event_pending = FALSE;
					set_flag (STAT, BIT_6);
					break;
			}
		}

		__BOOL IsEventPending (e_event_list Event)
		{
			switch (Event)
			{
				case POC:
					return info.is_collision_event_pending;
			}
			return FALSE;
		}

        c_mem_block CHRROM;

    private:

//		void render_bg_tileline (void);
		void render_bg_tileline16 (void);

//		void render_bg_tileline_fh (__UINT_16 y_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 x_start, __UINT_8 x_end);
		void render_bg_tileline_fh_16 (__UINT_16 y_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 x_start, __UINT_8 x_end);
//		void render_bg_tileline (__UINT_16 pattern, __UINT_32 attribute);
		void render_bg_tileline16 (__UINT_16 y_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 x_start, __UINT_8 x_end);

//		void render_sp_tileline (__UINT_16 x_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 flip_mask, __BOOL priority);
		void render_sp_tileline_16 (__UINT_16 x_offset, __UINT_16 tileline, __UINT_32 attribute, __UINT_8 flip_mask, __BOOL priority);

//		void render_po_tileline (__UINT_16, __UINT_16 y_offset, __UINT_16, __UINT_32, __BOOL, __BOOL);
		void render_po_tileline_16 (__UINT_16, __UINT_16 y_offset, __UINT_16, __UINT_32, __BOOL, __BOOL);

		__UINT_8 unpacked_pixel_data [33*8];
		s_rendering_information info;
		s_internal_register reg, lat, reg_backup;

		__UINT_8 tmp_oam_index,
			   buf_oam_index;

		__UINT_8 read_buffer,
			   oam_inrange_sp,
			   oam_inrange_sp_backup,
			   po_present,
			   is_primary_backup,
               oam_address,
               solid_pixel_lut [256 + 8];

		__UINT_8 *pattern_pages [8];
		__UINT_16 pattern_pages_backup [8], PatternAddress;
		__BOOL is_chr_rom;

		__UINT_8 bHT, bVT, fv, attribute,
			   bAttributeShift;

		__UINT_8 mirroring_mode;
		__UINT_8 * nametables [4];

		__BOOL bwriteSwitch;
		__UINT_8 registers [8];


		__UINT_16 nt_address, at_address, tileline;
		__UINT_16 uiPatternCache [4];
		__UINT_32 uiCacheIndex;
		__UINT_8 bAttributeCache;
		__UINT_8 bg_pixel_data [8];

		__UINT_8 low_pattern_set [34], high_pattern_set [34], at_set [34];
		__UINT_16 nt_set [34], unpacked_pixel_index;
		__UINT_8 accurate_tile_index, bTile, bLastColorSelected;

		__UINT_8 *pattern_backup [4]; 
		__UINT_8 *background_palette, *sprite_palette;
		__UINT_16 *emulator_palette;

		__UINT_16 spr_tmp_index,
			    spr_ram_index;

		__BOOL is_frame_even, state_changed, is_mmc5_vrom;

		c_mem_block VRAM, OAM, TMPOAM, BUFOAM;

#pragma optimize ("w", off)
};
#pragma optimize ("w", on)

#endif
