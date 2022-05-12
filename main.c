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
#define R39
//#define DEBUG

#include <stdio.h>
#include <stdint.h>
#include <conio.h>     // waitvsync()
#include <cbm.h>		// VERA.xxxx macros
#include "song.h"
#ifdef R39
#include "ymlookup35.h"
#else
#include "ymlookup.h"
#endif

struct YM2151_t {
    uint8_t reg;
    uint8_t dat;
};

struct xlate_t {
	uint8_t mask;
	uint8_t base;
	uint8_t shift;
};
#ifndef R39
#define YM (*(volatile struct YM2151_t*) 0x9fe0)
#else
#define YM (*(volatile struct YM2151_t*) 0x9f40)
#endif

#define songend (uint8_t *)(song + sizeof(song))

uint8_t YMshadow[256];
uint8_t *songpos;
uint8_t reg, val, perframe;
uint8_t oplkeys;     // shadow the KeyON bits for the 8 usable voices
uint16_t delay, i;
uint16_t oplfreq[8]; // shadow regs for the OPL frequencies

uint8_t debug = 0;
uint8_t fracframe = 0;

// assuming that channel 0 is unused in IMF music
// produced by Id Software, as they usu. reserved that
// channel as a dedicated SFX channel. This leaves 1-9
// for translation to the OPM.
const uint8_t YMoper[32] = { // maps OPL operators onto OPM operators
	0xff, 0, 1, 0xff, 16, 17, 0xff, 0xff,
	2, 3, 4, 18, 19, 20, 0xff, 0xff,
	5, 6, 7, 21, 22, 23, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const uint8_t YMvoice[32] = { // maps OPL operators onto OPM voices
	0xff, 0, 1, 0xff, 0, 1, 0xff, 0xff,
	2, 3, 4, 2, 3, 4, 0xff, 0xff,
	5, 6, 7, 5, 6, 7, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

void ymwrite(uint8_t reg, uint8_t data)
{
//	if (debug)
//		cprintf("ymwrite: 0x%02x <-- 0x%02x\n\r",reg,data);
	while (YM.reg & 0x80) { }; // wait for busy flag to be clear
	YM.reg = reg;
	YM.dat = data;
};

void YMtestpatch()
{
	// load the YM with the patches from my STARDUST.PRG demo

	uint8_t o,r,v;

	volatile const uint8_t val[4][6] = {
		{0x31,0x07,0x1f,0x09,0x00,0xff},
		{0x31,0x11,0x1f,0x09,0x00,0xff},
		{0x31,0x08,0x1f,0x09,0x00,0xff},
		{0x30,0x24,0x1f,0x09,0x00,0xff}
	};

	for (v=0 ; v<8 ; v++)
	{
		ymwrite	(0x20+v,0xe4);
		ymwrite (0x38+v,0);
		for (o=0;o<4;o++)
		{
			for (r=0;r<6;r++)
			{
				ymwrite(0x40+0x20*r+0x08*o+v,val[o][r]);
			}
		}
	}
}

static void vload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	//cbm_k_setlfs(2,1,0);  // original values from ?R32? ver. of Flappy Bird
	//cbm_k_setlfs(0,8,0);  // this works with the emulator Host FS
	cbm_k_setlfs(0,8,0);
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

#ifdef R39
#define time (*(uint8_t *) 0xa03f)
#define rambank (*(uint8_t *) 0x0000)

/* This was needed in pre-release R39 as cc65 waitvsync() was broken...
 * static void myvsync()
{
	uint8_t b,j;
	b = rambank;
	rambank = 0;
	j = time;
	while (time==j) {};
	rambank=b;
}
*/

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
	vload("pal.bin",1,0xfa00);
	vload("title.bin",0,0x0000);
	VERA.display.video		= 0x11; // hi-nib=layer0 ena, lo=VGA output


}
#endif

int8_t opl2ym (unsigned char reg, unsigned char data)
{
	/* This is the translation function to map OPL commands onto
	 * equivalent YM commands (where possible) and also does a few
	 * tricks to simulate behaviors that aren't 1:1 feature equivalent
	 * such as the "no-sustain" mode of OPL.
	 *
	 * This code could be tightened up to reduce the number of writes
	 * to the YM, but as this is just a demo, speed is not of the
	 * essence, and it might be faster anyway to just blindly dump the
	 * translations into the YM w/o checking whether they're redundant
	 * or not. Only the Freq/KeyON registers are checked like this.
	 */

	uint8_t adv;       // AdLib voice #
	uint8_t op, ch, s; // YM Operator, YM Channel, Shadow Register indx.
	uint8_t key;	   // New state of OPL keyON flag (if 0xb0 written)
	uint16_t freq;
	uint8_t keyonoff, block;

// -------------------------------------- debug
//	if (reg < 0xb0 || reg >= 0xb9)
//		return 0;
// -------------------------------------- debug

	// check the global registers first because 0xbd is right smack
	// in the middle of the keyon register range
	if (reg < 0x20 || reg == 0xbd )
	{
		switch (reg)
		{
			case 0x01:
			{
				// nop for now but if there's anything to
				// be done regarding waveform control, here
				// is where the feature is activated
				//cprintf ("Waveform Control set %d\n",(data&0x20)>>5);
				return (-1); // not yet supported....
				break;
			}
			case 0x08: // speech synth mode flag / keysplit note select
			{
				//  the speech synth mode mystifies even the gurus of old
				//	printf("Note Select set\n");
			}
			case 0x02: // timer 1 data
			case 0x03: // timer 2 data
			case 0x04: // timer/IRQ control flags
			case 0xbd: // AM Depth / Vibrato / Rhythm Control
			default:
			{
				return (-1); // (currently) unsupported register
				break;
			}
		};
		return(0);
	};

	// By now, the register is one of the per-op/per-channel registers...
	// only one of these interpretations is going to be valid, depending
	// on which register group is being written to, but go ahead
	// and look up the YM-mapped versions for either case
	op = YMoper[reg & 0x1f];
	ch = YMvoice[reg & 0x1f];	// This maps an OPL operator to a
								// YM channel I think it's unused for now
								// because the per-voice ranges use
								// OPL voice# as the index, not Oper#'s
	adv = (reg & 0x0f);	// adv = AdLib Voice #
	if (debug) {
	  cprintf("\n\ropl2ym: %02x %02x\n\r",reg, data);
	  cprintf(" - op %02x\n\r - ch %02x\n\r",op,ch);
	  cprintf(" -adv %02x\n\r",adv);
	};

	switch (reg & 0xe0) // mask the reg to just the range values
	{

		// translate per-operator settings
		           //   7     6   5    4    3..0
		case 0x20: // |Trem|Vibr|Sust|KSR |Multiplier|
		{
			if ( op == 0xff )
				return -1;

			// handle bit 5 (Sus mode) 1=notes sustain, 0=no sustain
			s = 0xc0 + op;  // 0xc0 is DT2|D2R(bits 0-5) on OPM
			if (data & 0x20) // if bit 5 is set in the value....
			{
				YMshadow[s]=0; // D2R=0. Clobber DT2 (not feat. of OPL)
			}
			else
			{
				// Use OPM D2R to mimic always-release mode of OPL
				// - copy the RR into D2R. (OPL has no D2R, so this works)
				YMshadow[s]=(YMshadow[s+0x20] & 0x0f) << 1;
			}
			ymwrite(s,YMshadow[s]);

			// ignore KS bit (4) for now. This effect not native to YM.

			// use LFO to do Vibrato effect
			// TODO: Vibrato effects

			// handle bits 0-3 (MUL) - just write them to YM
			s = 0x40 + op;
			YMshadow[s] = (YMshadow[s] & 0xf0) | ( data & 0x0f );
			ymwrite(s,YMshadow[s]);
			break;
		}
		case 0x40: // KS (2MSB) + TL (6LSB)
		{
			s = 0x80 + op; // YM 0x80 = KS(2msb) and AR (5lsb)
			YMshadow[s] = (YMshadow[s] & (0xc0 ^ 0xff)) | (data & 0xc0);
			ymwrite(s, YMshadow[s]);
			s = 0x60 + op; // YM 0x60 = TL (bits 0-6)
			//YMshadow[s] = ((data & 0x3f)<<1);
			YMshadow[s] = data & 0x3f; // try it not shifting left
			ymwrite(s, YMshadow[s]);
			break;
		}
		case 0x60: // AR+DR (4bits each)
		{
			s = 0x80+op;
			YMshadow[s] = (YMshadow[s] & 0xe0) | ((data & 0xf0)>>3);
			ymwrite(s, YMshadow[s]);
			s = 0xa0+op;
			YMshadow[s] = (YMshadow[s] & 0xe0) | ((data & 0x0f)<<1);
			ymwrite(s, YMshadow[s]);
			break;
		}
		case 0x80: // SL+RR (4 bits each) - OPM is an exact match!!!
		{
			s = 0xe0 + op;
			YMshadow[s] = data;
			ymwrite(s, data);
			break;
		}
		case 0xa0: // Freq. & Keyon
		{
			if (debug)
				cprintf("looking for changes to KeyON & Freq\n\r");
			if (adv > 8 || adv < 1) // NOP for invalid voice #s
				return -1;
			adv--;
			// load shadow freq/keycodes
			freq = oplfreq[adv];
			key  = oplkeys;
			if (debug)
				cprintf("--f0=%04x k0=%02x\n\r",freq,key);
			if (reg >= 0xb0)
			{
				freq = (freq & 0x00ff) | ((data & 0x1f) << 8);
				key  = key & (0xff ^ (0x01 << adv));
				key |= ((data & 0x20) >> 5) << adv;
			}
			else
			{
				freq = (freq & 0xff00) | data;
			}
			if (debug)
				cprintf("--f1=%04x k1=%02x\n\r",freq,key);
			if ( freq != oplfreq[adv] )
			{
				// convert OPL freq to OPM KS values (grrrrrr!)
				oplfreq[adv] = freq;
				block = (freq & 0x1c00) >> 10;
				freq &= 0x03ff; // remove 'block' value from freq
				freq = flut[block][freq];
				// freq is now the YM KC value - thanks LUT!
				s = 0x28 + adv;
				YMshadow[s] = freq;
				ymwrite(s,freq);

			};
			if ( key != oplkeys ) { // the keyon bit changed state
				// translate the keyon bit to the 2 OPs in the
				// YM KeyON register. (it has a keyON flag for each
				// operator in the voice)
				keyonoff  = (data & 0x20) >> 1;
				keyonoff |= (data & 0x20) >> 2;
				if (debug)
				{
					cprintf("KeyUPDN event for voice %d\n\r",adv);
					cprintf("--%02x\n\r",keyonoff);
					cprintf("--YM[08] <-- %02x\n\r",keyonoff+adv);
					while (!kbhit()) {}
					cgetc();
				}
				YMshadow[8] = keyonoff + adv;
				ymwrite(8,YMshadow[8]);
			};
			oplkeys = key;
			break;
		}
		case 0xc0: // D6&7 are OPL3, D5=R, D4=L, D3-1=FB, D0=Synth type
		{
			if (adv > 8 || adv < 1)
				return -1;
			// Bits 1-5 map to the OPM 0x20 bits 3-7
			// synth type maps to OPM ALG (0 = FM(4) 1 = AM(7))
			s=0x20 + adv-1;
			// writes RL + FB + CONNECT
			//YMshadow[s] = (data & 0x1e) << 3;
			//YMshadow[s] += 4 + 3 * (data & 0x01);
			YMshadow[s] = (YMshadow[s] & 0xc0) | ((data & 0x0e) << 2);
			YMshadow[s] += 4 + 3 * (data & 0x01); // ALG= 4 or 7
			ymwrite(s,YMshadow[s]);
			break;
		}
		case 0xe0:
		{
			//cprintf ("Waveform Select: 0x%02x <-- 0x%02x\n\r",reg,data);
			break;
		}
		default:
		{
			break;
		}
	}; // switch(reg & 0xe0)
};


void main()
{
	char k[8] = {0,0,0,0,0,0,0,0};

	// init the YM shadow registers and use them to init the YM chip
	for ( i=0 ; i < 256 ; i++)
	{
		YMshadow[i]=0x00;
	}
	// init some basic YM stuff here..
	//
	YMshadow[0x20] = 0xc4; // L&R audio ON, Connect alg 4 (most like OPL)
	YMshadow[0x21] = 0xc4; // ... set all 8 voices like this
	YMshadow[0x22] = 0xc4;
	YMshadow[0x23] = 0xc4;
	YMshadow[0x24] = 0xc4;
	YMshadow[0x25] = 0xc4;
	YMshadow[0x26] = 0xc4;
	YMshadow[0x27] = 0xc4;

	// hack - just to get some noise: put concert A in all 8 voices
	/*
	for (i = 0 ; i < 8 ; i++ )
	{
		YMshadow[0x21+4*i] = 10 + 16 * 4;
	}
	*/

	// write the YMshadow out to the real YM.
	for (i=0 ; i < 256 ; i++ )
	{
		YM.reg = i;
		YM.dat = YMshadow[i];
	};

	// initialize the OPL shadow values for freq/KeyON states
	for ( i=0 ; i < sizeof(oplfreq) ; i++)
	{
		oplfreq[i] = 0;
	}
	oplkeys = 0;

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

	//opl2ym (0x08,0x40); // Note-Split = 1 (does nothing RN tho)
	cprintf ("&oplfreq = 0x%04x (in main)\n\r",&oplfreq);
	cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
	opl2ym (0xa1,0x41);
	cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
	while (1) {
//		ymwrite (0x08,0x00);
//		ymwrite (0x08,0x78);
		while (!kbhit()) {};
		cgetc();
		opl2ym (0xb1,0x12);
		cprintf ("freq = 0x%04x\n\r",oplfreq[0]);
		opl2ym (0xb1,0x32);
		cprintf ("freq = 0x%04x\n\r",oplfreq[0]);

    }
 */

/* -- this code should just play a tone of quickly-rising pitch
	opl2ym(0xb1,0x14);
	opl2ym(0xb1,0x34);
    for (i=0 ; i<1024 ; i++)
    {
		opl2ym(0xa1,i&0xff);
		if (i > 255)
			opl2ym(0xb1,0x34 | ((i&0x30)>>8));
		for (delay=0 ; delay <= 65000 ; delay++) {}
	}

	while (1) {};
*/

#ifndef DEBUG
	setBG();
#endif

	songpos = (uint8_t*)&song;

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
#ifdef R39
				cprintf("R39\n\r");
#endif
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
				waitvsync();

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
				songpos = (uint8_t*)&song;
			}
			opl2ym(reg,val);
		}

	};
}
