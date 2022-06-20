#include <stdint.h>
#include <cbm.h>
#include <stdio.h>

#include "wolf3d_resources.h"

#define LFN 2
#define SA LFN // for cbm_read() to work properly for seekable files, SA must = LFN.
#define DEVICE 8

struct {
  uint32_t offset[NUMSONGS];
  uint16_t size[NUMSONGS];
} chunks;

int8_t cx16_fseek(uint8_t channel, uint32_t offset) {
  #define SETNAM 0xFFBD
  static struct cmd {
    char p;
    uint8_t lfn;
    uint32_t offset;
  } cmd;

  // open command channel to DOS and send P command.
  // P u8 u32 (no spaces) u8 is LFN to seek(), and u32 = offset.
  cmd.p='p';
  cmd.lfn=channel;
  cmd.offset=offset;
  // can't call cbm_open because the P command is binary and may
  // contain zeros, which C would interpret as null terminator.
  //
  // Roll-your-own inline asm call to SETNAM:
  __asm__ ("lda #6");
  __asm__ ("ldx #<%v",cmd);
  __asm__ ("ldy #>%v",cmd);
  __asm__ ("jsr %w",SETNAM);
  cbm_k_setlfs(15,8,15);
  cbm_k_open(); // this sends the CMD bytes..
  cbm_k_close(15); // close the command channel
  return 0;
  // TODO: ERROR HANDLING!!!!!
}

/* to load a piece o' file, and not the whole enchillada:
  cbm_open(LFN,DEVICE,SA*,FILENAME); // *for some reason SA must match LFN or it breaks
  cx16_fseek(CHAN,OFST)
  cbm_read(CHAN,char* BUFFER, uint16_t SIZE)
   (returns bytes read)
  cbm_close(CHAN)
*/

void build_song_index() {
  uint32_t last, current;
  uint8_t index;
  uint8_t numFound;

  cbm_open(LFN,DEVICE,SA,AUDIOHED);
  cx16_fseek(LFN,MUSIC_START_RECORD);
  cbm_read(LFN,&last,4);
  cbm_read(LFN,&current,4);
  index=0;
  while(index<NUMSONGS) {
    while(current==last) {
      cbm_read(LFN,&current,sizeof(current));
    }
    if (current-last == 4) {
      // iDsoft put these !ID! blocks in the chunks file, and references them
      // in the header file. Just ignore these as not-chunk indexes.
      last = current;
      continue;
    }
    chunks.offset[index] = last;
    // don't get the size here - get it from the AUDIOT file instead
    //chunks.size[index] = current-last;
    ++index;
    last=current;
  }
  cbm_close(LFN);
  numFound = index; // I know - it must be NUMSONGS right now, but this is
                    // future planning to account for EOF and so forth.
  cbm_open(LFN,DEVICE,SA,AUDIOT);
  for(index=0 ; index<numFound ; index++) {
    cx16_fseek(LFN,chunks.offset[index]);
    cbm_read(LFN,&chunks.size[index],sizeof(uint16_t));
  }
  cbm_close(LFN);
}

int main() {
  uint8_t i;
  //cbm_k_bsout(CH_M?????); // undo mixed-case mode
  build_song_index();
  for (i=0 ; i<NUMSONGS ; i++) {
    printf("%-30s : %08lx %5u bytes\n",SONGNAME[i],chunks.offset[i],chunks.size[i]);
  }
  return 0;
}
