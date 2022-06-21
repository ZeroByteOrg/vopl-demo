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
//#include "song.h"

#include "wolf3d_resources.h"


struct xlate_t {
	uint8_t mask;
	uint8_t base;
	uint8_t shift;
};

//#define songend (uint8_t *)(song + sizeof(song))

extern void __fastcall__ vsync(); // because cc65's keeps breaking...

extern int8_t vopl_write(unsigned char reg, unsigned char data);
extern void vopl_init();

extern uint16_t load_chunk(uint8_t index, void* buffer);
extern void build_song_index();


uint8_t *songpos;
uint8_t *songend;
uint8_t reg, val, perframe;
uint16_t delay, i;

uint8_t debug = 0;
uint8_t fracframe = 0;




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


  vopl_init();
  build_song_index();
/*
// -------------------- debug:
	YMtestpatch(); // set some patches we know will work....
	for (i=0 ;  i < 8 ; i++)
	{
		ymwrite(0x21+4*i,10+16*4); // concert A to KeyCode regs
	};
//----------------------------
*/


	debug = 0;

/* -- Debug loop that plays a note on the emulated OPL once per
 * -- keypress. Must have

	//vopl_write (0x08,0x40); // Note-Split = 1 (does nothing RN tho)
	cprintf ("&oplfreq = 0x%04x (in main)\n\r",&oplfreq);
	cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
	vopl_write (0xa1,0x41);
	cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
	while (1) {
//		ymwrite (0x08,0x00);
//		ymwrite (0x08,0x78);
		while (!kbhit()) {};
		cgetc();
		vopl_write (0xb1,0x12);
		cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
		vopl_write (0xb1,0x32);
		cprintf ("freq = 0x%04x\n\r",oplfreq[0]);

    }
 */

/* -- this code should just play a tone of quickly-rising pitch
	vopl_write(0xb1,0x14);
	vopl_write(0xb1,0x34);
    for (i=0 ; i<1024 ; i++)
    {
		vopl_write(0xa1,i&0xff);
		if (i > 255)
			vopl_write(0xb1,0x34 | ((i&0x30)>>8));
		for (delay=0 ; delay <= 65000 ; delay++) {}
	}

	while (1) {};
*/

  //songpos = (uint8_t*)&song;
  //songend = (uint8_t *)(song + sizeof(song))
  RAM_BANK = 1;
  songpos = (uint8_t*)0xa000;
  songend = songpos + load_chunk(WONDERIN_MUS,songpos);
  RAM_BANK = 1;
#ifndef DEBUG
	setBG();
#endif


	perframe = 0;
	delay = 0;
	while (1)
	{
#ifdef DEBUG
		if (kbhit()) {
			if (debug) {
				debug = 0;
			}
			else
			{
				debug = 1;
			}
			cgetc();
		}
#endif
		if (delay>0)
		{
			delay--;
			++perframe;
			if(((perframe == 11) && (fracframe!=2)) || (perframe>11))
			{
				fracframe = (fracframe+1) % 3;

				perframe=0;
				// draw little arrows to show channel activity....
#ifdef DEBUG
				clrscr();
				for (i = 0 ; i < 8 ; i++ )
				{
					if (oplkeys & (0x1<<i))
					{
						cprintf("^ ");
					}
					else
					{
						cprintf(". ");
					};
				};
#endif
				//waitvsync();
        vsync();

			};
		}
		else
		{
			reg = songpos[0];
			val = songpos[1];
			delay  = songpos[2] & 0xff;
			delay |= songpos[3] << 8;
			songpos += 4;
			if (songpos >= songend) {
				// todo: find a way to eliminate noise during loop reset
				//songpos = (uint8_t*)&song;
        songpos = (uint8_t*)0xa000;
			}
			vopl_write(reg,val);
		}

	};
}
