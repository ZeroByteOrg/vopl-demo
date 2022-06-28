// This is the Virtual OPL (VOPL) code that translates OPL writes into
// best-fit YM2151 equivalents. The biggest single assumption is that one of
// the OPL voices is ignored, as it has 9 while OPM only has 8.
//
// For the purposes of this demo, it is hard-wired to ignore OPL voice 0
// and map voices OPL:1-8 onto OPM:0-7 due to the fact that iDsoft dedicated
// voice 0 to SFX playback, and the primary function of this demo is to
// play back the IMF music from Wolfenstein3d.

// The goal here is to eventually whittle all of this away into being a
// set of routines in pure assembly.

#define FCONVERT

#include <stdint.h>

#ifdef DEBUG
#include <stdio.h>    // for the debug-related IO
#include <conio.h>
// in lieu of project H files, just declare the stuff we intend to use:
extern uint8_t debug;
#endif

extern uint8_t YMshadow[256]; // shadow the YM state
extern void ym_init();
extern void ym_silence();
extern void ym_write(uint8_t reg, uint8_t data);

#ifdef FCONVERT
extern uint16_t __fastcall__ fconvert(uint16_t blockfnum);
#endif

uint8_t oplkeys;       // shadow the KeyON bits for the 8 usable voices
uint16_t oplfreq[8];   // shadow regs for the OPL frequencies

#ifndef FCONVERT
#include "ymlookup.h" // contains a definition, so make sure this
                      // falls after all declarations (cc65 optimization)
#endif


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


int8_t vopl_write (unsigned char reg, unsigned char data)
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
#ifdef DEBUG
	if (debug) {
	  printf("\n\ropl2ym: %02x %02x\n\r",reg, data);
	  printf(" - op %02x\n\r - ch %02x\n\r",op,ch);
	  printf(" -adv %02x\n\r",adv);
	};
#endif
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
			ym_write(s,YMshadow[s]);

			// ignore KS bit (4) for now. This effect not native to YM.

			// use LFO to do Vibrato effect
			// TODO: Vibrato effects

			// handle bits 0-3 (MUL) - just write them to YM
			s = 0x40 + op;
			YMshadow[s] = (YMshadow[s] & 0xf0) | ( data & 0x0f );
			ym_write(s,YMshadow[s]);
			break;
		}
		case 0x40: // KS (2MSB) + TL (6LSB)
		{
			s = 0x80 + op; // YM 0x80 = KS(2msb) and AR (5lsb)
			YMshadow[s] = (YMshadow[s] & (0xc0 ^ 0xff)) | (data & 0xc0);
			ym_write(s, YMshadow[s]);
			s = 0x60 + op; // YM 0x60 = TL (bits 0-6)
			//YMshadow[s] = ((data & 0x3f)<<1);
			YMshadow[s] = data & 0x3f; // try it not shifting left
			ym_write(s, YMshadow[s]);
			break;
		}
		case 0x60: // AR+DR (4bits each)
		{
			s = 0x80+op;
			YMshadow[s] = (YMshadow[s] & 0xe0) | ((data & 0xf0)>>3);
			ym_write(s, YMshadow[s]);
			s = 0xa0+op;
			YMshadow[s] = (YMshadow[s] & 0xe0) | ((data & 0x0f)<<1);
			ym_write(s, YMshadow[s]);
			break;
		}
		case 0x80: // SL+RR (4 bits each) - OPM is an exact match!!!
		{
			s = 0xe0 + op;
			YMshadow[s] = data;
			ym_write(s, data);
			break;
		}
		case 0xa0: // Freq. & Keyon
		{
			// The formula to convert OPL BLOC:FNUM --> OPM KC KF values:
			// val=12 * (log2(fnum) + bloc - 8.5132730792261496510681735326167
			// INT part * 4/3 = KC. Frac part = KF
#ifdef DEBUG
			if (debug)
				printf("looking for changes to KeyON & Freq\n\r");
#endif
			if (adv > 8 || adv < 1) // NOP for invalid voice #s
				return -1;
			adv--;
			// load shadow freq/keycodes
			freq = oplfreq[adv];
			key  = oplkeys;
#ifdef DEBUG
			if (debug)
				printf("--f0=%04x k0=%02x\n\r",freq,key);
#endif
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
#ifdef DEBUG
			if (debug)
				printf("--f1=%04x k1=%02x\n\r",freq,key);
#endif
			if ( freq != oplfreq[adv] )
			{
				// convert OPL freq to OPM KS values (grrrrrr!)
				oplfreq[adv] = freq;
#ifdef FCONVERT
				freq = fconvert(freq);
				s = 0x28 + adv;
				YMshadow[s] = freq & 0xff;
				ym_write(s,freq & 0xff);
				YMshadow[s+8] = freq >> 8;
				ym_write(s+8,freq >> 8);
#else
				block = (freq & 0x1c00) >> 10;
				freq &= 0x03ff; // remove 'block' value from freq
				freq = flut[block][freq];
				// freq is now the YM KC value - thanks LUT!
				s = 0x28 + adv;
				YMshadow[s] = freq & 0xff;
				ym_write(s,freq);
#endif

			};
			if ( key != oplkeys ) { // the keyon bit changed state
				// translate the keyon bit to the 2 OPs in the
				// YM KeyON register. (it has a keyON flag for each
				// operator in the voice)
				keyonoff  = (data & 0x20) >> 1;
				keyonoff |= (data & 0x20) >> 2;
#ifdef DEBUG
				if (debug)
				{
					printf("KeyUPDN event for voice %d\n\r",adv);
					printf("--%02x\n\r",keyonoff);
					printf("--YM[08] <-- %02x\n\r",keyonoff+adv);
					while (!kbhit()) {}
					cgetc(); // consume the keystroke
				}
#endif
				YMshadow[8] = keyonoff + adv;
				ym_write(8,YMshadow[8]);
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
			ym_write(s,YMshadow[s]);
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

void vopl_silence() {
	ym_silence();
	oplkeys = 0;
}

void vopl_init() {
  uint8_t i;

  ym_init();
  // initialize the OPL shadow values for freq/KeyON states
  for ( i=0 ; i < 8 ; i++)
  {
    oplfreq[i] = 0;
  }
  oplkeys = 0;
}
