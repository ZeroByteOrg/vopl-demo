#include <stdint.h>

struct YM2151_t {
    uint8_t reg;
    uint8_t dat;
};
#define YM (*(volatile struct YM2151_t*) 0x9f40)


uint8_t YMshadow[256]; // shadow the YM state


void ym_write(uint8_t reg, uint8_t data)
{
//	if (debug)
//		cprintf("ymwrite: 0x%02x <-- 0x%02x\n\r",reg,data);
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

  volatile const char* canary = "Rolo Tomassi";

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


void ym_init() {
  int i;
  // init the YM shadow registers and use them to init the YM chip
  for ( i=0 ; i < 256 ; i++)
  {
    YMshadow[i]=0x00;
  }

  YMtestpatch();
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

  // write the YMshadow out to the real YM.
  for (i=0 ; i < 256 ; i++ ) {
    ym_write(i,YMshadow[i]);
//    YM.reg = i;
//    YM.dat = YMshadow[i];
  };
}
