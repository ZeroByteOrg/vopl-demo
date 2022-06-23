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
#include <conio.h>
#include <cbm.h>		// VERA.xxxx macros
#include <errno.h>
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

static void setBG()
{
	//int i;

	textcolor(COLOR_GRAY2);  // this color works well for transparent debug overlay
	videomode(VIDEOMODE_40x30);


	VERA.layer0.config		= 0x07; // MapH&W=0,Bitmap=1,depth=3
	VERA.layer0.hscroll		= 0;    // sets palette offset in bitmap mode
	VERA.layer0.tilebase	= 0x00;
	VERA.layer1.config   |= 0x08; // enable 256-color text mode
  VERA.display.video		= 0x31; // hi-nib=layer0 ena, lo=VGA output

	gotoxy (15,15);
	cprintf ("loading");
	vload("pal.bin",1,0xfa00);
	vload("title.bin",0,0x0000);
	clrscr();
	gotoxy(0,29);

}

char songlist[11]={
  NAZI_NOR_MUS, WONDERIN_MUS, GETTHEM_MUS, ENDLEVEL_MUS, SEARCHN_MUS,
  POW_MUS, SUSPENSE_MUS, GETOUT_MUS, URAHERO_MUS, ROSTER_MUS, CORNER_MUS,
};

void play_song(char id) {
	if (id > sizeof(songlist)) return;
	player_stop();
	printf("Loading song %d: \"%s\"",id,SONGNAME[songlist[id]]);
	load_chunk(songlist[id],(void*)0xa000);
	printf("%u bytes loaded. Done\n\n",chunkEnd.bytes);
	if(chunkEnd.ok)
		player_start(1,(void*)0xa000,chunkEnd.bank,(void*)chunkEnd.addr);
	else printf ("An error occured. Status byte = %02x\n",_oserror);
}

void printhelp() {
	printf("Debug Console Commands:\n");
	printf("CSR R: Next Song\n");
	printf("CSR L: Previous Song\n");
	printf("ENTER: Stop/Start music\n");
	printf("   F1: toggle console visibility\n");
	printf("    C: clear the debug console\n");
	printf("    Q: Quit\n\n");
}

void main()
{
  char active_song = 0;
	char k[8] = {0,0,0,0,0,0,0,0};
	char key;

	player_init();
	printf("player initialized.\n");
  build_song_index();
	printf("song index built.\n");
	debug = 0;
	play_song(active_song);
	setBG();
  while(kbhit()) {cgetc();} // clear the input buffer.
	VERA.display.video &= ~(1<<5);
	printf("Loading song %d: \"%s\"\n",active_song,SONGNAME[songlist[active_song]]);
	printf("%u bytes loaded. Done\n\n",chunkEnd.bytes);
	printf("Debug console ready.\n");
	printf("Press ? for help\n");
	VERA.display.video |= (1<<5);
	while (1) {

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
      key=cgetc();
			if (key=='q') break;
			switch (key) {
				case CH_F1:
					VERA.display.video ^= (1<<5); // toggle debug overlay
					break;
				case '?':
					VERA.display.video |= (1<<5); // make sure the overlay is visible
					printhelp();
					break;
				case 'c':
				case 'C':
					if (VERA.display.video & (1<<5)) {
						clrscr();
						gotoxy(0,29);
						printf("Debug console ready.\n");
					}
					break;
				case CH_ESC:
					break;
				case CH_CURS_UP:
					// speed up the music
				case CH_CURS_DOWN:
					// slow down the music
					break;
				case CH_CURS_LEFT:
					if (--active_song >=sizeof(songlist)) active_song = sizeof(songlist)-1;
					play_song(active_song);
					break;
				case CH_CURS_RIGHT:
				case ' ':
					if(++active_song >= sizeof(songlist)) active_song = 0;
					play_song(active_song);
					break;
				case CH_ENTER:
					if (player_isplaying()) {
						player_stop();
						printf("Music stopped.\n");
					}
					else
						play_song(active_song);

			}
    }
#endif
  }
	VERA.display.video |= (1<<5);
	if (player_isplaying()) {
		while (kbhit()) cgetc(); // consume any buffered key presses.
		printf("Leave the music running (y/n)?");
		while (1) {
			key=cgetc();
			switch (key) {
				case 'y':
				case 'Y':
					printf("\nUse SYS $%04x to stop the music later.\n",(uint16_t)&player_shutdown);
					return;
				case 'n':
				case 'N':
				case CH_ESC:
					player_shutdown();
					return;
			}
		}
	}
  player_shutdown();
}
