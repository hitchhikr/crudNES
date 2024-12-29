/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes

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
#include "include/clist.h"

extern c_machine *o_machine;
CList <char *> listing;
extern int warnings;

c_label_holder :: c_label_holder (void)
{
    char *line;
	int real_bank;
    int bank;
    int contents;
    e_dattype type;
    e_dattype sub_type;
    int jump_table;
	int offset;
	int ref_bank;

    memset(&unknown, 0, sizeof(unknown));
    unknown.type = TYPE_UNK;
    unknown.sub_type = TYPE_BYTE;
    unknown.alias = -2;
	type = TYPE_DATA;
    sub_type = TYPE_BYTE;

    // Load all the labels
	c_tracer reader (nes->Labels_Name, __READ);
    head = NULL;

    // skip header
    reader.f_read("%s")->string;
    
    line = reader.f_read("%s")->string;

    while(strlen(line))
    {
        // parse the data line
        offset = strtol(line, &line, 16);
        // Skip comma
        line++;
		real_bank = strtol(line, &line, 16);
        // Skip comma
        line++;
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
                line += 5;
                switch(((unsigned int ) *(unsigned int *) &line[0]))
                {
                    case 'EDOC':
                        sub_type = TYPE_CODE;
                        break;

                    case 'CLER':
                        sub_type = TYPE_RELCODE;
                        break;

					case 'ETYB':
                        sub_type = TYPE_BYTE;
                        break;
               }
                break;
        }
        line += 5;
        jump_table = strtol(line, &line, 16);
        line += 1;
        ref_bank = strtol(line, &line, 16);
                
        insert_label_bank(real_bank, contents, type, sub_type, 0, jump_table, bank, offset, ref_bank);

        line = reader.f_read("%s")->string;   
    }    
}

// Dump all labels before closing
c_label_holder :: ~c_label_holder (void)
{

}

s_label_node *c_label_holder::search_label(int bank_lo, int bank_hi, int address, int page_alias, int real_ref, int all_refs)
{
	s_label_node *navigator;
	if(real_ref == -1)
	{
		real_ref = page_alias;
	}
	if(all_refs == -1)
	{
		for(navigator = head; navigator;)
		{
			if(navigator->bank_lo >= bank_lo && navigator->bank_hi <= bank_hi &&
			   navigator->contents == address && get_bank_alias(navigator->ref_bank, address) == real_ref
			  )
			{
				return(navigator);
			}
			navigator = navigator->Next;
		}
	}
	else
	{
		for(navigator = head; navigator;)
		{
			if(navigator->bank_lo >= bank_lo && navigator->bank_hi <= bank_hi &&
			   navigator->contents == address
			  )
			{
				return(navigator);
			}
			navigator = navigator->Next;
		}
	}
	unknown.bank = bank_lo;
    unknown.alias = page_alias;
    return(&unknown);
}

// returns a page within a range
s_label_node *c_label_holder::search_page(int address)
{
	s_label_node *pages;

	for(pages = nes->prg_pages; pages;)
	{
		if((address >= pages->address) && (address < (pages->address + pages->size)))
		{
            return(pages);
        }
        pages = pages->Next;
	}
    return(NULL);
}

int c_label_holder::search_unknown_value(int address)
{
	s_label_node *navigator;

	for(navigator = head; navigator;)
	{
		if(navigator->address == address)
		{
            return(navigator->alias);
        }
        navigator = navigator->Next;
	}
    return(-1);
}

int c_label_holder::search_base(int alias, int address)
{
	s_label_node *navigator;

    if(address)
    {
	    for(navigator = head; navigator;)
	    {
		    if(navigator->jump_base_table == address && 
                navigator->alias == alias
               )
		    {
                return(1);
            }
            navigator = navigator->Next;
	    }
    }
    return(0);
}

int c_label_holder::fix_var_bank(int value, int ref_bank)
{
	s_label_node *page;
	int found_bank;

	page = search_page(value);
	if(page)
	{
		if(page->alias != ref_bank && !is_current_page(ref_bank, value))
		{
			found_bank = search_unknown_value(value);
			if(found_bank != -1)
			{
				return found_bank;
			}
			page = get_page_from_bank(ref_bank);
			if(page->address > value)
			{
				return -1;
			}
			else
			{
				page = search_page(value);
				ref_bank = page->alias;
			}
		}
	}
	return ref_bank;
}

