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

#include "include/c_label_holder.h"

#include "include/c_control.h"
#include "include/c_cpu.h"
#include "include/c_ppu.h"
#include "include/c_tracer.h"
#include "include/mappers/c_mapper.h"
#include "include/c_nes.h"
#include "include/c_rom.h"
#include "include/datatypes.h"

#define IMM 0
#define ZPA 1
#define ZPX 2
#define ZPY 3
#define AB_ 4
#define ABX 5
#define ABY 6
#define IDR 7
#define PRE 8
#define POS 9
#define IMP 10
#define REL 11
#define JMP 12
#define NIL 13
#define AB_2 14

void _2A03_disassembleInstruction(__INT_32);
void _2A03_disassemblePRGROM(void);
void _2A03_dumpPTTables(__UINT_8);
int write_address(char *operands, int dat);

int base_addr = 0x12345678;
int idx_addr;
int jump_addr;
int last_y = 0x12345678;
int last_x = 0x12345678;
int flipflop_addr;

static const char *_2A03_instructionSet[] =
{
	"brk",	   "ora", "(undef)", "(undef)", "(undef)",     "ora",     "asl", "(undef)",
	"php",     "ora",   "asl a", "(undef)", "(undef)",     "ora",     "asl", "(undef)",
	"bpl",     "ora", "(undef)", "(undef)", "(undef)",     "ora",     "asl", "(undef)",
	"clc",     "ora", "(undef)", "(undef)", "(undef)",     "ora",     "asl", "(undef)",
	"jsr",     "and", "(undef)", "(undef)",     "bit",     "and",     "rol", "(undef)",
	"plp",     "and",   "rol a", "(undef)",     "bit",     "and",     "rol", "(undef)",
    "bmi",     "and", "(undef)", "(undef)", "(undef)",     "and",     "rol", "(undef)",
	"sec",     "and", "(undef)", "(undef)", "(undef)",     "and",     "rol", "(undef)",
	"rti",     "eor", "(undef)", "(undef)", "(undef)",     "eor",     "lsr", "(undef)",
	"pha",     "eor",   "lsr a", "(undef)",     "jmp",     "eor",     "lsr", "(undef)",
	"bvc",     "eor", "(undef)", "(undef)", "(undef)",     "eor",     "lsr", "(undef)",
	"cli",     "eor", "(undef)", "(undef)", "(undef)",     "eor",     "lsr", "(undef)",
    "rts",     "adc", "(undef)", "(undef)", "(undef)",     "adc",     "ror", "(undef)", 
	"pla",     "adc",   "ror a", "(undef)",     "jmp",     "adc",     "ror", "(undef)", 
	"bvs",     "adc", "(undef)", "(undef)", "(undef)",     "adc",     "ror", "(undef)", 
	"sei",     "adc", "(undef)", "(undef)", "(undef)",     "adc",     "ror", "(undef)", 
	"(undef)", "sta", "(undef)", "(undef)",     "sty",     "sta",     "stx", "(undef)",
	"dey", "(undef)",     "txa", "(undef)",     "sty",     "sta",     "stx", "(undef)",
	"bcc",     "sta", "(undef)", "(undef)",     "sty",     "sta",     "stx", "(undef)",
	"tya",     "sta",     "txs", "(undef)", "(undef)",     "sta", "(undef)", "(undef)",
	"ldy",     "lda",     "ldx", "(undef)",     "ldy",     "lda",     "ldx", "(undef)", 
	"tay",     "lda",     "tax", "(undef)",     "ldy",     "lda",     "ldx", "(undef)", 
	"bcs",     "lda", "(undef)", "(undef)",     "ldy",     "lda",     "ldx", "(undef)", 
	"clv",     "lda",     "tsx", "(undef)",     "ldy",     "lda",     "ldx", "(undef)", 
	"cpy",     "cmp", "(undef)", "(undef)",     "cpy",     "cmp",     "dec", "(undef)", 
	"iny",     "cmp",     "dex", "(undef)",     "cpy",     "cmp",     "dec", "(undef)", 
	"bne",     "cmp", "(undef)", "(undef)", "(undef)",     "cmp",     "dec", "(undef)", 
	"cld",     "cmp", "(undef)", "(undef)", "(undef)",     "cmp",     "dec", "(undef)", 
	"cpx",     "sbc", "(undef)", "(undef)",     "cpx",     "sbc",     "inc", "(undef)", 
	"inx",     "sbc",     "nop", "(undef)",     "cpx",     "sbc",     "inc", "(undef)", 
    "beq",     "sbc", "(undef)", "(undef)", "(undef)",     "sbc",     "inc", "(undef)", 
	"sed",     "sbc", "(undef)", "(undef)", "(undef)",     "sbc",     "inc", "(undef)"
};

static __UINT_8 _2A03_instructionAddrMode[] =
{
	IMP, PRE, NIL, NIL, NIL, ZPA, ZPA, NIL,     // 0x00
	IMP, IMM, IMP, NIL, NIL, AB_, AB_, NIL,     // 0x08
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0x10
    IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL,     // 0x18
	JMP, PRE, NIL, NIL, ZPA, ZPA, ZPA, NIL,     // 0x20
	IMP, IMM, IMP, NIL, AB_, AB_, AB_, NIL,     // 0x28
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0x30
	IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL,     // 0x38
	IMP, PRE, NIL, NIL, NIL, ZPA, ZPA, NIL,     // 0x40
	IMP, IMM, IMP, NIL, JMP, AB_, AB_, NIL,     // 0x48
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0x50
	IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL,     // 0x58
	IMP, PRE, NIL, NIL, NIL, ZPA, ZPA, NIL,     // 0x60
	IMP, IMM, IMP, NIL, IDR, AB_, AB_, NIL,     // 0x68
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0x70
	IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL,     // 0x78
	NIL, PRE, NIL, NIL, ZPA, ZPA, ZPA, NIL,     // 0x80
	IMP, NIL, IMP, NIL, AB_, AB_, AB_, NIL,     // 0x88
	REL, POS, NIL, NIL, ZPX, ZPX, ZPY, NIL,     // 0x90
	IMP, ABY, IMP, NIL, NIL, ABX, NIL, NIL,     // 0x98
	IMM, PRE, IMM, NIL, ZPA, ZPA, ZPA, NIL,     // 0xa0
	IMP, IMM, IMP, NIL, AB_, AB_, AB_, NIL,     // 0xa8
	REL, POS, NIL, NIL, ZPX, ZPX, ZPY, NIL,     // 0xb0
	IMP, ABY, IMP, NIL, ABX, ABX, ABY, NIL,     // 0xb8
	IMM, PRE, NIL, NIL, ZPA, ZPA, ZPA, NIL,     // 0xc0
	IMP, IMM, IMP, NIL, AB_, AB_, AB_, NIL,     // 0xc8
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0xd0
	IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL,     // 0xd8
	IMM, PRE, NIL, NIL, ZPA, ZPA, ZPA, NIL,     // 0xe0
	IMP, IMM, IMP, NIL, AB_, AB_, AB_, NIL,     // 0xe8
	REL, POS, NIL, NIL, NIL, ZPX, ZPX, NIL,     // 0xf0
	IMP, ABY, NIL, NIL, NIL, ABX, ABX, NIL      // 0xf8
};

//Cycles Counts - as specified in official Rockwell 6502 docs.

static __UINT_32 *_2A03_cycleCounts;

static __UINT_32 _2A03_PPUCycleCounts[0x100];

static __UINT_32 _2A03_regularCycleCounts[] =
{
	7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
	2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
	2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
	2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
	2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0
};

static __INT_32 *_2A03_accessCycles;

static __INT_32 _2A03_PPUAccessCycles[0x100];

static __INT_32 _2A03_regularAccessCycles[] =
{
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, -4, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0, 
	0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -4, -4, 0,
	0, -6, 0, 0, 0, -4, -4, 0, 0, -5, 0, 0, 0, -5, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -4, -4, 0, 
	0, 0, 0, 0, 0, 0, 0, 0,	0, -5, 0, 0, -5, -5, -5, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0
};

#define _2A03_FIX_CYCLES(Count) ((Count) * 3 * 16)

/******************************************************************************/
/** External Data		                                                     **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Memory Access Functions                                                  **/
/**																			 **/
/** Description:															 **/
/******************************************************************************/

#define NESCTL_ReadByte nes->o_control->read_byte
#define NES_8000_ReadByte nes->o_cpu->read_byte
#define NESPRGRAM_ReadByte nes->o_control->read_byte
#define NESPRGRAM_readWord nes->o_control->read_word
#define NESRAM_ReadByte nes->o_ram->read_byte
#define NESRAM_readWord nes->o_ram->read_word
#define NESCTL_write_byte nes->o_control->write_byte
#define NESRAM_write_byte nes->o_ram->write_byte

/******************************************************************************/
/** Internal Registers                                                       **/
/**																			 **/
/** Description:															 **/
/******************************************************************************/

