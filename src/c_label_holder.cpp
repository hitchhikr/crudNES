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

#include <string.h>
#include <stdlib.h>

#include "include/c_label_holder.h"
#include "include/c_tracer.h"
#include "include/c_cpu.h"
#include "include/c_ppu.h"
#include "include/mappers/c_mapper.h"
#include "include/datatypes.h"

#include "include/c_save_state.h"
#include "include/c_nes.h"

extern c_machine *o_machine;

c_label_holder :: c_label_holder (void)
{
    char *line;
    int bank;
    int contents;
    int type;
    int sub_type;
    int jump_table;

    memset(&unknown, 0, sizeof(unknown));
    unknown.type = TYPE_UNK;
    unknown.sub_type = TYPE_BYTE;
    unknown.alias = -2;

    // Load all the labels
	c_tracer reader (nes->Labels_Name, __READ);
    head = NULL;

    // skip header
    reader.f_read("%s")->string;
    
    line = reader.f_read("%s")->string;

    while(strlen(line))
    {
        // parse the data line
        bank = strtol(line, &line, 16);
        // Skip comma
        line++;
        contents = strtol(line, &line, 16);
        // Skip comma
        line++;
        switch(((unsigned int ) *(unsigned int *) &line[0]))
        {
            case 'ATAD':
                type = TYPE_DATA;
                line += 5;
                switch(((unsigned int ) *(unsigned int *) &line[0]))
                {
                    case 'ETYB':
                        sub_type = TYPE_BYTE;
                        break;

                    case 'DROW':
                        sub_type = TYPE_WORD;
                        break;
                    case 'WWAR':
                        sub_type = TYPE_RAWWORD;
                        break;
                }
                break;

            case 'EDOC':
                type = TYPE_CODE;
                switch(((unsigned int ) *(unsigned int *) &line[0]))
                {
                    case 'EDOC':
                        sub_type = TYPE_CODE;
                        break;

                    case 'CLER':
                        sub_type = TYPE_RELCODE;
                        break;
                }
                break;
        }
        line += 5;
        // parse the data line
        jump_table = strtol(line, &line, 16);
        
        insert_label_bank(bank, contents, type, sub_type, 0, jump_table);

        line = reader.f_read("%s")->string;   
    }    
}

// Dump all labels before closing
c_label_holder :: ~c_label_holder (void)
{
	if (!head)
	{
	    return;
	}
    
	c_tracer writer (nes->Labels_Name);
	writer.f_write ("s", "Bank,Address,Type,Access,Sub,Jump\r\n");

	s_label_node *navigator = head;

    while(navigator)
	{
		writer.f_write ("nsnsssns",
		                navigator->bank,
		                ",",
		                navigator->contents,
		                navigator->type == TYPE_CODE ? ",CODE" : ",DATA",
		                navigator->type == TYPE_CODE ? 
		                    navigator->sub_type == TYPE_RELCODE ? ",RELC" : ",BYTE" :
		                    navigator->sub_type == TYPE_BYTE ? ",BYTE" : 
		                    navigator->sub_type == TYPE_WORD ? ",WORD" : ",RAWW",
		                ",",
		                navigator->jump_base_table,
		                "\n"
		               );

		navigator = navigator->Next;
	}
    // Free the pages mapping
	while(head)
	{
		navigator = head->Next;
		delete head;
		head = navigator;
	}
	
	
	writer.close ();
}

// -1 = none
// 1 = data
// 0 = code
s_label_node *c_label_holder::search_label(int bank_lo, int bank_hi, int address)
{
	s_label_node *navigator;

	for(navigator = head; navigator;)
	{
		if(navigator->bank >= bank_lo && navigator->bank <= bank_hi &&
		   navigator->contents == address)
		{
            return(navigator);
        }
        navigator = navigator->Next;
	}
    unknown.bank = bank_lo;
    return(&unknown);
}

