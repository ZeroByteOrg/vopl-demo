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
#include "fileio.h"
#include "wolf3d_resources.h"

uint16_t i;

#ifndef DEBUG
uint8_t debug = 0;
#else
uint8_t debug = 1;
#endif

extern void __fastcall__ vsync();

static void vload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,0);
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

#if(0)
// switch to this so the graphics don't need that stupid PRG header anymore!
static void bvload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,2);        // SA=2 denotes "headerless" mode loading
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}
#endif

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

char songlist[11]={
  NAZI_NOR_MUS, WONDERIN_MUS, GETTHEM_MUS, ENDLEVEL_MUS, SEARCHN_MUS,
  CORNER_MUS, GETOUT_MUS, POW_MUS, SUSPENSE_MUS, URAHERO_MUS, ROSTER_MUS
};


void main()
{
  char active_song = 1;
	char k[8] = {0,0,0,0,0,0,0,0};

	player_init();
	printf("player initialized.\n");
  build_song_index();
	printf("song index built.\n");
	debug = 0;

  RAM_BANK = 1;
	printf("loading song...");
  load_chunk(songlist[active_song],(void*)0xa000);
	printf(" done\n");
  RAM_BANK = 1;
  if (chunkEnd.ok)
    player_start(1,(void*)0xa000,chunkEnd.bank,(void*)chunkEnd.addr);
#ifndef DEBUG
	setBG();
#endif
  while(kbhit()) {cgetc();}
	while (!kbhit()) {
#ifdef DEBUG
    vsync();
    printf("advancing the music\n");
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
    player_advance();
#else
    if(kbhit()) {
      cgetc();
      if(++active_song > sizeof(songlist)) active_song = 0;
      player_stop();
      load_chunk(songlist[active_song],(void*)0xa000);
      if(chunkEnd.ok)
        player_start(1,(void*)0xa000,chunkEnd.bank,(void*)chunkEnd.addr);
    }
#endif
  }
  player_shutdown();
}