__BOOL _2A03_instructionDumper, _2A03_labelHolder,
                                _2A03_IRQLine,
                                _2A03_IRQRequested,
                                _2A03_NMIRequested,
                                _2A03_NMISecondRequested,
                                _2A03_unfinishedOp;
static __UINT_8 S, X, Y, tmpByte, SP;
__UINT_16 A, tmpAddress, IRQAddr, NMIAddr, resetAddr;
static AWORD PC, tmpWord;
static __INT_32 tmpInt;
static __INT_32 iCurrentTime, iEndTime;

/******************************************************************************/
/** status Flags                                                             **/
/**																			 **/
/** Description:															 **/
/** - C - Carry Flag;														 **/
/** - Z - Zero Flag;														 **/
/** - I - Interrupt Flag;													 **/
/** - B - BRK Flag;															 **/
/** - V - Overflow Flag;													 **/
/** - N - Sign Flag.														 **/
/******************************************************************************/

#define C BIT_0
#define Z BIT_1
#define I BIT_2
#define D BIT_3
#define FLAG_B BIT_4
#define V BIT_6
#define N BIT_7

#define _2A03_SET_NZ(condition) \
{ \
	if (condition & BIT_7) S |= N; \
	else if (0 == condition) S |= Z ;\
}

/******************************************************************************/
/** Addressing Modes                                                         **/
/**																			 **/
/** Description:															 **/
/******************************************************************************/

__inline __UINT_16 _2A03_immediate(void)
{
	return ++PC.W;
}

__inline __UINT_8 _2A03_zpAbsolute(void)
{
    return NESPRGRAM_ReadByte(++PC.W);
}

__inline __UINT_8 _2A03_zpIndexed(__UINT_8 _2A03_index)
{
	tmpAddress = _2A03_index + _2A03_zpAbsolute();
	return (__UINT_8)(tmpAddress);
}

__inline __UINT_16 _2A03_absolute(void)
{
	PC.W ++;
	register __UINT_16 value = NESPRGRAM_readWord(PC.W);

	if (_2A03_labelHolder)
	{
		if ((int) value > 0x7fff)
		{
		    nes->BankJMPList->insert_label(value, TYPE_DATA, TYPE_BYTE, 0, 0);
	    }
	}

	PC.W ++;
	return value;
}

__inline __UINT_16 _2A03_absolute_write(void)
{
	PC.W ++;
	register __UINT_16 value = NESPRGRAM_readWord(PC.W);
	PC.W ++;
	return value;
}

__inline __UINT_16 _2A03_indexed(__UINT_8 _2A03_index)
{
	PC.W ++;
	register __UINT_16 value = NESPRGRAM_readWord(PC.W);

	if (_2A03_labelHolder)
	{
		if (value > 0x7fff)
		{
		    nes->BankJMPList->insert_label(value, TYPE_DATA, TYPE_BYTE, 0, 0);
	    }
	}

    value += _2A03_index;

	PC.W ++;
	return value;
}

__inline __UINT_16 _2A03_indexed_write(__UINT_8 _2A03_index)
{
	PC.W ++;
	register __UINT_16 value = NESPRGRAM_readWord(PC.W);
    value += _2A03_index;
	PC.W ++;
	return value;
}

__inline __UINT_16 _2A03_indexedCheckBounds(__UINT_8 _2A03_index)
{
	register AWORD address;
	register __UINT_8 bHighByte;

	address.B.L = NESPRGRAM_ReadByte(++PC.W);
	address.B.H = bHighByte = NESPRGRAM_ReadByte(++PC.W);

	if(_2A03_labelHolder)
	{
		if(address.W > 0x7fff)
		{
		    nes->BankJMPList->insert_label(address.W, TYPE_DATA, TYPE_BYTE, 0, 0);
	    }
	}

	address.W += _2A03_index;

	if(address.B.H != bHighByte) iCurrentTime += _2A03_FIX_CYCLES(1);
	
	return address.W;
}

__inline __UINT_16 _2A03_indirect(void)
{
	tmpAddress = _2A03_absolute();
	tmpByte = NESCTL_ReadByte(tmpAddress);
	tmpAddress = (tmpAddress & 0xff00) | ((tmpAddress + 1) & 0xff);
    register __UINT_16 value = (tmpByte | (NESCTL_ReadByte(tmpAddress) << 8));
    
	if (_2A03_labelHolder)
	{
		if ((int) value > 0x7fff)
		{
		    nes->BankJMPList->insert_label(value, TYPE_DATA, TYPE_BYTE, 0, 0);
	    }
	}
   
    return value;
}

__inline __UINT_16 _2A03_preIndexed (void)
{
	tmpByte = _2A03_zpIndexed (X);
	return NESRAM_ReadByte(tmpByte) | (NESRAM_ReadByte((__UINT_8) (tmpByte + 1)) << 8);
}

__inline __UINT_16 _2A03_postIndexed (void)
{
	tmpByte = _2A03_zpAbsolute();
	tmpWord.W = NESCTL_ReadByte(tmpByte) | (NESCTL_ReadByte((__UINT_8) (tmpByte + 1)) << 8);
	tmpWord.W += Y;
	return tmpWord.W;
}

__inline __UINT_16 _2A03_postIndexedCheckBounds(void)
{
	tmpByte = _2A03_zpAbsolute();
	tmpWord.W = NESCTL_ReadByte(tmpByte) | (NESCTL_ReadByte( (__UINT_8)(tmpByte + 1)) << 8);
	tmpAddress = tmpWord.W + Y;
	if ((tmpAddress >> 8) != tmpWord.B.H) iCurrentTime += _2A03_FIX_CYCLES (1);//iCurrentTime ++; 
	return tmpAddress;
}

#define _2A03_BRANCH(condition) \
{ \
	if (condition) \
	{ \
		tmpWord.W = PC.W + 2; \
		tmpByte = NESPRGRAM_ReadByte(_2A03_immediate()); \
		tmpInt = PC.W + (INT_8)(tmpByte) + 1; \
		iCurrentTime += ((tmpInt >> 8) != (tmpWord.B.H)) ? (_2A03_FIX_CYCLES(2)) : (_2A03_FIX_CYCLES(1)); \
	    if (_2A03_labelHolder) \
	    { \
		    nes->BankJMPList->insert_label(tmpInt, TYPE_CODE, TYPE_RELCODE, 0, 0); \
	    } \
		PC.W = (__UINT_16)(tmpInt); \
	} \
	else \
	{ \
	    if (_2A03_labelHolder) \
	    { \
		    tmpByte = NESPRGRAM_ReadByte(PC.W + 1); \
		    tmpInt = (PC.W + 1) + (INT_8)(tmpByte) + 1; \
		    nes->BankJMPList->insert_label(tmpInt, TYPE_CODE, TYPE_RELCODE, 0, 0); \
	    } \
	    PC.W += 2; \
    } \
    last_x = 0x12345678; \
    last_y = 0x12345678; \
}

/******************************************************************************/
/** Stack Operations                                                         **/
/**																			 **/
/** Description:															 **/
/******************************************************************************/

#define _2A03_PUSH(value) \
{ \
	NESRAM_write_byte(0x100 + SP, value); \
	SP --; \
}

#define _2A03_POP(dest) \
{ \
	SP++; \
	dest = NESRAM_ReadByte(0x100 + SP); \
}

/******************************************************************************/
/** Processing Operations                                                    **/
/**																			 **/
/** Description:															 **/
/******************************************************************************/

#define _2A03_ADC(_2A03_ReadByte, _2A03_mode) \
{ \
	tmpByte = _2A03_ReadByte(_2A03_mode); \
    tmpWord.W = A + tmpByte + (S & C); \
	S &= ~(N | V | Z | C); \
    if(~(A ^ tmpByte) & (A ^ tmpWord.B.L) & 0x80) S |= V; \
	S |= tmpWord.B.H & 1; \
	A = tmpWord.B.L; \
	_2A03_SET_NZ(tmpWord.B.L); \
}

#define _2A03_AND(_2A03_ReadByte, _2A03_mode) \
{ \
	S &= ~(N | Z); \
	A &= _2A03_ReadByte(_2A03_mode); \
	_2A03_SET_NZ(A); \
}

#define _2A03_ASLA() \
{ \
	S &= ~(N | Z | C); \
	if(A & BIT_7) S |= C;	\
	A = (__UINT_8)(A << 1); \
	_2A03_SET_NZ(A); \
}

#define _2A03_ASL(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	S &= ~(N | Z | C); \
	tmpAddress = _2A03_mode; \
	tmpByte = _2A03_ReadByte (tmpAddress); \
	if(tmpByte & BIT_7) S |= C; \
	tmpByte <<= 1; \
	_2A03_write_byte(tmpAddress, tmpByte); \
    _2A03_SET_NZ(tmpByte); \
}