void c_label_holder::dump_rom(void)
{
    int i;
    int k;
    char inc_name[1024];
    char bank_name[1024];
    char instruction[1024];
    SRominformation *infos;
    FILE *out;
    int instruction_size;
    int pos_export;
    int rom_offset;
    int rom_offset_glob;
    int label_type;
    int pos_data;
    int sub_t;
    int double_label;
    int nbr_prg_pages;
    int nbr_chr_pages;
    int w_dat;
    int in_raw_word;
    int no_labels;
    int old_k;
    int old_offset;
	s_label_node *label;
	s_label_node *navigator;
	s_label_node *pages;
    
	if (!head)
	{
	    return;
	}

    infos = &nes->o_rom->information();

    pages = nes->prg_pages;
    nbr_prg_pages = 0;
    if(pages)
    {
        rom_offset_glob = pages->offset;
        while(pages)
        {
            nbr_prg_pages++;
            pages = pages->Next;
        }
    }

    nbr_chr_pages = 0;
    pages = nes->chr_pages;
    if(pages)
    {
        while(pages)
        {
            nbr_chr_pages++;
            pages = pages->Next;
        }
    }

    pages = nes->prg_pages;

    if(pages)
    {
        nes->o_mapper->reset();

        // Pass 1: fix unresolved code labels
        for(i = 0; i < (int) nbr_prg_pages; i++)
        {
            rom_offset = rom_offset_glob;

            k = pages->address;
            while(k < pages->address + pages->size)
            {
                label = search_label(pages->bank_lo, pages->bank_hi, k);
                label_type = label->type;
                sub_t = label->sub_type;

                switch(label_type)
                {
                    case TYPE_CODE:
                        label_type = TYPE_UNK;
                        while(label_type == TYPE_UNK &&
                              k < pages->address + pages->size)
                        {
                            instruction_size = _2A03_map_instruction(pages->address,
                                                                     k,
                                                                     i,
                                                                     rom_offset,
                                                                     i);
                            if(instruction_size == -1)
                            {
                                // restart
                                k = pages->address;
                                rom_offset = rom_offset_glob;
                                break;
                            }
                            k += instruction_size;
                            switch(nes->o_cpu->PRGROM[rom_offset])
                            {
                                // BRK
                                case 0x0:
                                // JMP abs
                                case 0x4c:
                                // JMP ind
                                case 0x6c:
                                // RTI
                                case 0x40:
                                // RTS
                                case 0x60:
                                    rom_offset += instruction_size;
                                    label_type = TYPE_DATA;
                                    break;
                                default:
                                    rom_offset += instruction_size;
                                    label = search_label(pages->bank_lo, pages->bank_hi, k);
                                    label_type = label->type;
                                    sub_t = label->sub_type;
                                    break;
                            }
                        }
                        break;

                    case TYPE_DATA:       // Data
                    case TYPE_UNK:        // Unknown

                        // Check if all the entries are located in the code
                        // Determine the size of the array of bytes to dump
                        label_type = TYPE_UNK;
                        in_raw_word = 0;
                        no_labels = 0;
                        old_offset = rom_offset;
                        old_k = k;
                        while(label_type == TYPE_UNK && k < pages->address + pages->size)
                        {
                            switch(sub_t)
                            {
                                case TYPE_WORD:
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
		                            if((int) w_dat <= 0x7fff)
		                            {
		                                no_labels++;
		                            }
	                                break;

                                case TYPE_RAWWORD:
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
		                            if((int) w_dat <= 0x7fff)
		                            {
		                                no_labels++;
		                            }
		                            else
		                            {
                                        nes->BankJMPList->insert_label_bank(i,
                                                          w_dat,
                                                          TYPE_DATA,
                                                          TYPE_BYTE, 0, 0);
		                            }
		                            in_raw_word = 1;
	                                break;

                                default:
		                            no_labels++;
		                            in_raw_word = 0;
		                            break;
	                        }
    	                    
                            if(sub_t == TYPE_WORD ||
                               sub_t == TYPE_RAWWORD)
                            {
                                k += 2;
                                rom_offset += 2;
                            }
                            else
                            {
                                k++;
                                rom_offset++;
                            }
                            label = search_label(pages->bank_lo, pages->bank_hi, k);
                            label_type = label->type;
                            sub_t = label->sub_type;
	                        if(in_raw_word && label_type == TYPE_UNK)
	                        {
	                            sub_t = TYPE_RAWWORD;
	                        }
	                        if(no_labels) break;
                        }
                        rom_offset = old_offset;
                        k = old_k;

                        // Determine the size of the array of bytes to dump
                        label_type = TYPE_UNK;
                        while(label_type == TYPE_UNK && k < pages->address + pages->size)
                        {
                            if(sub_t == TYPE_WORD || sub_t == TYPE_RAWWORD)
                            {
                                k += 2;
                                rom_offset += 2;
                            }
                            else
                            {
                                k++;
                                rom_offset++;
                            }
                            label = search_label(pages->bank_lo, pages->bank_hi, k);
                            label_type = label->type;
                            sub_t = label->sub_type;
                        }
                        break;
                }
            }
            pages = pages->Next;
            if(pages) rom_offset_glob = pages->offset;
        }


        pages = nes->prg_pages;
        rom_offset_glob = pages->offset;
        
        // Create the constants file
        sprintf(inc_name, "%s_prg.inc", nes->Game_FileName);
        out = fopen(inc_name, "wb");
        if(out)
        {
            fprintf(out, "PPU_CTRL1       =       $2000\n");
            fprintf(out, "PPU_CTRL2       =       $2001\n");
            fprintf(out, "PPU_STATUS      =       $2002\n");

            fprintf(out, "PPU_SPRMEM      =       $2003\n");
            fprintf(out, "PPU_SPRDAT      =       $2004\n");
            fprintf(out, "PPU_SCROLL      =       $2005\n");

            fprintf(out, "PPU_MEM         =       $2006\n");
            fprintf(out, "PPU_MEMDAT      =       $2007\n");

            fprintf(out, "APU_SQU1_REG1   =       $4000\n");
            fprintf(out, "APU_SQU1_REG2   =       $4001\n");
            fprintf(out, "APU_SQU1_REG3   =       $4002\n");
            fprintf(out, "APU_SQU1_REG4   =       $4003\n");

            fprintf(out, "APU_SQU2_REG1   =       $4004\n");
            fprintf(out, "APU_SQU2_REG2   =       $4005\n");
            fprintf(out, "APU_SQU2_REG3   =       $4006\n");
            fprintf(out, "APU_SQU2_REG4   =       $4007\n");

            fprintf(out, "APU_TRI_REG1    =       $4008\n");
            fprintf(out, "APU_TRI_REG2    =       $4009\n");
            fprintf(out, "APU_TRI_REG3    =       $400a\n");
            fprintf(out, "APU_TRI_REG4    =       $400b\n");

            fprintf(out, "APU_NOISE_REG1  =       $400c\n");
            fprintf(out, "APU_NOISE_REG2  =       $400d\n");
            fprintf(out, "APU_NOISE_REG3  =       $400e\n");
            fprintf(out, "APU_NOISE_REG4  =       $400f\n");

            fprintf(out, "PPU_SPR_DMA     =       $4014\n");

            fprintf(out, "APU_CTRL        =       $4015\n");

            fprintf(out, "NES_JOY1        =       $4016\n");
            fprintf(out, "NES_JOY2        =       $4017\n");
            fclose(out);
        }

        double_label = 0;
        for(i = 0; i < (int) nbr_prg_pages; i++)
        {
            rom_offset = rom_offset_glob;
            sprintf(bank_name, "%s_%.03d_prg.asm", nes->Game_FileName, i);
            out = fopen(bank_name, "wb");
            if(!out) break;

            fprintf(out, "; Game name: %s\n", nes->Game_Name);
            fprintf(out, "; 16k prg-rom: %d\n", infos->prg_pages);
            fprintf(out, "; 8k chr-rom: %d\n", infos->chr_pages);
            fprintf(out, "; Mapper: %d\n", infos->mapper);
            fprintf(out, "; Mirroring: %d\n\n", infos->mirroring);
            fprintf(out, "        .autoimport +\n\n");
            fprintf(out, "        .include \"%s_prg.inc\"\n\n", nes->Game_Name);
            // Dump the exports of this bank

            int bnk_alias = pages->alias;

            // Write them 8 by 8
            navigator = head;
            pos_export = 0;
	        for(navigator = head; navigator;)
	        {
	            if(nes->o_mapper->get_bank_alias(i, navigator->contents) == i)
	            {
	                if(navigator->bank == bnk_alias)
	                {
		                if(navigator->bank >= pages->bank_lo ||
		                   navigator->bank <= pages->bank_hi &&
		                   navigator->sub_type != TYPE_RELCODE)
		                {
		                    if(navigator->sub_type == TYPE_WORD &&
		                       navigator->jump_base_table != navigator->contents)
		                    {
		                        goto No_Label;
	                        }
		                    if(navigator->sub_type == TYPE_RAWWORD &&
		                       navigator->jump_base_table != navigator->contents)
		                    {
		                        goto No_Label;
	                        }
	                        if((int) navigator->contents > 0x7fff &&
	                           (int) navigator->contents <= 0xfffe)
	                        {
	                            if((pos_export % 8) == 0)
	                            {
                                    fprintf(out, "        .export Lbl_%.04x",
                                                    navigator->contents);
	                            }
	                            else
	                            {
		                            fprintf(out, ", Lbl_%.04x",
		                                         navigator->contents);
	                            }
                            }
	                        if((pos_export % 8) == 7)
	                        {
                                fprintf(out, "\n");
	                            pos_export = -1;
	                        }
	                        pos_export++;
No_Label:;
		                }
                    }
                }
        	    navigator = navigator->Next;
	        }
	        if((pos_export % 8) != 0)
	        {
                fprintf(out, "\n");
	        }

            fprintf(out, "\n        .segment \"PRG_%d\"\n", i);

			rom_offset = rom_offset_glob;
            // Pass 2: Disassemble the complete bank
            k = pages->address;
            
            while(k < pages->address + pages->size)
            {
                label = search_label(pages->bank_lo, pages->bank_hi, k);
                label_type = label->type;
                sub_t = label->sub_type;
                pos_data = 0;

                if(label_type == TYPE_DATA &&
                   sub_t == TYPE_WORD ||
                   sub_t == TYPE_RAWWORD)
                {
                    if(label->jump_base_table == k)
                    {
                        if(k == nes->o_mapper->vectors_address)
                        {
                            fprintf(out, "\n        .segment \"VECTORS\"\n\n");
                            fprintf(out, "Lbl_%.04x:\n", k);
                        }
                        else
                        {
                            fprintf(out, "\nLbl_%.04x:\n", k);
                            fprintf(out, "Lbl_%.04x=Lbl_%.04x+1\n",
                                         k + 1,
                                         k);
                        }
                    }
                }
                else
                {
                    if(!double_label)
                    {
                        fprintf(out, "\nLbl_%.04x:\n",
                                     k);
                    }
                    double_label = 0;
                }
                    
                switch(label_type)
                {
                    case TYPE_CODE:
                        label_type = TYPE_UNK;
                        while(label_type == TYPE_UNK && k < pages->address + pages->size)
                        {
                            instruction_size = _2A03_get_instruction(pages->address,
                                                                     k,
                                                                     pages->bank_lo,
                                                                     pages->bank_hi,
                                                                     rom_offset,
                                                                     instruction,
                                                                     i);
                            if(instruction_size == 0)
                            {
                                fprintf(out, "\n; --- WARNING: Unreachable code ! ---\n");
                                double_label = 1;
                                break;
                            }
                            fprintf(out, instruction);
                            k += instruction_size;
                            switch(nes->o_cpu->PRGROM[rom_offset])
                            {
                                // BRK
                                case 0x0:
                                // JMP abs
                                case 0x4c:
                                // JMP ind
                                case 0x6c:
                                // RTI
                                case 0x40:
                                // RTS
                                case 0x60:
                                    rom_offset += instruction_size;
                                    label_type = TYPE_DATA;
                                    break;
                                default:
                                    rom_offset += instruction_size;
                                    label = search_label(pages->bank_lo, pages->bank_hi, k);
                                    label_type = label->type;
                                    sub_t = label->sub_type;
                                    break;
                            }
                        }
                        break;

                    case TYPE_DATA:       // Data
                    case TYPE_UNK:        // Unknown

                        // Determine the size of the array of bytes to dump
                        label_type = TYPE_UNK;
                        in_raw_word = 0;
                        while(label_type == TYPE_UNK && k < pages->address + pages->size)
                        {
                            switch(sub_t)
                            {
                                case TYPE_WORD:
	                                if((pos_data % 16) == 0)
	                                {
                                        fprintf(out, "        .word ");
                                    }
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
		                            if(w_dat > 0x7fff)
		                            {
		                                fprintf(out, "Lbl_%.04x", w_dat);
		                            }
		                            else
		                            {
		                                fprintf(out, "$%.04x", w_dat);
		                            }
		                            // Re-init the line
		                            pos_data = 15;
	                                break;

                                case TYPE_RAWWORD:
	                                if((pos_data % 16) == 0)
	                                {
                                        fprintf(out, "        .word ");
                                    }
                                    else
                                    {
                                        if(!in_raw_word)
                                        {
                                            fprintf(out, "\n");
                                            fprintf(out, "        .word ");
                                        }
                                    }
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
		                            if(w_dat > 0x7fff)
		                            {
		                                fprintf(out, "Lbl_%.04x", w_dat);
		                            }
		                            else
		                            {
		                                fprintf(out, "$%.04x", w_dat);
		                            }
		                            in_raw_word = 1;
		                            pos_data = 15;
	                                break;

                                default:
                                    if((pos_data % 16) == 0)
	                                {
                                        fprintf(out, "        .byte ");
                                    }
                                    else
                                    {
                                        if(in_raw_word)
                                        {
                                            fprintf(out, "\n");
                                            fprintf(out, "        .byte ");
                                        }
                                    }
		                            fprintf(out, "$%.02x", nes->o_cpu->PRGROM[rom_offset]);
		                            in_raw_word = 0;
		                            break;
	                        }
	                        if((pos_data % 16) == 15)
	                        {
                                fprintf(out, "\n");
	                        }
    	                    
                            if(sub_t == TYPE_WORD || sub_t == TYPE_RAWWORD)
                            {
                                k += 2;
                                rom_offset += 2;
                            }
                            else
                            {
                                k++;
                                rom_offset++;
                            }
                            label = search_label(pages->bank_lo, pages->bank_hi, k);
                            label_type = label->type;
                            sub_t = label->sub_type;
	                        if((pos_data % 16) != 15)
	                        {
	                            if(label_type != TYPE_UNK ||
	                               k >= pages->address + pages->size)
	                            {
                                    fprintf(out, "\n");
	                            }
	                            else
	                            {
                                    fprintf(out, ",");
	                            }
	                        }
	                        if(in_raw_word && label_type == TYPE_UNK)
	                        {
	                            sub_t = TYPE_RAWWORD;
	                        }
                            pos_data++;
                        }
                        break;
                }
            }
            
            fclose(out);
            pages = pages->Next;
            if(pages) rom_offset_glob = pages->offset;
        }
    }

    pages = nes->chr_pages;

    if(pages)
    {
        rom_offset_glob = pages->offset;
        //nes->o_ppu->decode_chr_rom();
        for(i = 0; i < nbr_chr_pages; i++)
        {
            rom_offset = rom_offset_glob;
            sprintf(bank_name, "%s_%.03d_chr.asm", nes->Game_FileName, pages->bank_lo);
            out = fopen(bank_name, "wb");
            if(!out) break;
            
            fprintf(out, "; Game name: %s\n", nes->Game_Name);
            fprintf(out, "; 16k prg-rom: %d\n", infos->prg_pages);
            fprintf(out, "; 8k chr-rom: %d\n", infos->chr_pages);
            fprintf(out, "; Mapper: %d\n\n", infos->mapper);
            fprintf(out, "; Mirroring: %d\n", infos->mirroring);

            fprintf(out, "        .autoimport +\n\n");
            fprintf(out, "\n        .segment \"CHR_%d\"\n", pages->bank_lo);
            
            // Dump the complete chr bank
            k = pages->address;
            while(k < pages->address + pages->size)
            {
                pos_data = 0;

                fprintf(out, "\nLbl_%.04x:\n", k);

                // Determine the size of the array of bytes to dump
                while(k < pages->address + pages->size)
                {
	                if((pos_data % 16) == 0)
	                {
                        fprintf(out, "        .byte ");
					}

		            fprintf(out, "$%.02x", nes->o_ppu->CHRROM[rom_offset]);

					if((pos_data % 16) == 15)
	                {
                        fprintf(out, "\n");
	                }
    	            
                    k++;
                    rom_offset++;

	                if((pos_data % 16) != 15)
	                {
	                    if(k >= pages->address + pages->size)
	                    {
                            fprintf(out, "\n");
	                    }
	                    else
	                    {
                            fprintf(out, ",");
	                    }
	                }
                    pos_data++;
                }
            }

            fclose(out);
            pages = pages->Next;
            if(pages) rom_offset_glob = pages->offset;
        }
    }

    // Generate the config file
    if(nbr_chr_pages || nbr_prg_pages)
    {
        sprintf(bank_name, "%s.cfg", nes->Game_FileName);
        out = fopen(bank_name, "wb");
        if(out)
        {
            fprintf(out, "MEMORY {\n");
            pages = nes->prg_pages;
            if(nes->prg_pages)
            {
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "    PRG%d: start = $%.04x, size = $%.04x;\n",
                                pages->bank_lo,
                                pages->address,
                                pages->size
                                );
                    pages = pages->Next;
                }
            }
            fprintf(out, "    VECTORS: start = $%.04x, size = 6;\n",
                         nes->o_mapper->vectors_address);

            pages = nes->chr_pages;
            if(nes->chr_pages)
            {
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "    CHR%d: start = $%.04x, size = $%.04x;\n",
                                pages->bank_lo,
                                pages->address,
                                pages->size
                                );
                    pages = pages->Next;
                }
            }

            fprintf(out, "}\n");

            fprintf(out, "SEGMENTS {\n");
            pages = nes->prg_pages;
            if(nes->prg_pages)
            {
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "    PRG_%d: load = PRG%d, start = $%.04x;\n",
                                pages->bank_lo,
                                pages->bank_lo,
                                pages->address
                                );
                    pages = pages->Next;
                }
            }
            fprintf(out, "    VECTORS: load = VECTORS, type = ro;\n");

            pages = nes->chr_pages;
            if(nes->chr_pages)
            {
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "    CHR_%d: load = CHR%d;\n",
                                pages->bank_lo,
                                pages->bank_lo
                                );
                    pages = pages->Next;
                }
            }
            fprintf(out, "}\n");
        }

        // Generate the header file
        sprintf(bank_name, "%s_header.bin", nes->Game_FileName);
        nes->dump_header(bank_name);

        // Generate the batch file
        sprintf(bank_name, "%s.bat", nes->Game_FileName);
        out = fopen(bank_name, "wb");
        if(out)
        {
            fprintf(out, "@echo off\n");
            if(nes->prg_pages)
            {
                pages = nes->prg_pages;
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "ca65.exe \"%s_%.03d_prg.asm\"\n", nes->Game_Name, pages->bank_lo);
                    fprintf(out, "if errorlevel 1 goto end\n");
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "ca65.exe \"%s_%.03d_chr.asm\"\n", nes->Game_Name, pages->bank_lo);
                    fprintf(out, "if errorlevel 1 goto end\n");
                    pages = pages->Next;
                }
            }
            fprintf(out, "ld65.exe -C \"%s.cfg\" -o \"%s.prg\"",
                        nes->Game_Name,
                        nes->Game_Name);
            if(nes->prg_pages)
            {
                pages = nes->prg_pages;
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, " \"%s_%.03d_prg.o\"", nes->Game_Name, pages->bank_lo);
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, " \"%s_%.03d_chr.o\"", nes->Game_Name, pages->bank_lo);
                    pages = pages->Next;
                }
            }
            fprintf(out, "\n");
            fprintf(out, "if errorlevel 1 goto end\n");
            fprintf(out, "copy /B \"%s_header.bin\"+\"%s.prg\" \"%s.nes\"\n",
                        nes->Game_Name,
                        nes->Game_Name,
                        nes->Game_Name
                );
            fprintf(out, ":end\n");
            fprintf(out, "del \"%s.prg\"\n", nes->Game_Name);
            if(nes->prg_pages)
            {
                pages = nes->prg_pages;
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "del \"%s_%.03d_prg.o\"\n", nes->Game_Name, pages->bank_lo);
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "del \"%s_%.03d_chr.o\"\n", nes->Game_Name, pages->bank_lo);
                    pages = pages->Next;
                }
            }
            
            fclose(out);
        }
    }

    // Free the pages mapping
	for(pages = nes->prg_pages; pages;)
	{
		nes->prg_pages = nes->prg_pages->Next;
		delete pages;
		pages = nes->prg_pages;
	}
	
	for(pages = nes->chr_pages; pages;)
	{
		nes->chr_pages = nes->chr_pages->Next;
		delete pages;
		pages = nes->chr_pages;
	}
}

