/*
 * main.c
 *
 * Copyright 2021 Barry Yost <thatguy@zerobyte.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */
//#define DEBUG

#include <stdio.h>
#include <stdint.h>
#include <conio.h>     // waitvsync()
#include <cbm.h>		// VERA.xxxx macros
#include "player.h"
#include "wolf3d_resources.h"

extern uint16_t load_chunk(uint8_t index, void* buffer);
extern void build_song_index();

uint16_t i;

uint8_t debug = 0;

musicframe chunk_end;

static void vload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,0);
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

// switch to this so the graphics don't need that stupid PRG header anymore!
static void bvload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,2);        // SA=2 denotes "headerless" mode loading
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

#ifndef DEBUG
static void setBG()
{

//#define _Vbgbase (((_0x4000 >> 2)|(_1 << 14)) & 0xffff)

	// set display to 320x240
	VERA.display.hscale		= 64;
	VERA.display.vscale		= 64;

	VERA.layer0.config		= 0x07; // MapH&W=0,Bitmap=1,depth=3
	VERA.layer0.hscroll		= 0;    // sets palette offset in bitmap mode
	VERA.layer0.tilebase	= 0x00;
  VERA.display.video		= 0x11; // hi-nib=layer0 ena, lo=VGA output
	vload("pal.bin",1,0xfa00);
	vload("title.bin",0,0x0000);


}
#endif


void main()
{
	char k[8] = {0,0,0,0,0,0,0,0};

	player_init();
	printf("player initialized.\n");
  build_song_index();
	printf("song index built.\n");
	debug = 0;

  RAM_BANK = 1;
	printf("loading song...");
  load_chunk(SEARCHN_MUS,(void*)0xa000);
	songEOF = chunk_end;
	printf(" done\n");
  RAM_BANK = 1;
#ifndef DEBUG
	setBG();
#endif
	player_start();
	while (1) {}
}