#define _2A03_BIT(_2A03_ReadByte, _2A03_mode) \
{ \
	S &= ~(N | Z | V); \
	tmpByte = _2A03_ReadByte(_2A03_mode); \
	if(tmpByte & BIT_7) S |= N; \
	if(tmpByte & BIT_6) S |= V; \
	if(!(A & tmpByte)) S |= Z; \
}

#define _2A03_COMPARE(_2A03_reg, _2A03_ReadByte, _2A03_mode) \
{ \
	S &= ~(N | Z | C); \
	tmpByte = _2A03_ReadByte (_2A03_mode); \
	if(_2A03_reg >= tmpByte) S |= C; \
	tmpByte = _2A03_reg - tmpByte; \
	_2A03_SET_NZ(tmpByte); \
}

#define _2A03_DEC(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	S &= ~(N | Z); \
	tmpAddress = _2A03_mode; \
	tmpByte = (__UINT_8) (_2A03_ReadByte(tmpAddress) - 1); \
	_2A03_write_byte(tmpAddress, tmpByte); \
	_2A03_SET_NZ(tmpByte); \
}

#define _2A03_EOR(_2A03_ReadByte, _2A03_mode) \
{ \
	S &= ~(N | Z); \
	A ^= _2A03_ReadByte(_2A03_mode); \
	_2A03_SET_NZ(A); \
}

#define _2A03_INC(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	S &= ~(N | Z); \
	tmpAddress = _2A03_mode; \
	tmpByte = _2A03_ReadByte(tmpAddress) + 1; \
	_2A03_write_byte(tmpAddress, tmpByte); \
	_2A03_SET_NZ(tmpByte); \
}

#define _2A03_LOAD(_2A03_reg, _2A03_ReadByte, _2A03_mode) \
{ \
	S &= ~(N | Z); \
	_2A03_reg = _2A03_ReadByte(_2A03_mode); \
	_2A03_SET_NZ(_2A03_reg); \
}

#define _2A03_LSRA() \
{ \
	S &= ~(N | Z | C); \
	if (A & BIT_0) S |= C;	\
	A >>= 1; \
	if (0 == A) S |= Z; \
}

#define _2A03_LSR(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	S &= ~(N | Z | C); \
	tmpAddress = _2A03_mode; \
	tmpByte = _2A03_ReadByte(tmpAddress); \
	if(tmpByte & BIT_0) S |= C; \
	tmpByte >>= 1; \
	_2A03_write_byte(tmpAddress, tmpByte); \
	if(0 == tmpByte) S |= Z; \
}

#define _2A03_ORA(_2A03_ReadByte,_2A03_mode) \
{ \
	S &= ~(N | Z); \
	A |= _2A03_ReadByte(_2A03_mode); \
	_2A03_SET_NZ(A); \
}

#define _2A03_ROLA() \
{ \
	tmpWord.W = A << 1; \
	tmpWord.W |= (S & C); \
	S &= ~(N | Z | C); \
	if(tmpWord.B.H) S |= C; \
	A = tmpWord.B.L; \
	_2A03_SET_NZ(tmpWord.B.L); \
}

#define _2A03_ROL(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	tmpAddress = _2A03_mode; \
	tmpWord.W = _2A03_ReadByte(tmpAddress) << 1; \
	tmpWord.W |= (S & C); \
	_2A03_write_byte(tmpAddress, tmpWord.B.L); \
	S &= ~(N | Z | C); \
	if(tmpWord.B.H) S |= C; \
	_2A03_SET_NZ(tmpWord.B.L); \
}

#define _2A03_RORA() \
{ \
	if(S & C) A |= 0x100; \
	S &= ~(N | Z | C); \
	if(A & BIT_0) S |= C; \
	A >>= 1; \
	_2A03_SET_NZ(A); \
}

#define _2A03_ROR(_2A03_ReadByte, _2A03_write_byte, _2A03_mode) \
{ \
	tmpAddress = _2A03_mode; \
	tmpWord.W = _2A03_ReadByte(tmpAddress); \
	if(S & C) tmpWord.B.H |= 1; \
	S &= ~(N | Z | C); \
	if(tmpWord.W & BIT_0) S |= C; \
	tmpWord.W >>= 1; \
	_2A03_SET_NZ(tmpWord.B.L); \
	_2A03_write_byte(tmpAddress, tmpWord.B.L); \
}

#define _2A03_SBC(_2A03_ReadByte, _2A03_mode) \
{ \
	tmpByte = _2A03_ReadByte(_2A03_mode); \
    tmpWord.W = A - tmpByte - ((S & C) ? 0 : 1); \
	S &= ~(N | V | Z | C); \
    if ((A ^ tmpByte) & (A ^ tmpWord.B.L) & 0x80) S |= V; \
	S |= (tmpWord.B.H + 1) & 1; \
	A = tmpWord.B.L; \
	_2A03_SET_NZ(A); \
}

#define _2A03_STORE(_2A03_reg, _2A03_write_byte, _2A03_mode) \
{ \
	_2A03_write_byte(_2A03_mode, (__UINT_8)(_2A03_reg)); \
}

/******************************************************************************/
/** Execution                                                                **/
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// reset                                                                      //
////////////////////////////////////////////////////////////////////////////////

void _2A03_reset(void)
{
	A = X = Y = S = 0x0000; SP = 0xfd;
	iCurrentTime = iEndTime = 0;
	S |= (FLAG_B | BIT_5);

	iCurrentTime += _2A03_FIX_CYCLES (7);

	_2A03_IRQLine = _2A03_NMIRequested = _2A03_IRQRequested = FALSE;
	_2A03_unfinishedOp = FALSE;

	NMIAddr = NESPRGRAM_readWord (0xfffa);
	if(_2A03_instructionDumper) nes->general_log.f_write("sws", "NMI Vector: ", NMIAddr, "\r\n");
	resetAddr = NESPRGRAM_readWord (0xfffc);
	if(_2A03_instructionDumper) nes->general_log.f_write("sws", "Reset Vector: ", resetAddr, "\r\n");
	IRQAddr = NESPRGRAM_readWord (0xfffe);
	if(_2A03_instructionDumper) nes->general_log.f_write("sws", "IRQ Vector: ", IRQAddr, "\r\n");
 
	PC.W = resetAddr;

	for(__UINT_32 iIndex = 0; iIndex < 0x100; iIndex ++)
	{
		_2A03_PPUCycleCounts[iIndex] = _2A03_FIX_CYCLES(_2A03_regularCycleCounts[iIndex]);
		_2A03_PPUAccessCycles[iIndex] = _2A03_FIX_CYCLES(_2A03_regularAccessCycles[iIndex]);
	}

	_2A03_cycleCounts = _2A03_PPUCycleCounts;
	_2A03_accessCycles = _2A03_PPUAccessCycles;
}

////////////////////////////////////////////////////////////////////////////////
// NMI Handler                                                                //
////////////////////////////////////////////////////////////////////////////////

//__inline
void _2A03_request_nmi(void)
{
    _2A03_NMIRequested = TRUE;
}

void _2A03_request_secondary_nmi (void)
{
    _2A03_NMISecondRequested = TRUE;
}

void _2A03_request_irq(void)
{
    _2A03_IRQRequested = TRUE;
}

void _2A03_NMI (void)
{
    _2A03_PUSH(PC.B.H);
	_2A03_PUSH(PC.B.L);
	_2A03_PUSH(S & ~FLAG_B);
	PC.W = NESPRGRAM_readWord(0xfffa);
	iCurrentTime += _2A03_FIX_CYCLES(7);
}

////////////////////////////////////////////////////////////////////////////////
// IRQ Handler                                                                //
////////////////////////////////////////////////////////////////////////////////

//__inline 
void _2A03_set_irq_line(__BOOL status)
{
	_2A03_IRQLine = status;
}

__BOOL _2A03_getIRQLine(void)
{
    return _2A03_IRQLine;
}

void _2A03_IRQ(void)
{
	if(!(S & I))
	{
	    S &= ~FLAG_B;
		_2A03_PUSH(PC.B.H);
		_2A03_PUSH(PC.B.L);
		_2A03_PUSH(S);
		S |= I;
		PC.W = NESPRGRAM_readWord(0xfffe);
		iCurrentTime += _2A03_FIX_CYCLES(7);
	}
}

__UINT_8 _2A03_get_interrupt_flag(void)
{
    return S & I;
}

////////////////////////////////////////////////////////////////////////////////
// run                                                                        //
////////////////////////////////////////////////////////////////////////////////

void _2A03_run(__INT_32 cycles)
{
	_2A03_set_end_time(cycles);
	_2A03_run();
}

