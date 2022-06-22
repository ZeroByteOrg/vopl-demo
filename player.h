#ifndef _player_h_
#define _player_h_

#include <stdint.h>



typedef struct frame_s {
  uint8_t reg;
  uint8_t val;
  uint16_t delay;
} musicframe;

#define SONG_START *(musicframe*)0xA000
#define SONG_BANK 1

extern void irqhandler();
extern char player_init();
extern void player_start();
extern void player_stop();
extern char player_loadsong(unsigned char id);

extern musicframe* songEOF;
extern musicframe NULLSONG;

#endif
