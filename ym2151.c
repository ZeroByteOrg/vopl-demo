#include <stdint.h>

struct YM2151_t {
    uint8_t reg;
    uint8_t dat;
};
#define YM (*(volatile struct YM2151_t*) 0x9f40)


uint8_t YMshadow[256]; // shadow the YM state


void ym_write(uint8_t reg, uint8_t data)
{
//  if ((reg != 0x08) && (reg != 0x01)) {
//    if (reg == 0x19) (YMshadow[0x1a]=data);
//    else YMshadow[reg]=data;
//  }
	while (YM.dat & 0x80) { }; // wait for busy flag to be clear
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

  volatile const char* canary = "rolo tomassi";

	for (v=0 ; v<8 ; v++)
	{
    continue;
		ym_write (0x20+v,0xe4);
		ym_write (0x38+v,0);
		for (o=0;o<4;o++)
		{
			for (r=0;r<6;r++)
			{
				ym_write(0x40+0x20*r+0x08*o+v,val[o][r]);
			}
		}
	}
}

void ym_silence() {
  uint8_t voice;
  for (voice=0  ; voice<8 ; voice++) {
    ym_write(0xe0+voice,0x0f);
    ym_write(0xe8+voice,0x0f);
    ym_write(0xf0+voice,0x0f);
    ym_write(0xf8+voice,0x0f);
    ym_write(0x08,voice);
    YMshadow[0xe0] = 0x0f;
    YMshadow[0xe8] = 0x0f;
    YMshadow[0xf0] = 0x0f;
    YMshadow[0xf8] = 0x0f;
  }
}

void ym_init() {
  int i;
  // init the YM shadow registers and use them to init the YM chip
  for ( i=0 ; i < 256 ; i++)
  {
    if ((i & 0xE0) == 0x60) {
      YMshadow[i]=0x7f; // TL 0x7F = off
    }
    else if (i==0x1a)
      YMshadow[i]=0x80; // fake register for PMD value to be stored separately
    else
      YMshadow[i]=0x00;
  }

  //YMtestpatch();
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
  YMshadow[0x38] = 0x02; // AMS=2 PMS=0
  YMshadow[0x39] = 0x02;
  YMshadow[0x3a] = 0x02;
  YMshadow[0x3b] = 0x02;
  YMshadow[0x3c] = 0x02;
  YMshadow[0x3d] = 0x02;
  YMshadow[0x3e] = 0x02;
  YMshadow[0x3f] = 0x02;

  YMshadow[0x18] = 0xd0; // LFO Freq = 6.83hz - splitting the diff
                         // between OPL values for AM and PM.
  YMshadow[0x1b] = 0x01; // LFO Waveform = triangle
  YMshadow[0x19] = 0x03; // AMD at low setting by default
  YMshadow[0x1a] = 0x80 | 0x2c; // "PMS" register at low setting by default.


  // release all 8 voices
  for (i=0; i<8 ; i++) {
    ym_write(0xe0+i,0x0f);
    ym_write(0xe8+i,0x0f);
    ym_write(0xf0+i,0x0f);
    ym_write(0xf8+i,0x0f);
    ym_write(0x08,i);
  }


  // write the YMshadow out to the real YM.
  for (i=0 ; i < 256 ; i++ ) {
    if (i==0x1a) ym_write(0x19,YMshadow[0x1a]);
    else ym_write(i,YMshadow[i]);
  };
}