int c_label_holder::fix_rom_offset(int value, int ref_bank, int offset)
{
	s_label_node *page;

	page = get_page_from_bank(ref_bank);
	if(page)
	{
		offset = (page->rom_offset + (value - page->address));
	}
	return offset;
}

s_label_node *c_label_holder::get_page_from_bank(int bank)
{
	s_label_node *pages;

	for(pages = nes->prg_pages; pages;)
	{
		if(pages->alias == bank)
		{
			return pages;
        }
        pages = pages->Next;
	}
    return NULL;
}

int c_label_holder::get_bank_alias(int bank, int value)
{
	s_label_node *pages;

	for(pages = nes->prg_pages; pages;)
	{
		if((pages->bank == bank) &&
		   (value >= pages->address) && (value < (pages->address + pages->size))
		   )
		{
			return pages->alias;
        }
        pages = pages->Next;
	}
    return 0;
}

int c_label_holder::get_real_bank(int alias)
{
	s_label_node *pages;

	for(pages = nes->prg_pages; pages;)
	{
		if(pages->alias == alias)
		{
			return pages->bank;
        }
        pages = pages->Next;
	}
    return 0;
}

int c_label_holder::is_current_page(int bank, int address)
{
	s_label_node *pages;

	for(pages = nes->prg_pages; pages;)
	{
		if((address >= pages->address) && (address < (pages->address + pages->size)))
		{
			if(bank == pages->alias) return TRUE;
        }
        pages = pages->Next;
	}
    return FALSE;
}