void c_label_holder :: insert_label (__UINT_16 value, __UINT_8 type, __UINT_8 sub_type, int force, int base)
{
	s_label_node *new_label;

	if (!head)
	{
		__NEW (head, s_label_node);
		nes->o_mapper->create_label (head, value, type, sub_type, base);
		head->Next = NULL;
		return;
	}
	
	s_label_node *navigator = head;
    s_label_node *PrevNode = head;

	__BOOL is_found = FALSE;
	__UINT_8 bank = nes->o_mapper->get_bank_number (value);

	while ((NULL != navigator->Next) && !is_found)
	{
		if ((navigator->contents == value) &&
			(navigator->bank == bank))
	    {
	        is_found = TRUE;
	        break;
        }
		navigator = navigator->Next;
	}
	
	if ((navigator->contents == value) &&
		(navigator->bank == bank))
	{
	    if(force)
	    {
	        if(navigator->sub_type != TYPE_WORD &&
	            navigator->sub_type != TYPE_RAWWORD ||
	            type == TYPE_CODE)
	        {
	            navigator->type = type;
	            navigator->bank = bank;
	            navigator->sub_type = sub_type;
		        navigator->offset = 0;
		        navigator->jump_base_table = base;
            }

	        //navigator->bank = bank;
	        //navigator->type = type;
	        //navigator->sub_type = sub_type;
	    }
	    else
	    {
	        if(type == TYPE_CODE && navigator->type != TYPE_CODE)
	        {
	            // Replace the label if the new one is of CODE type
	            // Since it must be disassembled (code have precedence).
	            navigator->bank = bank;
	            navigator->type = TYPE_CODE;
	            navigator->sub_type = sub_type;
	        }
        }
        return;
    }

	if (!is_found)
	{
		__NEW (new_label, s_label_node);
		nes->o_mapper->create_label (new_label, value, type, sub_type, base);
		new_label->Next = NULL;
		navigator->Next = new_label;
	}
}

