#include <cx16.h>
#include <stdio.h>
#include "player.h"
#include "vopl.h"

#ifdef DEBUG
#include <conio.h>
#endif

#define IRQVECTOR (*(uint16_t*)0x0314)
#define VIA_IRQ_DISABLE_T1 0x40
#define VIA_IRQ_ENABLE_T1  0xC0

void player_advance();
extern char* chunk_end;

// music state variables
musicframe* songPtr;
musicframe* songStart;
musicframe* songEOF;
uint8_t songStartBank;
uint8_t songEndbank;
uint8_t songBank;

// holds the IRQ vector we found when installing the player IRQ.
uint16_t sysIRQ;

musicframe NULLSONG = { 5, 0, 0xffff }; // NOP and wait 0xffff
                                              // (reg 5 is an invalid register)

uint16_t delay = 0;
uint8_t playing = 0;

char player_isplaying() {
  return playing;
}

char player_init() {
  // Configure VIA2 to generate 700hz signal for music player
  VIA2.ier = VIA_IRQ_DISABLE_T1;
  VIA2.acr &= 0x3F; // mask off the T1 control bits from current value
  VIA2.acr |= 0x40; // set the T1 free-run mode bit.
  VIA2.t1_lo = (8000000/700) & 0xFF;
  VIA2.t1_hi = (((8000000/700) >> 8) & 0xFF);
  playing = 0;
  // install IRQ handler if not already installed
  if (IRQVECTOR != (uint16_t)&irqhandler) {
    sysIRQ = IRQVECTOR;
    __asm__("sei");
    IRQVECTOR = (uint16_t)&irqhandler;
    __asm__("cli");
  }
  printf ("survived setting up the irq\n");
  songStart = &NULLSONG;
  songEOF = &NULLSONG+1;
  songStartBank = 1; // irrelevent for NULLSONG, but set to be thorough.
  songEndbank = 1;   // ditto.
  songPtr = songStart;
  songBank = songStartBank;
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

#define PAUSE \
while(!kbhit()) {} \
cgetc();

void player_advance() {
  static uint8_t b;
  if (playing) {
    b = RAM_BANK;
    RAM_BANK = songBank;

    while (delay==0) {
#ifdef DEBUG
      printf("%02x.%04x: ",RAM_BANK,songPtr);
      printf(" r%02x v%02x d%04x \n",songPtr->reg, songPtr->val, songPtr->delay);
      PAUSE
#endif
      // write the current frame out to VOPL and set the delay.
      vopl_write(songPtr->reg, songPtr->val);
      delay = songPtr->delay;
      // next frame....
      ++songPtr;
      // check for HiRam bank wrap
      if((uint16_t)songPtr >= 0xC000) {
        songPtr -= 0x2000/sizeof(musicframe);
        ++RAM_BANK;
        ++songBank;
      }
      // see if we are at the end of the song... If so, loop to beginning.
      if ((RAM_BANK >= songEndbank) && (songPtr >= songEOF)) {
        songPtr = songStart;
        songBank = songStartBank;
        RAM_BANK = songBank;
      }
    }
#ifdef DEBUG
    printf ("\n -- delay %u --\n",delay);
    PAUSE
#endif
    --delay;
    RAM_BANK = b;
  }
}

void player_start(uint8_t s_bank, void* s_addr, uint8_t e_bank, void* e_addr) {
    songStart = (musicframe*)s_addr;
    songStartBank = s_bank;
    songEOF = (musicframe*)e_addr;
    songEndbank = e_bank;

    songPtr  = songStart;
    songBank = songStartBank;

    VIA2.ier = VIA_IRQ_ENABLE_T1;
    playing = 1;
    VIA2.t1_lo = (8000000/700) & 0xFF;
    VIA2.t1_hi = (((8000000/700) >> 8) & 0xFF);
}

void player_stop() {
  VIA2.ier = VIA_IRQ_DISABLE_T1;
  if(playing) vopl_init(); // sledgehammer
  playing = 0;
  songStart = &NULLSONG;
  songStartBank = 1;
  songEOF = &NULLSONG;
  songEOF++;
  songEndbank = 1;
  songPtr = songStart;
  songBank = songStartBank;
  RAM_BANK = SONG_BANK;
}

void player_shutdown() {
  player_stop();
  if (IRQVECTOR == (uint16_t)&irqhandler) {
    __asm__ ("sei");
    IRQVECTOR = sysIRQ;
    __asm__ ("cli");
  }
}