void c_label_holder::dump_rom(void)
{
    int i;
	int j;
    int k;
    char inc_name[1024];
    char bank_name[1024];
    char instruction[1024];
	char line[1024];
	char dat_line[1024];
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
	int repass;
	int done_vectors;
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
	rom_offset_glob = 0;
    if(pages)
    {
        rom_offset_glob = pages->rom_offset;
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
		printf("Disassembling... ");

		nes->o_mapper->reset();

        // Pass 1: fix unresolved code labels
        for(i = 0; i < (int) nbr_prg_pages; i++)
        {
            rom_offset = rom_offset_glob;

            k = pages->address;
            while(k < pages->address + pages->size)
            {
                label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
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
                                                                     pages->alias,
																	 label->alias,
																	 sub_t);
                            if(instruction_size == -2)
                            {
								label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
								if(label->type == TYPE_CODE)
								{
									label->type = TYPE_DATA;
									label->sub_type = TYPE_DEAD;
								}
								instruction_size = 1;
                                label_type = TYPE_DATA;
                                break;
							}
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
//                                // JSR abs
  //                              case 0x20:
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
									label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
									label_type = label->type;
									sub_t = label->sub_type;
                                    break;
                            }
							if(label_type != TYPE_UNK)
							{
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
                                                          TYPE_BYTE, 0, 0, pages->alias, rom_offset);
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
                            label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
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
                            label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
                            label_type = label->type;
                            sub_t = label->sub_type;
                        }
                        break;
                }
            }
			pages = pages->Next;
			if(pages)
			{
				rom_offset_glob = pages->rom_offset;
			}
         }

		printf("Done.\n");
		printf("---------------------------------------------------------------------------------\n");
		printf("Assembly files generation:\n");

        pages = nes->prg_pages;
        rom_offset_glob = pages->rom_offset;
        done_vectors = FALSE;

        // Create the constants file
        sprintf(inc_name, "%s_prg.inc", nes->Game_FileName);
        out = fopen(inc_name, "wb");
        if(out)
        {
			printf("Generating \"%s\" file... ", inc_name);

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

            fprintf(out, "DMC_CTRL        =       $4010\n");
            fprintf(out, "DMC_LEV         =       $4011\n");
            fprintf(out, "DMC_ADDR        =       $4012\n");
            fprintf(out, "DMC_LEN         =       $4013\n");

            fprintf(out, "PPU_SPR_DMA     =       $4014\n");

            fprintf(out, "APU_CTRL        =       $4015\n");

            fprintf(out, "NES_JOY1        =       $4016\n");
            fprintf(out, "NES_JOY2        =       $4017\n");
            fclose(out);

			printf("Done.\n");
		}

        for(i = 0; i < (int) nbr_prg_pages; i++)
        {
			double_label = 0;
            rom_offset = rom_offset_glob;
            sprintf(bank_name, "%s_%.03d_prg.asm", nes->Game_FileName, i);
            out = fopen(bank_name, "wb");
            if(!out) break;

			listing.Erase();
			printf("Generating \"%s\" bank file... ", bank_name);

			sprintf(line, "; Game name: %s\n", nes->Game_Name);
            listing.Add(line);
            sprintf(line, "; 16k prg-rom: %d\n", infos->prg_pages);
            listing.Add(line);
            sprintf(line, "; 8k chr-rom: %d\n", infos->chr_pages);
            listing.Add(line);
            sprintf(line, "; Mapper: %d\n", infos->mapper);
            listing.Add(line);
            sprintf(line, "; Mirroring: %d\n", infos->mirroring);
            listing.Add(line);
            sprintf(line, "; ------------------------------\n");
            listing.Add(line);
			sprintf(line, "; Disassembled with "APPNAME" "APPVERSION"\n");
            listing.Add(line);
            sprintf(line, "; ------------------------------\n\n");
            listing.Add(line);
            sprintf(line, "        .autoimport +\n\n");
            listing.Add(line);
            sprintf(line, "        .include \"%s_prg.inc\"\n\n", nes->Game_Name);
            listing.Add(line);

			// Dump the exports of this bank

            // Write them 8 by 8
            navigator = head;
            pos_export = 0;
			warnings = 0;
	        for(navigator = head; navigator;)
	        {
		        if(navigator->ref_bank <= pages->bank)
		        {
					if(navigator->alias == pages->alias &&
						navigator->sub_type != TYPE_RELCODE &&
						navigator->sub_type != TYPE_DEAD)
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
						if((int) navigator->contents >= pages->address &&
						   (int) navigator->contents < (pages->address + pages->size))
						{
							if((int) navigator->contents > 0x7fff &&
							   (int) navigator->contents <= 0xfffe)
							{
								if((pos_export % 8) == 0)
								{
									sprintf(line, "        .export ");
									listing.Add(line);
								}
								else
								{
									sprintf(line, ", ");
									listing.Add(line);
								}
								sprintf(line, "Lbl_%.02x%.04x", navigator->alias, navigator->contents);
								listing.Add(line);
								if((pos_export % 8) == 7)
								{
									sprintf(line, "\n");
									listing.Add(line);
									pos_export = -1;
								}
								pos_export++;
							}
						}
No_Label:;
					}
				}
				navigator = navigator->Next;
	        }
	        if((pos_export % 8) != 0)
	        {
                sprintf(line, "\n");
				listing.Add(line);
	        }

            sprintf(line, "\n        .segment \"PRG_%d\"\n", i);
            listing.Add(line);

			rom_offset = rom_offset_glob;
            // Pass 2: Disassemble the complete bank
            k = pages->address;
            repass = -1;
			while(k < pages->address + pages->size)
            {
                label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
                label_type = label->type;
                sub_t = label->sub_type;
                pos_data = 0;

				if(label_type == TYPE_DATA &&
                   sub_t == TYPE_WORD ||
                   sub_t == TYPE_RAWWORD)
                {
					if(repass < k)
					{
						if(label->jump_base_table == k)
						{
							if(k == nes->o_mapper->vectors_address)
							{
								if(!done_vectors)
								{
									sprintf(line, "\n        .segment \"VECTORS\"\n\n");
									done_vectors = TRUE;
								}
								listing.Add(line);
								sprintf(line, "Lbl_%.02x%.04x:\n", label->alias, k);
								listing.Add(line);
							}
							else
							{
								sprintf(line, "\nLbl_%.02x%.04x:\n", label->alias, k);
								listing.Add(line);
								sprintf(line, "Lbl_%.02x%.04x = Lbl_%.02x%.04x+1\n",
											label->alias,
											k + 1,
											label->alias,
											k);
								listing.Add(line);
							}
						}
					}
				}
                else
                {
					if(label_type != TYPE_UNK)
					{
						if(!double_label)
						{
							if(repass < k)
							{
								sprintf(line, "\nLbl_%.02x%.04x:\n",
											label->alias,
											k);
								listing.Add(line);
							}
						}
						double_label = 0;
					}
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
                                                                     pages->alias,
																	 label->bank,
																	 label->ref_bank,
																	 pages->bank);
                            if(instruction_size == -2)
                            {
								instruction_size = 1;
                                label_type = TYPE_DATA;
								goto Direct_Data;
                                break;
							}
                            if(instruction_size == 0xfffffff)
                            {
                                // restart
								repass = k;
                                k = pages->address;
                                rom_offset = rom_offset_glob;
                                break;
                            }
							if(k >= repass)
							{
								repass = -1;
							}
                            if(instruction_size == 0)
                            {
                                sprintf(line, "\n; <<< WARNING: Unreachable code !n");
								listing.Add(line);
                                double_label = 1;
                                break;
                            }
							if(repass < k)
							{
								sprintf(line, instruction);
								listing.Add(line);
							}
                            k += instruction_size;
                            switch(nes->o_cpu->PRGROM[rom_offset])
                            {
                                // BRK
                                case 0x0:
  //                              // JSR abs
//                                case 0x20:
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
									label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
									label_type = label->type;
									sub_t = label->sub_type;
                                    break;
                            }
							if(label_type != TYPE_UNK)
							{
								break;
							}
                        }
                        break;

                    case TYPE_DATA:       // Data
                    case TYPE_UNK:        // Unknown
