/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
	Portable 2A03 CPU Core
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

#ifndef _2A03_H
#define _2A03_H

#include "datatypes.h"

/******************************************************************************/
/** Execution                                                                **/
/******************************************************************************/

extern void _2A03_reset (void);
extern void _2A03_run (void);
extern void _2A03_run (__INT_32 ucycles);

extern void _2A03_request_nmi (void);
extern void _2A03_request_secondary_nmi (void);
extern void _2A03_request_irq (void);
extern void _2A03_set_irq_line (__BOOL);
extern __BOOL _2A03_getIRQLine (void);

extern __UINT_8 _2A03_get_interrupt_flag (void);
extern __BOOL _2A03_labelHolder;
extern __UINT_16 IRQAddr, NMIAddr, resetAddr;

/******************************************************************************/
/** Timing                                                                   **/
/******************************************************************************/

extern void _2A03_kill_cycles (__INT_32);

extern __INT_32 _2A03_getRelativeTime (void);
extern __INT_32 _2A03_get_current_time (void);
extern __INT_32 _2A03_get_end_time (void);

extern void _2A03_set_current_time (__INT_32 iTime);
extern void _2A03_set_end_time (__INT_32 iTime);
extern __BOOL _2A03_has_enough_cycles (void);

/******************************************************************************/
/** Emulator Specifics                                                       **/
/******************************************************************************/

extern void _2A03_toggle_label_holder (void);
extern void _2A03_set_label_holder (__BOOL);
extern void _2A03_toggle_tracer (void);
extern void _2A03_set_tracer (__BOOL);
extern __BOOL _2A03_get_tracer (void);
extern int _2A03_get_instruction(int base_addr,
                                 int address,
                                 int bank_lo,
                                 int bank_hi,
                                 __INT_32 iROMOffset,
                                 char *instruction,
                                 int bank_alias);
extern int _2A03_map_instruction (int base_addr,
                                  int address,
                                  int bank,
                                  __INT_32 iROMOffset,
                                  int bank_alias);

class c_tracer;

extern void _2A03_save_state (c_tracer &TDump);
extern void _2A03_load_state (c_tracer &TDump);

#endif
