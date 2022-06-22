#include <cx16.h>
#include <stdio.h>
#include "player.h"
#include "vopl.h"

#define IRQVECTOR (*(uint16_t*)0x0314)
#define VIA_IRQ_DISABLE_T1 0x40
#define VIA_IRQ_ENABLE_T1  0xC0

void player_advance();


musicframe* songPtr;
musicframe* songEOF;
uint16_t sysIRQ;

musicframe NULLSONG = { 5, 0, 0xffff }; // NOP and wait 0xffff
                                              // (reg 5 is an invalid register)

uint16_t delay = 0;
uint8_t playing = 0;

char player_init() {
  // Configure VIA2 to generate 700hz signal for music player
  VIA2.ier = VIA_IRQ_DISABLE_T1;
  VIA2.acr &= 0x3F; // mask off the T1 control bits from current value
  VIA2.acr |= 0x40; // set the T1 free-run mode bit.
  VIA2.t1l_lo = (8000000/700) & 0xFF;
  VIA2.t1l_hi = (((8000000/700) >> 8) & 0xFF);
  playing = 0;
  // install IRQ handler if not already installed
  if (IRQVECTOR != (uint16_t)&irqhandler) {
    sysIRQ = IRQVECTOR;
    __asm__("sei");
    IRQVECTOR = (uint16_t)&irqhandler;
    __asm__("cli");
  }
  printf ("survived setting up the irq\n");
  songPtr = &NULLSONG;
  songEOF = &NULLSONG+1;
  printf ("initializing vopl...\n");
  vopl_init();
  printf ("vopl initialized.\n");
  return 1;
}

// VIA IRQs for T1 Cleared by: Read T1C-L low or write T1L-H high
void irqhandler() {
  if ((VIA2.ifr & 0xC0) == 0xC0) {
    asm("bit %w",0x9F14); // ACK the IRQ
    player_advance(); // inline this later..... extracted for testbed access...
    __asm__("ply");
    __asm__("plx");
    __asm__("pla");
    __asm__("rti");
  }
  else __asm__ ("jmp (%v)",sysIRQ); // not a VIA IRQ for us.

}

void player_advance() {
  if (playing) {
    while (delay==0) {
      delay = songPtr->delay;
      vopl_write(songPtr->reg, songPtr->val);
      if (++songPtr == songEOF) {
        songPtr = &SONG_START;
        RAM_BANK = SONG_BANK;
      }
      else if (*(uint16_t*)songPtr > 0xC000) {
        songPtr = (musicframe*) 0xA000;
        ++RAM_BANK;
      }
    }
    --delay;
  }
}

void player_start() {
    RAM_BANK = SONG_BANK;
    songPtr = &SONG_START;
    VIA2.ier = VIA_IRQ_ENABLE_T1;
    playing = 1;
}

void player_stop() {
  VIA2.ier = VIA_IRQ_DISABLE_T1;
  songPtr = &NULLSONG;
  RAM_BANK = SONG_BANK;
  if(playing) {
    playing = 0;
    vopl_init(); // sledgehammer
  }
}