Direct_Data:
                        // Determine the size of the array of bytes to dump
                        label_type = TYPE_UNK;
                        in_raw_word = 0;
						strcpy(line, "");
						strcpy(dat_line, "");
                        while(label_type == TYPE_UNK && k < pages->address + pages->size)
                        {
                            switch(sub_t)
                            {
                                case TYPE_WORD:
	                                if((pos_data % 16) == 0)
	                                {
										if(repass < k)
										{
											sprintf(dat_line, "        .word ");
											strcat(line, dat_line);
										}
                                    }
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
		                            if(w_dat > 0x7fff)
		                            {
										if(repass < k)
										{
											sprintf(dat_line, "Lbl_%.02x%.04x", pages->alias, w_dat);
											strcat(line, dat_line);
										}
		                            }
		                            else
									{
										if(repass < k)
										{
											sprintf(dat_line, "$%.04x", w_dat);
											strcat(line, dat_line);
										}
		                            }
		                            // Re-init the line
		                            pos_data = 15;
	                                break;

                                case TYPE_RAWWORD:
	                                if((pos_data % 16) == 0)
	                                {
										if(repass < k)
										{
											sprintf(dat_line, "        .word ");
											strcat(line, dat_line);
										}
                                    }
                                    else
                                    {
                                        if(!in_raw_word)
                                        {
											if(repass < k)
											{
												sprintf(dat_line, "\n");
												strcat(line, dat_line);
												sprintf(dat_line, "        .word ");
												strcat(line, dat_line);
											}
                                        }
                                    }
                                    w_dat = nes->o_cpu->PRGROM[rom_offset];
                                    w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
Set_Jump_Table_Start:
		                            if(w_dat > 0x7fff)
		                            {
										if(repass < k)
										{
											sprintf(dat_line, "Lbl_%.02x%.04x", pages->alias, w_dat);
											strcat(line, dat_line);
										}
		                            }
		                            else
		                            {
										if(repass < k)
										{
											sprintf(dat_line, "$%.04x", w_dat);
											strcat(line, dat_line);
										}
		                            }
		                            in_raw_word = 1;
		                            pos_data = 15;
	                                break;

                                default:
                                    if((pos_data % 16) == 0)
	                                {
										if(repass < k)
										{
                                            if(search_base(label->alias, label->address))
                                            {
                                                // It's a jump table start
								                sprintf(dat_line, "Lbl_%.02x%.04x = Lbl_%.02x%.04x+1\n",
											                label->alias,
											                k + 1,
											                label->alias,
											                k);
											    strcat(line, dat_line);

                                                sprintf(dat_line, "        .word ");
											    strcat(line, dat_line);
                                                w_dat = nes->o_cpu->PRGROM[rom_offset];
                                                w_dat |= nes->o_cpu->PRGROM[rom_offset + 1] << 8;
                                                // Skip it for next reading
                                                k++;
                                                rom_offset++;
                                                goto Set_Jump_Table_Start;
                                            }
                                            else
                                            {
											    sprintf(dat_line, "        .byte ");
											    strcat(line, dat_line);
                                            }
										}
                                    }
                                    else
                                    {
                                        if(in_raw_word)
                                        {
											if(repass < k)
											{
												sprintf(dat_line, "\n");
												strcat(line, dat_line);
												sprintf(dat_line, "        .byte ");
												strcat(line, dat_line);
											}
                                        }
                                    }
									if(repass < k)
									{
										sprintf(dat_line, "$%.02x", nes->o_cpu->PRGROM[rom_offset]);
										strcat(line, dat_line);
									}
		                            in_raw_word = 0;
		                            break;
	                        }
	                        if((pos_data % 16) == 15)
	                        {
								if(repass < k)
								{
	                                sprintf(dat_line, "\n");
									strcat(line, dat_line);
									listing.Add(line);
									strcpy(line, "");
								}
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
							label = search_label(pages->bank_lo, pages->bank_hi, k, pages->alias, -1, 1);
                            label_type = label->type;
                            sub_t = label->sub_type;
	                        if((pos_data % 16) != 15)
	                        {
	                            if(label_type != TYPE_UNK ||
	                               k >= pages->address + pages->size)
	                            {
									if(repass < k)
									{
										sprintf(dat_line, "\n");
										strcat(line, dat_line);
										listing.Add(line);
										strcpy(line, "");
									}
	                            }
	                            else
	                            {
									if(repass < k)
									{
										sprintf(dat_line, ",");
										strcat(line, dat_line);
									}
	                            }
	                        }
	                        if(in_raw_word && label_type == TYPE_UNK)
	                        {
	                            sub_t = TYPE_RAWWORD;
	                        }
                            pos_data++;
                        }
						listing.Add(line);
                        break;
                }
            }
			for(j = 0; j < listing.Amount(); j++)
			{
				fprintf(out, listing.Get(j)->Content);
			}

			fclose(out);
            pages = pages->Next;
			if(pages)
			{
				rom_offset_glob = pages->rom_offset;
			}

			printf("Done.\n");
			if(warnings)
			{
				printf("CAUTION: Source contains one or more warnings !\n");
			}
        }
    }

    pages = nes->chr_pages;

    // dump the chr pages (if any)
    if(pages)
    {
        rom_offset_glob = pages->rom_offset;
        //nes->o_ppu->decode_chr_rom();
        for(i = 0; i < nbr_chr_pages; i++)
        {
            rom_offset = rom_offset_glob;

			sprintf(bank_name, "%s_%.03d_chr.asm", nes->Game_FileName, pages->bank_lo);
            out = fopen(bank_name, "wb");
            if(!out) break;
            
			printf("Generating \"%s\" bank file... ", bank_name);

			fprintf(out, "; Game name: %s\n", nes->Game_Name);
            fprintf(out, "; prg-rom: %d\n", infos->prg_pages);
            fprintf(out, "; chr-rom: %d\n", infos->chr_pages);
            fprintf(out, "; Mapper: %d\n", infos->mapper);
            fprintf(out, "; Mirroring: %d\n", infos->mirroring);
            fprintf(out, "; ------------------------------\n");
			fprintf(out, "; Disassembled with "APPNAME" "APPVERSION"\n");
            fprintf(out, "; ------------------------------\n\n");
            fprintf(out, "        .autoimport +\n\n");
            fprintf(out, "\n        .segment \"CHR_%d\"\n\n", pages->bank_lo);

            // Dump the complete chr bank
            k = pages->address;
			pos_data = 0;

			// Determine the size of the array of bytes to dump
            while(k < pages->address + pages->size)
            {
	            if((pos_data % 16) == 0)
	            {
                    fprintf(out, "        .byte ");
				}

		        fprintf(out, "$%.02x", nes->o_rom->ROM.read_byte(rom_offset));

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

            fclose(out);
            pages = pages->Next;
            if(pages)
			{
				rom_offset_glob = pages->rom_offset;
			}

			printf("Done.\n");
		}
    }

    // Generate the config file
    if(nbr_chr_pages || nbr_prg_pages)
    {
		printf("---------------------------------------------------------------------------------\n");
		printf("Misc. files generation:\n");

		// Generate the header file
        sprintf(bank_name, "%s_header.bin", nes->Game_FileName);
		printf("Dumping the ROM header as \"%s\" ... ", bank_name);
        out = fopen(bank_name, "wb");
        if(out)
        {
			fwrite ((void *) &nes->o_rom->HEADER[0], 1, 0x10, out);
			nes->dump_header(bank_name);
			fclose(out);
		}
		printf("Done.\n");

		sprintf(bank_name, "%s.cfg", nes->Game_FileName);
        out = fopen(bank_name, "wb");
        if(out)
        {
			printf("Generating \"%s\" file... ", bank_name);
			
			fprintf(out, "MEMORY {\n");
            pages = nes->prg_pages;
            if(nes->prg_pages)
            {
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "    PRG_%d: start = $%.04x, size = $%.04x;\n",
                                i,
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
                    fprintf(out, "    CHR_%d: start = $%.04x, size = $%.04x;\n",
                                i,
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
                    fprintf(out, "    PRG_%d: load = PRG_%d;\n",
                                i,
                                i
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
                    fprintf(out, "    CHR_%d: load = CHR_%d;\n",
                                i,
                                //pages->address,
                                i
                                );
                    pages = pages->Next;
                }
            }
            fprintf(out, "}\n");

			printf("Done.\n");
		}

		// Generate the batch file
        sprintf(bank_name, "%s.bat", nes->Game_FileName);
        out = fopen(bank_name, "wb");
        if(out)
        {
			printf("Generating \"%s\" file... ", bank_name);

            fprintf(out, "@echo off\n");
            if(nes->prg_pages)
            {
                pages = nes->prg_pages;
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "ca65.exe \"%s_%.03d_prg.asm\"\n", nes->Game_Name, i);
                    fprintf(out, "if errorlevel 1 goto end\n");
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "ca65.exe \"%s_%.03d_chr.asm\"\n", nes->Game_Name, i);
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
                    fprintf(out, " \"%s_%.03d_prg.o\"", nes->Game_Name, i);
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, " \"%s_%.03d_chr.o\"", nes->Game_Name, i);
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
            fprintf(out, "del \"%s.prg\"\n", nes->Game_Name);
            if(nes->prg_pages)
            {
                pages = nes->prg_pages;
                for(i = 0; i < nbr_prg_pages; i++)
                {
                    fprintf(out, "del \"%s_%.03d_prg.o\"\n", nes->Game_Name, i);
                    pages = pages->Next;
                }
            }
            if(nes->chr_pages)
            {
                pages = nes->chr_pages;
                for(i = 0; i < nbr_chr_pages; i++)
                {
                    fprintf(out, "del \"%s_%.03d_chr.o\"\n", nes->Game_Name, i);
                    pages = pages->Next;
                }
            }
            fprintf(out, ":end\n");
            
            fclose(out);

			printf("Done.\n");
		}
    }

	if (head)
	{
    
		c_tracer writer (nes->Labels_Name);
		writer.f_write ("s", "Offset,Real,Bank,Address,Type,Access,Jump,RefBank\r\n");

		s_label_node *navigator = head;

		while(navigator != NULL)
		{
			writer.f_write ("lsnsnsnsssnsns",
							navigator->offset,
							",",
							navigator->real_bank,
							",",
							navigator->alias,
							",",
							navigator->contents,
							navigator->type == TYPE_CODE ? ",CODE" : ",DATA",
							navigator->type == TYPE_CODE ? 
								navigator->sub_type == TYPE_RELCODE ? ",RELC" : ",CODE" :
									navigator->sub_type == TYPE_BYTE ? ",BYTE" : 
										navigator->sub_type == TYPE_WORD ? ",WORD" : ",RAWW",
							",",
							navigator->jump_base_table,
							",",
							navigator->ref_bank,
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

void c_label_holder :: insert_label (__UINT_16 value, e_dattype type, e_dattype sub_type, int force, int base, int ref_bank)
{
    int bank_alias;
    int bank_num;
	__UINT_8 bank = nes->o_mapper->get_real_prg_bank_number (value);

	if(ref_bank == -1) ref_bank = bank;

	if (!head)
	{
        bank_num = bank;
        bank_alias = nes->BankJMPList->get_bank_alias(bank_num, value);
		head = nes->o_mapper->create_label (head, value, type, sub_type, base, nes->o_cpu->get_rom_offset(value), ref_bank, 
                                            bank_num, bank_alias);
		return;
	}
	
	s_label_node *navigator = head;
    s_label_node *PrevNode = head;

	__BOOL is_found = FALSE;

	while ((NULL != navigator) && !is_found)
	{
		PrevNode = navigator;
		if ((navigator->address == value) &&
			(navigator->ref_bank == ref_bank))
	    {
	        is_found = TRUE;
	        break;
        }
		navigator = navigator->Next;
	}
	
	if(is_found)
	{
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
					navigator->sub_type = sub_type;
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
					navigator->sub_type = sub_type;
				}
			}
			return;
		}
	}
	else
	{
        bank_num = bank;
        bank_alias = nes->BankJMPList->get_bank_alias(bank_num, value);
		PrevNode->Next = nes->o_mapper->create_label (head, value, type, sub_type, base, nes->o_cpu->get_rom_offset(value), ref_bank,
                                                      bank_num, bank_alias);
	}
}