// Return 0 if label has been found.
int c_label_holder :: insert_label_bank (__UINT_8 bank, __UINT_16 value, __UINT_8 type, __UINT_8 sub_type, int force, int base)
{
	s_label_node *new_label;

	if (!head)
	{
		__NEW (head, s_label_node);
		head->contents = value;
        head->bank = bank;
		head->type = type;
		head->sub_type = sub_type;
		head->jump_base_table = base;
		head->Next = NULL;
		return 1;
	}
	
	s_label_node *navigator = head;
    s_label_node *PrevNode = head;
    bank = nes->o_mapper->get_bank_alias(bank, value);
    if(bank == 0xff) return(0);

	__BOOL is_found = FALSE;

	while ((NULL != navigator->Next) && !is_found)
	{
		if ((navigator->contents == value)
			&& (navigator->bank == bank))
	    {
	        is_found = TRUE;
	        break;
        }
		navigator = navigator->Next;
	}
	
	if ((navigator->contents == value) &&
		(navigator->bank == bank))
	{
        if(navigator->sub_type != TYPE_WORD ||
           type == TYPE_CODE)
        {
	        if(force)
	        {
	            if(navigator->sub_type != TYPE_WORD &&
	               navigator->sub_type != TYPE_RAWWORD ||
	               type == TYPE_CODE)
	            {
	                navigator->type = type;
	                navigator->bank = bank;
	                navigator->sub_type = sub_type;
		            navigator->offset = 0;
		            navigator->jump_base_table = base;
                }
	        }
	        else
	        {
	            if(type == TYPE_CODE && navigator->type != TYPE_CODE)
	            {
	                // Replace the label if the new one is of CODE type
	                // Since it must be disassembled (code have precedence).
	                navigator->type = TYPE_CODE;
	                navigator->bank = bank;
	                navigator->sub_type = sub_type;
	                navigator->jump_base_table = base;
	            }
            }
        }
        return 0;
    }

	if (!is_found)
	{
		__NEW (new_label, s_label_node);

		new_label->contents = value;
        new_label->bank = bank;
		new_label->type = type;
		new_label->sub_type = sub_type;
        new_label->jump_base_table = base;

		new_label->Next = NULL;
		navigator->Next = new_label;
        return 1;
	}
    return 0;
}