void _2A03_run(void)
{
    int old_pc;
    int zp_addr;
    register __UINT_16 value;
	if(iCurrentTime == iEndTime) return;

	if(_2A03_instructionDumper && (iCurrentTime > (iEndTime + 21)))
	{
	    nes->general_log.f_write("s", "---------------------------------- MURDERING DMA CYCLES ------------------------------------\r\n");
    }

	while(iCurrentTime < iEndTime)
	{
		if(!_2A03_unfinishedOp)
		{
			if(_2A03_NMIRequested) { _2A03_NMI(); _2A03_NMIRequested = FALSE; continue; }
			if(_2A03_NMISecondRequested) { _2A03_NMIRequested = TRUE; _2A03_NMISecondRequested = FALSE; }
			if(_2A03_IRQRequested || (_2A03_IRQLine && !(S & I))) { _2A03_IRQ(); _2A03_IRQRequested = FALSE; continue; }
		}

		register __UINT_8 _2A03_instruction = NESCTL_ReadByte (PC.W);
		if(_2A03_instructionDumper && !_2A03_unfinishedOp)
		{
//		    _2A03_disassembleInstruction (PC.W);
        }
		
		if(!_2A03_unfinishedOp) iCurrentTime += _2A03_cycleCounts [_2A03_instruction];

		if((iCurrentTime > iEndTime) && _2A03_accessCycles[_2A03_instruction])
		{
			_2A03_unfinishedOp = TRUE;
			return;
		} else _2A03_unfinishedOp = FALSE;
	
		switch (_2A03_instruction)
		{
			//LDA - load accumulator with memory//
			case 0xa9:
			    _2A03_LOAD(A, NESPRGRAM_ReadByte, _2A03_immediate());
			    break;
			case 0xa5:
			    _2A03_LOAD(A, NESRAM_ReadByte, _2A03_zpAbsolute());
			    break;
			case 0xb5:
			    _2A03_LOAD(A, NESRAM_ReadByte, _2A03_zpIndexed(X));
			    break;
			case 0xad:
			    _2A03_LOAD(A, NESCTL_ReadByte, _2A03_absolute());
			    break;
			case 0xbd:
                // x, x / x + 1, x
                if((base_addr + 1) == (int) NESPRGRAM_readWord(PC.W + 1))
                {
                    // Next part of the address
                    jump_addr = NESPRGRAM_readWord(idx_addr);
                    last_x = 0x12345678;
/*                    if((int) jump_addr > 0x7fff && (int) base_addr > 0x7fff)
                    {
                        nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(base_addr),
                                                            base_addr,
                                                            TYPE_DATA,
                                                            TYPE_RAWWORD,
                                                            1,
                                                            base_addr);
                        nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(jump_addr),
                                                            jump_addr,
                                                            TYPE_DATA,
                                                            TYPE_BYTE,
                                                            1,
                                                            jump_addr);
	                }*/
	            }
	            else
	            {
                    // Variant x, x / x, x + 1
                    if(base_addr == (int) NESPRGRAM_readWord(PC.W + 1) &&
                       ((idx_addr + 1) == base_addr + X) &&
                       (X == last_x + 1))
                    {
                        // Next part of the address
                        jump_addr = NESPRGRAM_readWord(idx_addr);
                        last_x = 0x12345678;
/*                        if((int) jump_addr > 0x7fff && (int) base_addr > 0x7fff)
                        {
                            nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(base_addr),
                                                                base_addr,
                                                                TYPE_DATA,
                                                                TYPE_RAWWORD,
                                                                1,
                                                                base_addr);
                            nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(jump_addr),
                                                                jump_addr,
                                                                TYPE_DATA,
                                                                TYPE_BYTE,
                                                                1,
                                                                jump_addr);
                        }*/
                    }
                    else
                    {
	                    base_addr = NESPRGRAM_readWord(PC.W + 1);
			            idx_addr = base_addr + X;
			            last_x = X;
                    }
                }
			    _2A03_LOAD (A, NESCTL_ReadByte, _2A03_indexedCheckBounds(X));
			    break;
			case 0xb9:
                // x, y / x + 1, y
                if((base_addr + 1) == (int) NESPRGRAM_readWord(PC.W + 1))
                {
                    // Next part of the address
                    jump_addr = NESPRGRAM_readWord(idx_addr);
                    last_y = 0x12345678;
/*                    if((int) jump_addr > 0x7fff && (int) base_addr > 0x7fff)
                    {
                        nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(base_addr),
                                                            base_addr,
                                                            TYPE_DATA,
                                                            TYPE_RAWWORD,
                                                            1,
                                                            base_addr);
                        nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(jump_addr),
                                                            jump_addr,
                                                            TYPE_DATA,
                                                            TYPE_BYTE,
                                                            1,
                                                            jump_addr);
	                }*/
	            }
	            else
	            {
                    // Variant x, y / x, y + 1
                    if(base_addr == (int) NESPRGRAM_readWord(PC.W + 1) &&
                       ((idx_addr + 1) == base_addr + Y) &&
                       (Y == last_y + 1))
                    {
                        // Next part of the address
                        jump_addr = NESPRGRAM_readWord(idx_addr);
                        last_y = 0x12345678;
/*                        if((int) jump_addr > 0x7fff && (int) base_addr > 0x7fff)
                        {
                            nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(base_addr),
                                                                base_addr,
                                                                TYPE_DATA,
                                                                TYPE_RAWWORD,
                                                                1,
                                                                base_addr);
                            nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(jump_addr),
                                                                jump_addr,
                                                                TYPE_DATA,
                                                                TYPE_BYTE,
                                                                1,
                                                                jump_addr);
                        }*/
                    }
                    else
                    {
	                    base_addr = NESPRGRAM_readWord(PC.W + 1);
			            idx_addr = base_addr + Y;
			            last_y = Y;
                    }
                }
			    _2A03_LOAD (A, NESCTL_ReadByte, _2A03_indexedCheckBounds(Y));
			    break;
			case 0xa1:
			    _2A03_LOAD (A, NESCTL_ReadByte, _2A03_preIndexed());
			    break;
			case 0xb1:
			    // ZP post indexed
                // x, y / x + 1 ,y
                zp_addr = NESPRGRAM_ReadByte(PC.W + 1);
                zp_addr = NESPRGRAM_readWord(zp_addr);
                if((base_addr + 1) == (int) zp_addr)
                {
                    // Next part of the address
                    jump_addr = NESPRGRAM_readWord(idx_addr);
	            }
	            else
	            {
                    // Variant x, y / x, y + 1
                    if(base_addr == (int) zp_addr &&
                       ((idx_addr + 1) == base_addr + Y) &&
                       (Y == last_y + 1))
                    {
                        // Next part of the address
                        jump_addr = NESPRGRAM_readWord(idx_addr);
                        // Some games use (konami's only ?)
                        // stack return address to store the pointers table address
                        if((idx_addr - base_addr) & 1) base_addr++;
                        last_y = 0x12345678;
                    }
                    else
                    {
	                    base_addr = zp_addr;
			            idx_addr = base_addr + Y;
			            last_y = Y;
                    }
                }
			    _2A03_LOAD(A, NESCTL_ReadByte, _2A03_postIndexedCheckBounds());
			    break;

			//STA - Store accumulator in memory//
			case 0x85:
			    _2A03_STORE(A, NESRAM_write_byte, _2A03_zpAbsolute());
			    break;
			case 0x95:
			    _2A03_STORE(A, NESRAM_write_byte, _2A03_zpIndexed(X));
			    break;
			case 0x8d:
			    _2A03_STORE(A, NESCTL_write_byte, _2A03_absolute_write());
			    break;
			case 0x9d:
			    _2A03_STORE(A, NESCTL_write_byte, _2A03_indexed_write(X));
			    break;
			case 0x99:
			    _2A03_STORE(A, NESCTL_write_byte, _2A03_indexed_write(Y));
			    break;
			case 0x81:
			    _2A03_STORE(A, NESCTL_write_byte, _2A03_preIndexed());
			    break;
			case 0x91:
			    _2A03_STORE(A, NESCTL_write_byte, _2A03_postIndexed());
			    break;

			//ADC - Add memory to accumulator with carry//
			case 0x69: _2A03_ADC(NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0x65: _2A03_ADC(NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0x75: _2A03_ADC(NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0x6d: _2A03_ADC(NESCTL_ReadByte, _2A03_absolute()); break;
			case 0x7d: _2A03_ADC(NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0x79: _2A03_ADC(NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0x61: _2A03_ADC(NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0x71: _2A03_ADC(NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break;

			//AND - AND memory with accumulator//
			case 0x29: _2A03_AND(NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0x25: _2A03_AND(NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0x35: _2A03_AND(NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0x2d: _2A03_AND(NESCTL_ReadByte, _2A03_absolute()); break;
			case 0x3d: _2A03_AND(NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0x39: _2A03_AND(NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0x21: _2A03_AND(NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0x31: _2A03_AND(NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break; 

			//ASL - Shift left one bit (memory or accumulator)//
			case 0x0a: _2A03_ASLA(); break;
			case 0x06: _2A03_ASL(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x16: _2A03_ASL(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0x0e: _2A03_ASL(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0x1e: _2A03_ASL(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;
			
			//BCC - Branch on carry clear//
			case 0x90: _2A03_BRANCH(!(S & C)); continue;

			//BCS - Branch on carry set//
			case 0xb0: _2A03_BRANCH(S & C); continue;

			//BEQ - Branch on result zero//
			case 0xf0: _2A03_BRANCH(S & Z); continue;

			//BIT - Test bit in memory with accumulator//
			case 0x24: _2A03_BIT(NESRAM_ReadByte, _2A03_zpAbsolute ()); break;
			case 0x2c: _2A03_BIT(NESCTL_ReadByte, _2A03_absolute ()); break;

			//BMI - Branch on result minus//
			case 0x30: _2A03_BRANCH(S & N); continue;

			//BNE - Branch on result not zero//
			case 0xd0: _2A03_BRANCH(!(S & Z)); continue;

			//BPL - Branch on result plus//
			case 0x10: _2A03_BRANCH(!(S & N)); continue;

			//BVC - Branch on overflow clear//
			case 0x50: _2A03_BRANCH(!(S & V)); continue;

			//BVS - Branch on overflow set//
			case 0x70: _2A03_BRANCH(S & V); continue;

			//CLC - Clear carry flag//
			case 0x18: S &= ~C; break;
		
			//CLD - Clear decimal flag//
			//Note: There is no such thing as a decimal mode on the NES.
			case 0xd8: S &= ~D; break; 

			//CLI - Clear interrupt disable bit//
			case 0x58: if(S & I) { S &= ~I; PC.W++; return; } break;

			//CLV - Clear overflow flag//
			case 0xb8: S &= ~V; break;

			//CMP - Compare memory and accumulator//
			case 0xc9: _2A03_COMPARE(A, NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0xc5: _2A03_COMPARE(A, NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0xd5: _2A03_COMPARE(A, NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0xcd: _2A03_COMPARE(A, NESCTL_ReadByte, _2A03_absolute()); break;
			case 0xdd: _2A03_COMPARE(A, NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0xd9: _2A03_COMPARE(A, NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0xc1: _2A03_COMPARE(A, NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0xd1: _2A03_COMPARE(A, NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break; 

			//CPX - Compare memory and index X//
			case 0xe0: _2A03_COMPARE(X, NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0xe4: _2A03_COMPARE(X, NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0xec: _2A03_COMPARE(X, NESCTL_ReadByte, _2A03_absolute()); break;

			//CPY - Compare memory and index Y//
			case 0xc0: _2A03_COMPARE(Y, NESPRGRAM_ReadByte, _2A03_immediate ()); break;
			case 0xc4: _2A03_COMPARE(Y, NESRAM_ReadByte, _2A03_zpAbsolute ()); break;
			case 0xcc: _2A03_COMPARE(Y, NESCTL_ReadByte, _2A03_absolute ()); break;

			//DEC - Decrement memory by one//
			case 0xc6: _2A03_DEC(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0xd6: _2A03_DEC(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0xce: _2A03_DEC(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0xde: _2A03_DEC(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;

			//DEX - Decrement index X by one//
			case 0xca: S &= ~(N | Z); X--; _2A03_SET_NZ(X); break;

			//DEY - Decrement index Y by one//
			case 0x88: S &= ~(N | Z); Y--; _2A03_SET_NZ(Y); break;

			//EOR - Exclusive OR memory with accumulator//
			case 0x49: _2A03_EOR(NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0x45: _2A03_EOR(NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0x55: _2A03_EOR(NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0x4d: _2A03_EOR(NESCTL_ReadByte, _2A03_absolute()); break;
			case 0x5d: _2A03_EOR(NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0x59: _2A03_EOR(NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0x41: _2A03_EOR(NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0x51: _2A03_EOR(NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break;	

			//INC - Increment memory by one//
			case 0xe6: _2A03_INC(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0xf6: _2A03_INC(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0xee: _2A03_INC(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0xfe: _2A03_INC(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;

			//INX - Increment index X by one//
			case 0xe8: S &= ~(N | Z); X++; _2A03_SET_NZ(X); break;

			//INY - Increment index Y by one//
			case 0xc8: S &= ~(N | Z); Y++; _2A03_SET_NZ(Y); break;

			//JMP abs - Jump to new location//
			case 0x4c: 
		        old_pc = PC.W + 3;
			    PC.W = _2A03_absolute();
				if(_2A03_labelHolder)
				{
					if(PC.W > 0x7fff)
					{
						nes->BankJMPList->insert_label(PC.W, TYPE_CODE, TYPE_CODE, 0, 0);
                    }
                    // Terminate it
                    nes->BankJMPList->insert_label(old_pc, TYPE_DATA, TYPE_BYTE, 0, 0);
                }
                last_x = 0x12345678;
                last_y = 0x12345678;
				continue;

		    // JMP (ind)
			case 0x6c:
		        old_pc = PC.W + 3;
	            value = NESPRGRAM_readWord(PC.W + 1);
				if(_2A03_labelHolder)
				{
                    if(NESPRGRAM_readWord(value) == jump_addr)
                    {
                        // Turn it into a .word
                        while((idx_addr >= base_addr))
                        {
                            // Fill the previous entries of the table
                            nes->BankJMPList->insert_label_bank(nes->o_mapper->get_bank_number(NESPRGRAM_readWord(value)),
                                                                idx_addr, TYPE_DATA, TYPE_WORD, 1, base_addr);
                            nes->BankJMPList->insert_label(NESPRGRAM_readWord(idx_addr), TYPE_CODE, TYPE_CODE, 1, 0);
                            idx_addr -= 2;
                        }
                        idx_addr = 0x12345678;
                        base_addr = 0x12345678;
                    }
                }
		        // This would be a pointer (or a pointer table)
				if(_2A03_labelHolder)
				{
                    if((int) value > 0x7fff)
                    {
                        nes->BankJMPList->insert_label(value, TYPE_DATA, TYPE_WORD, 0, 0);
                    }
                }

			    PC.W = _2A03_indirect();
				if(_2A03_labelHolder)
				{
					// Dest address
                    if(PC.W > 0x7fff)
                    {
                        nes->BankJMPList->insert_label(PC.W, TYPE_CODE, TYPE_CODE, 0, 0);
                    }
                    // Next byte is undefined
                    // Note: if a table word pointers is right after
                    // the the jmp the label is already added above
                    // (no overwriting occurs).
                    nes->BankJMPList->insert_label(old_pc, TYPE_DATA, TYPE_BYTE, 0, 0);
                }
                last_x = 0x12345678;
                last_y = 0x12345678;
				continue;

			//JSR - Jump to new location saving continue address//
			case 0x20:
			    _2A03_PUSH((PC.W + 2) >> 8);
			    _2A03_PUSH((__UINT_8)(PC.W) + 2);
			    PC.W = _2A03_absolute();
				if(_2A03_labelHolder)
				{
					if(PC.W > 0x7fff)
					{
						nes->BankJMPList->insert_label(PC.W, TYPE_CODE, TYPE_CODE, 0, 0);
                    }
                }
                last_x = 0x12345678;
                last_y = 0x12345678;
				continue;

			//LDX - load index X with memory//
			case 0xa2: _2A03_LOAD(X, NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0xa6: _2A03_LOAD(X, NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0xb6: _2A03_LOAD(X, NESRAM_ReadByte, _2A03_zpIndexed(Y)); break;
			case 0xae: _2A03_LOAD(X, NESCTL_ReadByte, _2A03_absolute()); break;
			case 0xbe: _2A03_LOAD(X, NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;

			//LDY - load index Y with memory//
			case 0xa0: _2A03_LOAD(Y, NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0xa4: _2A03_LOAD(Y, NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0xb4: _2A03_LOAD(Y, NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0xac: _2A03_LOAD(Y, NESCTL_ReadByte, _2A03_absolute()); break;
			case 0xbc: _2A03_LOAD(Y, NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;

			//LSR - Shift right one bit (memory or accumulator)//
			case 0x4a: _2A03_LSRA(); break;
			case 0x46: _2A03_LSR(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x56: _2A03_LSR(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0x4e: _2A03_LSR(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0x5e: _2A03_LSR(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;

			//NOP - No operation//
			case 0xea: break;
			
			//ORA - OR memory with accumulator//
			case 0x09: _2A03_ORA(NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0x05: _2A03_ORA(NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0x15: _2A03_ORA(NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0x0d: _2A03_ORA(NESCTL_ReadByte, _2A03_absolute()); break;
			case 0x1d: _2A03_ORA(NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0x19: _2A03_ORA(NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0x01: _2A03_ORA(NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0x11: _2A03_ORA(NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break;

			//PHA - Push accumulator on stack//
			case 0x48: _2A03_PUSH((__UINT_8)(A)); break;

			//PHP - Push processor status on stack//
			case 0x08: S |= FLAG_B; _2A03_PUSH (S); break;

			//PLA - Pull accumulator from stack//
			case 0x68: S &= ~(N | Z); _2A03_POP(A); if(A & BIT_7) S |= N; else if(A == 0) S |= Z; break;

			//PLP - Pull processor status from stack//
			case 0x28: _2A03_POP(S); S &= ~FLAG_B; S |= BIT_5; break;

			//ROL - Rotate one bit left//
			case 0x2a: _2A03_ROLA(); break;
			case 0x26: _2A03_ROL(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x36: _2A03_ROL(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0x2e: _2A03_ROL(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0x3e: _2A03_ROL(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;

			//ROR - Rotate one bit right//
			case 0x6a: _2A03_RORA(); break;
			case 0x66: _2A03_ROR(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x76: _2A03_ROR(NESRAM_ReadByte, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0x6e: _2A03_ROR(NESCTL_ReadByte, NESCTL_write_byte, _2A03_absolute()); break;
			case 0x7e: _2A03_ROR(NESCTL_ReadByte, NESCTL_write_byte, _2A03_indexed(X)); break;

			//RTI - Return from interrupt//
			case 0x40:
                last_x = 0x12345678;
                last_y = 0x12345678;
				if(_2A03_labelHolder)
				{
					nes->BankJMPList->insert_label(PC.W + 1, TYPE_DATA, TYPE_BYTE, 0, 0);
                }
			    _2A03_POP(S);
			    S |= BIT_5;
			    _2A03_POP(PC.B.L);
			    _2A03_POP(PC.B.H);
			    continue;
			
			//RTS - Return from subroutine//
			case 0x60:
                last_x = 0x12345678;
                last_y = 0x12345678;
 
 				if(_2A03_labelHolder)
				{
					nes->BankJMPList->insert_label(PC.W + 1, TYPE_DATA, TYPE_BYTE, 0, 0);
                }
			    _2A03_POP(PC.B.L);
			    _2A03_POP(PC.B.H);
			    PC.W++; 
				continue;

			//SBC - Subtract memory from accumulator with borrow//
			case 0xe9: _2A03_SBC(NESPRGRAM_ReadByte, _2A03_immediate()); break;
			case 0xe5: _2A03_SBC(NESRAM_ReadByte, _2A03_zpAbsolute()); break;
			case 0xf5: _2A03_SBC(NESRAM_ReadByte, _2A03_zpIndexed(X)); break;
			case 0xed: _2A03_SBC(NESCTL_ReadByte, _2A03_absolute()); break;
			case 0xfd: _2A03_SBC(NESCTL_ReadByte, _2A03_indexedCheckBounds(X)); break;
			case 0xf9: _2A03_SBC(NESCTL_ReadByte, _2A03_indexedCheckBounds(Y)); break;
			case 0xe1: _2A03_SBC(NESCTL_ReadByte, _2A03_preIndexed()); break;
			case 0xf1: _2A03_SBC(NESCTL_ReadByte, _2A03_postIndexedCheckBounds()); break;

 			//SEC - Set carry flag//
			case 0x38: S |= C; break;

			//SED - Set decimal flag//
			case 0xf8: 
				S |= D;
				if(_2A03_instructionDumper) nes->general_log.f_write("s", "2A03: WARNING - This processor does not support decimal mode operations.\r\n");
				break;

			//SEI - Set interrupt disable status//
			case 0x78: S |= I; break;
			
			//STX - Store index X in memory//
			case 0x86: _2A03_STORE(X, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x96: _2A03_STORE(X, NESRAM_write_byte, _2A03_zpIndexed(Y)); break;
			case 0x8e: _2A03_STORE(X, NESCTL_write_byte, _2A03_absolute()); break;

			//STY - Store index Y in memory//
			case 0x84: _2A03_STORE(Y, NESRAM_write_byte, _2A03_zpAbsolute()); break;
			case 0x94: _2A03_STORE(Y, NESRAM_write_byte, _2A03_zpIndexed(X)); break;
			case 0x8c: _2A03_STORE(Y, NESCTL_write_byte, _2A03_absolute()); break;

			//TAX - Transfer accumulator to index X//
			case 0xaa: S &= ~(N | Z); X = (__UINT_8)(A); _2A03_SET_NZ(X); break;

			//TAY - Transfer accumulator to index Y//
			case 0xa8: S &= ~(N | Z); Y = (__UINT_8)(A); _2A03_SET_NZ(Y); break;

			//TSX - Transfer stack pointer to index X//
			case 0xba: S &= ~(N | Z); X = (__UINT_8)(SP); _2A03_SET_NZ(X); break;

			//TXA - Transfer index X to accumulator//
			case 0x8a: S &= ~(N | Z); A = X; _2A03_SET_NZ(A); break;

			//TXS - Transfer index X to stack pointer//
			case 0x9a: SP = X; break;

			//TYA - Transfer index Y to accumulator//
			case 0x98: S &= ~(N | Z); A = Y; _2A03_SET_NZ(A); break;

			//BRK - Force break (produce a NMI) //
			case 0x00: 
				if(_2A03_labelHolder)
				{
					nes->BankJMPList->insert_label(PC.W + 1, TYPE_DATA, TYPE_BYTE, 0, 0);
					nes->BankJMPList->insert_label(PC.W + 2, TYPE_CODE, TYPE_CODE, 0, 0);
                }
				S |= (FLAG_B | I);
				_2A03_PUSH((PC.W + 2) >> 8);
				_2A03_PUSH(PC.W + 2);
				_2A03_PUSH(S);
				PC.W = NESPRGRAM_ReadByte(0xfffe) | (NESPRGRAM_ReadByte(0xffff) << 8);
				continue;

			default:
				if(_2A03_instructionDumper) nes->general_log.f_write("sbs", "2A03: Invalid Instruction: ", NESPRGRAM_ReadByte(PC.W), "\r\n");
				break;
		}

		PC.W++;
	}		
	
	return;
}
void _2A03_toggle_label_holder(void) { _2A03_labelHolder = (_2A03_labelHolder) ? FALSE : TRUE; }
void _2A03_toggle_tracer(void) { _2A03_instructionDumper = (_2A03_instructionDumper) ? FALSE : TRUE; }
void _2A03_set_label_holder(__BOOL status) { _2A03_labelHolder = status; }
void _2A03_set_tracer(__BOOL status) { _2A03_instructionDumper = status; }
__BOOL _2A03_get_tracer(void) { return _2A03_instructionDumper; }

/******************************************************************************/
/** Timing                                                                   **/
/******************************************************************************/

void _2A03_kill_cycles(__INT_32 cycles) { iCurrentTime += cycles; }

__INT_32 _2A03_get_current_time(void) { return iCurrentTime; }
__INT_32 _2A03_get_end_time(void) { return iEndTime; }
__INT_32 _2A03_getRelativeTime(void) { return iEndTime - iCurrentTime; }

void _2A03_set_current_time(__INT_32 iTime) { iCurrentTime = iTime; }
void _2A03_set_end_time(__INT_32 iTime) 
{
	if(iTime < iCurrentTime)
	{
		iTime = iCurrentTime;
    }
    iEndTime = iTime;
}

__BOOL _2A03_has_enough_cycles(void)
{
	return (iEndTime >= iCurrentTime) ? TRUE : FALSE;
}

/******************************************************************************/
/** Emulator Specifics                                                       **/
/******************************************************************************/

void _2A03_save_state(c_tracer &TDump)
{
	TDump.write(&iCurrentTime, 4);
	TDump.write(&iEndTime, 4);
	TDump.write(&PC.W, 2);
	TDump.write(&A, 1);
	TDump.write(&S, 1);
	TDump.write(&X, 1);
	TDump.write(&Y, 1);
    TDump.write(&SP, 1);
	TDump.write(&_2A03_unfinishedOp, 1);
	TDump.write(&_2A03_IRQLine, 1);
	TDump.write(&_2A03_IRQRequested, 1);
	TDump.write(&_2A03_NMIRequested, 1);
	TDump.write(&_2A03_NMISecondRequested, 1);
}

void _2A03_load_state(c_tracer &TDump)
{
	TDump.read(&iCurrentTime, 4);
	TDump.read(&iEndTime, 4);
	TDump.read(&PC.W, 2);
	TDump.read(&A, 1);
	TDump.read(&S, 1);
	TDump.read(&X, 1);
	TDump.read(&Y, 1);
    TDump.read(&SP, 1);
	TDump.read(&_2A03_unfinishedOp, 1);
	TDump.read(&_2A03_IRQLine, 1);
	TDump.read(&_2A03_IRQRequested, 1);
	TDump.read(&_2A03_NMIRequested, 1);
	TDump.read(&_2A03_NMISecondRequested, 1);
}

void _2A03_disassembleInstruction(__INT_32 iROMOffset)
{
	nes->general_log.f_write("ws", iROMOffset, " ");
	nes->general_log.f_write("sbsbsbsb", "A=", A, " X=", X, " Y=", Y, " SP=", SP);
	nes->general_log.f_write("s", " F= ");
	nes->general_log.f_write("s", (S & BIT_7) ? "N " : "  ");
	nes->general_log.f_write("s", (S & BIT_6) ? "V " : "  ");
	nes->general_log.f_write("s", (S & BIT_4) ? "B " : "  ");
	nes->general_log.f_write("s", (S & BIT_3) ? "D " : "  ");
	nes->general_log.f_write("s", (S & BIT_2) ? "I " : "  ");
	nes->general_log.f_write("s", (S & BIT_1) ? "Z " : "  ");
	nes->general_log.f_write("s", (S & BIT_0) ? "C " : "  ");
	nes->general_log.f_write("sds", " Cycle:", _2A03_getRelativeTime(), " ");
	nes->general_log.f_write("ss", _2A03_instructionSet[NESPRGRAM_ReadByte(iROMOffset)], " ");

	union NESROMData
	{
		__UINT_8 b;
		__UINT_16 w;
	} read;

	switch(_2A03_instructionAddrMode[NESPRGRAM_ReadByte(iROMOffset)])
	{
		case IMM:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("sb", "#", read.b);
			break;
		case ZPA:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("b", read.b);
			break;
		case ZPX:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("bs", read.b, ", x");
			break;
		case ZPY:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("bs", read.b, ", y");
			break;
		case JMP:
		case AB_:
			read.w = NESPRGRAM_ReadByte(++iROMOffset);
			read.w += NESPRGRAM_ReadByte(++iROMOffset) << 8;
			nes->general_log.f_write("w", read.w);
			break;
		case ABX:
			read.w = NESPRGRAM_ReadByte(++iROMOffset);
			read.w += NESPRGRAM_ReadByte(++iROMOffset) << 8;
			nes->general_log.f_write("ws", read.w, ", x");
			break;
		case ABY:
			read.w = NESPRGRAM_ReadByte(++iROMOffset);
			read.w += NESPRGRAM_ReadByte(++iROMOffset) << 8;
			nes->general_log.f_write("ws", read.w, ", y");
			break;
		case IDR:
			read.w = NESPRGRAM_ReadByte(++iROMOffset);
			read.w += NESPRGRAM_ReadByte(++iROMOffset) << 8;
			nes->general_log.f_write("sws", "(", read.w, ")");
			break;
		case PRE:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("sbs", "(", read.b, ", x)");
			break;
		case POS:
			read.b = NESPRGRAM_ReadByte(++iROMOffset);
			nes->general_log.f_write("sbs", "(", read.b, "), y");
			break;
		case IMP:
			break;
		case REL:
			read.w = ++iROMOffset;
			read.w += (INT_8)(NESPRGRAM_ReadByte(iROMOffset)) + 1;
			nes->general_log.f_write("w", read.w);
			break;
        case NIL:
			break;
	}

	nes->general_log.f_write("s", "\r\n");
}

int _2A03_Check_Code_Sanity(char *operands,
                            int bank_lo,
                            int bank_hi,
                            int address,
                            int bank_alias)
{
    char constant[1024];
	s_label_node *label1;
	s_label_node *label2;
	int offset = 0;

    label1 = nes->BankJMPList->search_label(bank_lo, bank_hi, address);
    if(label1->type == TYPE_DATA)
    {
        do
        {
            offset++;
            label2 = nes->BankJMPList->search_label(bank_lo, bank_hi, address + offset);
        }
        while(label2->type == TYPE_UNK);
        sprintf(constant, "\nLbl_%.04x = Lbl_%.04x - %d",
                          address,
                          address + offset,
                          offset);
        strcat(operands, constant);
        return(1);
    }
    return(1);
}

int get_cross_ref_validity(int address,
                           int bank_alias,
                           int bank_lo,
                           int bank_hi,
                           int code_jmp)
{
	s_label_node *label;

    label = nes->BankJMPList->search_label(0,
		                                  nes->o_mapper->get_max_pages(),
			                              address);
    // Didn't found it anywhere
    if(label->alias == -2) return(0);

	if(code_jmp)
	{
		if(label->type == TYPE_CODE)
		{
		    return(1);
        }
		return(0);
    }
    else
    {
		if(label->type == TYPE_CODE)
		{
		    return(0);
        }
		return(1);
    }
}

int _2A03_get_instruction(int base_addr,
                          int address,
                          int bank_lo,
                          int bank_hi,
                          __INT_32 iROMOffset,
                          char *instruction,
                          int bank_alias)
{
    char operands[1024];
    int length = 1;
    int code_jmp = 0;
    int offset;
    int writing;
    int addr;
	s_label_node *label;
	
	union NESROMData
	{
		__UINT_8 b;
		__UINT_16 w;
	} read;

    sprintf(operands, "");

    if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address, bank_alias)) return(0);
    sprintf(instruction, "        %s", _2A03_instructionSet[nes->o_cpu->PRGROM[iROMOffset]]);

	if(strcmp(instruction, "        (undef)") == 0)
    {
        // Shouldn't occur under normal circumstances.
        nes->BankJMPList->insert_label_bank(bank_lo, address, TYPE_DATA, TYPE_BYTE, 1, 0);
        return(0);
    }

    switch(nes->o_cpu->PRGROM[iROMOffset])
    {
        // JMP abs
        case 0x4c:
        // JSR
        case 0x20:
            code_jmp = 1;
            break;
    }

    writing = 0;
    switch(nes->o_cpu->PRGROM[iROMOffset])
    {
		case 0x8d:
		case 0x9d:
		case 0x99:
		    writing = 1;
		    break;
    }
    addr = _2A03_instructionAddrMode[nes->o_cpu->PRGROM[iROMOffset]];

	switch (addr)
	{
		case IMM: // OK
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " #$%.02x", read.b);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
		case ZPA: // OK
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " $%.02x", read.b);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
  		    break;
		case ZPX: // OK
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " $%.02x, x", read.b);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
		case ZPY: // OK
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " $%.02x, y", read.b);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
		case AB_2:   // OK
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
		        if(writing)
		        {
		            write_address(operands, read.w);
			    }
			    else
			    {
			        if(get_cross_ref_validity(read.w, bank_alias, bank_lo, bank_hi, code_jmp))
			        {
			            sprintf(operands, " Lbl_%.04x", read.w);
			        }
			        else
			        {
			            sprintf(operands, " $%.04x ; --- WARNING: unknown switchable bank location !", read.w);
                    }
                }
			}
			else
			{
			    // Optimizing can produce shifts if not all
			    // pointers are decoded.
			    // (Ca65 will optimize that anyway).
			    //if((int) read.w < 0x100) 
			    //sprintf(operands, " $%.02x", read.w);
			    //else 
		        write_address(operands, read.w);
		    }
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 2, bank_alias)) return(0);
		    length += 2;
			break;
		case JMP:
		case AB_:   // OK
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
		        if(writing)
		        {
		            write_address(operands, read.w);
			    }
			    else
			    {
			        if(get_cross_ref_validity(read.w, bank_alias, bank_lo, bank_hi, code_jmp))
			        {
			            sprintf(operands, " Lbl_%.04x", read.w);
			        }
			        else
			        {
                        sprintf(operands, " $%.04x ; --- WARNING: unknown switchable bank location !", read.w);
                    }
                }
			}
			else
			{
			    // Optimizing can produce shifts if not all
			    // pointers are decoded.
			    // (Ca65 will optimize that anyway).
			    //if((int) read.w < 0x100) 
			    //sprintf(operands, " $%.02x", read.w);
			    //else 
    		    write_address(operands, read.w);
			}
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 2, bank_alias)) return(0);
		    length += 2;
			break;
		case ABX: // OK
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
            label = nes->BankJMPList->search_label(bank_lo, bank_hi, address + 1);
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
		        if(writing)
		        {
    		        write_address(operands, read.w);
			        strcat(operands, ", x");
			    }
			    else
			    {
			        if(get_cross_ref_validity(read.w, bank_alias, bank_lo, bank_hi, code_jmp))
			        {
    			        sprintf(operands, " Lbl_%.04x, x", read.w);
			        }
			        else
			        {
                        sprintf(operands, " $%.04x, x ; --- WARNING: unknown switchable bank location !", read.w);
                    }
                }
			}
			else
			{
			    if((int) read.w < 0x100)
			    {
			        sprintf(operands, " $%.02x, x", read.b);
			    }
			    else
			    {
		            write_address(operands, read.w);
			        strcat(operands, ", x");
		        }
		    }
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 2, bank_alias)) return(0);
		    length += 2;
			break;
		case ABY: // OK
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
		        if(writing)
		        {
		            write_address(operands, read.w);
			        strcat(operands, ", y");
			    }
			    else
			    {
			        if(get_cross_ref_validity(read.w, bank_alias, bank_lo, bank_hi, code_jmp))
			        {
			            sprintf(operands, " Lbl_%.04x, y", read.w);
			        }
			        else
			        {
                        sprintf(operands, " $%.04x, y ; --- WARNING: unknown switchable bank location !", read.w);
                    }
                }
			}
			else
			{
			    if((int) read.w < 0x100)
			    {
			        sprintf(operands, " $%.02x, y", read.b);
			    }
			    else
			    {
		            write_address(operands, read.w);
			        strcat(operands, ", y");
		        }
		    }
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 2, bank_alias)) return(0);
		    length += 2;
			break;
		case IDR: // OK
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
			    if(get_cross_ref_validity(read.w, bank_alias, bank_lo, bank_hi, code_jmp))
			    {
			        sprintf(operands, " (Lbl_%.04x)", read.w);
			    }
			    else
			    {
			        sprintf(operands, " ($%.04x) ; --- WARNING: unknown switchable bank location !", read.w);
                }
			}
			else
			{
			    if((int) read.w < 0x100)
			    {
					sprintf(operands, " ($%.02x) ; --- WARNING: unknown switchable bank location !", read.b);
			    }
			    else
			    {
		            write_address(operands, read.w);
		        }
		    }
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 2, bank_alias)) return(0);
		    length += 2;
			break;
		case PRE:
		    // In zero page
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " ($%.02x, x)", read.b);

			if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
		case POS:
		    // In zero page
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
			sprintf(operands, " ($%.02x), y", read.b);

			if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
		case IMP:
			break;
		case REL:
		    offset = nes->o_cpu->PRGROM[++iROMOffset];
			read.w = address;
			read.w += (int) ((short) ((char) offset)) + 2;
			sprintf(operands, " Lbl_%.04x", read.w);
            if(!_2A03_Check_Code_Sanity(operands, bank_lo, bank_hi, address + 1, bank_alias)) return(0);
		    length++;
			break;
    	case NIL:
			break;
	}
    strcat(operands, "\n");

    strcat(instruction, operands);
    return(length);
}

int _2A03_map_instruction(int base_addr,
                          int address,
                          int bank,
                          __INT_32 iROMOffset,
                          int bank_alias)
{
    char operands[1024];
    int length = 1;
    int code_jmp = 0;
    int offset;
    int writing;

	union NESROMData
	{
		__UINT_8 b;
		__UINT_16 w;
	} read;

    sprintf(operands, "%s", _2A03_instructionSet[nes->o_cpu->PRGROM[iROMOffset]]);
    if(strcmp(operands, "(undef)") == 0)
    {
        nes->BankJMPList->insert_label_bank(bank_alias, address, TYPE_DATA, TYPE_BYTE, 1, 0);
        return(-1);
    }

    switch(nes->o_cpu->PRGROM[iROMOffset])
    {
        // JMP abs
        case 0x4c:
        // JSR
        case 0x20:
            code_jmp = 1;
            break;
    }

    writing = 0;
    switch(nes->o_cpu->PRGROM[iROMOffset])
    {
		case 0x8d:
		case 0x9d:
		case 0x99:
		    writing = 1;
		    break;
    }

	switch (_2A03_instructionAddrMode[nes->o_cpu->PRGROM[iROMOffset]])
	{
		case IMM:
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case ZPA:
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case ZPX:
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case ZPY:
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case JMP:
		case AB_:
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
                if(code_jmp)
                {
                    if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_CODE, TYPE_CODE, 0, 0))
                    {
                        return(-1);
                    }
			    }
			    else
			    {
			        if(!writing)
			        {
                        if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_DATA, TYPE_BYTE, 1, 0))
                        {
                            return(-1);
                        }
                    }
			    }
			}
		    length += 2;
			break;
		case ABX:
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
                if(code_jmp)
                {
                    if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_CODE, TYPE_CODE, 0, 0))
                    {
                        return(-1);
                    }
			    }
			    else
			    {
			        if(!writing)
			        {
                        if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_DATA, TYPE_BYTE, 1, 0))
                        {
                            return(-1);
                        }
                    }
			    }
			}
		    length += 2;
			break;
		case ABY:
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;

			if((int) read.w > 0x7fff)
			{
                if(code_jmp)
                {
                    if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_CODE, TYPE_CODE, 0, 0))
                    {
                        return(-1);
                    }
			    }
			    else
			    {
			        if(!writing)
			        {
                        if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_DATA, TYPE_BYTE, 1, 0))
                        {
                            return(-1);
                        }
                    }
			    }
			}
		    length += 2;
			break;
		case IDR:
			read.w = nes->o_cpu->PRGROM[++iROMOffset];
			read.w += nes->o_cpu->PRGROM[++iROMOffset] << 8;
			if((int) read.w > 0x7fff)
			{
                if(code_jmp)
                {
                    if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_CODE, TYPE_CODE, 0, 0))
                    {
                        return(-1);
                    }
                }
			    else
			    {
                    if(nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_DATA, TYPE_BYTE, 1, 0))
                    {
                        return(-1);
                    }
			    }
			}
		    length += 2;
			break;
		case PRE:
		    // In zero page
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case POS:
		    // In zero page
			read.b = nes->o_cpu->PRGROM[++iROMOffset];
		    length++;
			break;
		case IMP:
			break;
		case REL:
		    offset = nes->o_cpu->PRGROM[++iROMOffset];
			read.w = address;
			read.w += (int) ((short) ((char) offset)) + 2;
            nes->BankJMPList->insert_label_bank(bank_alias, read.w, TYPE_CODE, TYPE_RELCODE, 0, 0);
		    length++;
			break;
    	case NIL:
			break;
	}

    return(length);
}

int write_address(char *operands, int dat)
{
    char temp[1024];
    switch(dat)
    {
	    case 0x2000:
		    sprintf(temp, " PPU_CTRL1");
            strcat(operands, temp);
		    return 0;
	    case 0x2001:
		    sprintf(temp, " PPU_CTRL2");
            strcat(operands, temp);
		    return 0;
	    case 0x2002:
		    sprintf(temp, " PPU_STATUS");
            strcat(operands, temp);
		    return 0;
	    case 0x2003:
		    sprintf(temp, " PPU_SPRMEM");
            strcat(operands, temp);
		    return 0;
	    case 0x2004:
		    sprintf(temp, " PPU_SPRDAT");
            strcat(operands, temp);
		    return 0;
	    case 0x2005:
		    sprintf(temp, " PPU_SCROLL");
            strcat(operands, temp);
		    return 0;
	    case 0x2006:
		    sprintf(temp, " PPU_MEM");
            strcat(operands, temp);
		    return 0;
	    case 0x2007:
		    sprintf(temp, " PPU_MEMDAT");
            strcat(operands, temp);
		    return 0;
	    case 0x4000:
		    sprintf(temp, " APU_SQU1_REG1");
            strcat(operands, temp);
		    return 0;
	    case 0x4001:
		    sprintf(temp, " APU_SQU1_REG2");
            strcat(operands, temp);
		    return 0;
	    case 0x4002:
		    sprintf(temp, " APU_SQU1_REG3");
            strcat(operands, temp);
		    return 0;
	    case 0x4003:
		    sprintf(temp, " APU_SQU1_REG4");
            strcat(operands, temp);
		    return 0;
	    case 0x4004:
		    sprintf(temp, " APU_SQU2_REG1");
            strcat(operands, temp);
		    return 0;
	    case 0x4005:
		    sprintf(temp, " APU_SQU2_REG2");
            strcat(operands, temp);
		    return 0;
	    case 0x4006:
		    sprintf(temp, " APU_SQU2_REG3");
            strcat(operands, temp);
		    return 0;
	    case 0x4007:
		    sprintf(temp, " APU_SQU2_REG4");
            strcat(operands, temp);
		    return 0;
	    case 0x4008:
		    sprintf(temp, " APU_TRI_REG1");
            strcat(operands, temp);
		    return 0;
	    case 0x4009:
		    sprintf(temp, " APU_TRI_REG2");
            strcat(operands, temp);
		    return 0;
	    case 0x400a:
		    sprintf(temp, " APU_TRI_REG3");
            strcat(operands, temp);
		    return 0;
	    case 0x400b:
		    sprintf(temp, " APU_TRI_REG4");
            strcat(operands, temp);
		    return 0;
	    case 0x400c:
		    sprintf(temp, " APU_NOISE_REG1");
            strcat(operands, temp);
		    return 0;
	    case 0x400d:
		    sprintf(temp, " APU_NOISE_REG2");
            strcat(operands, temp);
		    return 0;
	    case 0x400e:
		    sprintf(temp, " APU_NOISE_REG3");
            strcat(operands, temp);
		    return 0;
	    case 0x400f:
		    sprintf(temp, " APU_NOISE_REG4");
            strcat(operands, temp);
		    return 0;
	    case 0x4014:
		    sprintf(temp, " PPU_SPR_DMA");
            strcat(operands, temp);
		    return 0;
	    case 0x4015:
		    sprintf(temp, " APU_CTRL");
            strcat(operands, temp);
		    return 0;
	    case 0x4016:
		    sprintf(temp, " NES_JOY1");
            strcat(operands, temp);
		    return 0;
	    case 0x4017:
		    sprintf(temp, " NES_JOY2");
            strcat(operands, temp);
		    return 0;
        default:
		    sprintf(temp, " $%.04x", dat);
            strcat(operands, temp);
		    return 1;
    }
}