// Return 0 if label has been found.
int c_label_holder :: insert_label_bank (__UINT_8 bank,
										 __UINT_16 value,
										 e_dattype type,
										 e_dattype sub_type,
										 int force,
										 int base,
										 int ref_bank,
										 int offset,
										 int old_bank,
										 int jmp_pos)
{
	if (!head)
	{
        head = nes->o_mapper->create_label (head, value, type, sub_type, base, offset,
                                            old_bank == -1 ? bank : old_bank,
                                            bank, get_bank_alias(bank, value));
        if(head == NULL)
        {
		    return 0;
        }
		return 1;
	}
	
	s_label_node *navigator = head;
    s_label_node *PrevNode = head;

	__BOOL is_found = FALSE;

	while ((NULL != navigator) && !is_found)
	{
		PrevNode = navigator;
		if ((navigator->contents == value)
			&& (get_bank_alias(navigator->ref_bank, value) == ref_bank)
		   )
	    {
	        is_found = TRUE;
	        break;
        }
		navigator = navigator->Next;
	}
	
	if(is_found)
	{
		if ((navigator->contents == value) &&
			(navigator->offset == offset))
		{
			if(navigator->sub_type != TYPE_WORD || type == TYPE_CODE)
			{
				if(force)
				{
					if(navigator->sub_type != TYPE_WORD && navigator->sub_type != TYPE_RAWWORD || type == TYPE_CODE)
					{
						navigator->type = type;
						navigator->sub_type = sub_type;
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
						navigator->sub_type = TYPE_CODE;
						navigator->jump_base_table = base;
						if(jmp_pos != -1 &&
						   (old_bank == navigator->real_bank))
						{
							if(jmp_pos > value)
								return -1;
						}
					}
				}
			}
			return 0;
		}
	}
	else
	{
		PrevNode->Next = nes->o_mapper->create_label (head, value, type, sub_type, base, offset,
                                                      old_bank == -1 ? bank : old_bank,
                                                      bank, ref_bank);
        if(PrevNode->Next == NULL)
        {
            return 0;
        }
        return 1;
	}
    return 0;
}
