#include <stdint.h>
#include <cbm.h>
#include <stdio.h>
#include <errno.h>
#include "wolf3d_resources.h"

#define LFN 2
#define SA LFN
#define DEVICE 8

#ifdef TESTLOADER
#include <conio.h>
#endif


// for cbm_read() to work properly in sequential mode, SA must = LFN.
// furthermore, SA/LFN must be 2 or 6 to work in SEQ mode... I suspect that
// something to do with the fact that cbm.h defines:
//   CBM_READ=0
//   CBM_WRITE=1
//   CBM_SEQ=2     Note that 6 would result in bit 1 being set just as with 2.
// The value being sent to cbm_open() may be causing it to silently inject
// some text to the filename, such as appending ",r" | ",s" | ",p"

// returns -1 if error (unsupported device/file channel) or num bytes read.
// MACPTR can be sent num_bytes = 0 and the underlying routine will read up to
// 512 bytes at the driver's discretion.
// Sets STATUS byte upon exit.
int16_t __fastcall__ macptr(uint8_t num_bytes, void* buffer);
extern uint32_t __fastcall__ rdtim();

struct {
  uint32_t offset[NUMSONGS];
  uint16_t size[NUMSONGS];
} chunks;

const char* SONGNAME[NUMSONGS] = {
  "corner music", "dungeon music", "war march", "get them", "headache",
  "hitler waltz", "introcw3?", "hors wessel lied", "nazi omi music",
  "pow", "salute", "searching for...", "suspense", "victor",
  "wondering", "funk you", "end level", "going aft", "pregnant (really)",
  "ultimate", "nazi rap", "zero hour", "twelfth", "roster", "you're a hero",
  "victory march", "war march 1"
};


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

int cx16_read(unsigned char lfn, void* buffer, unsigned int size) {
  int error = 0;
  char* buf = (char*)buffer;
  static unsigned int bytesread;
  static int tmp;

  /* if we can't change to the inputchannel #lfn then return an error */
  if (_oserror = cbm_k_chkin(lfn)) return -1;

  bytesread = 0;
  printf("\n");
  while (size > 0 && !cbm_k_readst()) {
    if (size>=256)
      tmp = macptr(0,buf); // let MACPTR read as much as it wants
    else
      tmp = macptr((size), buf);
    if (tmp == -1) return -1;
    bytesread += tmp;
    size -= tmp;
    printf("macptr: read %3u bytes. %5u/%-5u  0x06%lx ticks\n",
      tmp,
      bytesread,
      size,
      rdtim()
    );
    buf += tmp;
    // wrap the buffer pointer back into the bank window
    // if it advances above 0xbfff. Note that MACPTR did
    // wrap banks correctly, but our calculation must match.
    // also note that MACPTR incremented the active bank,
    // so there is no need to do RAM_BANK++ here.
    if (buf >= (char*)0xc000) buf -= 0x2000;
    if (cbm_k_readst() & 0xBF) break;
  }
  printf("status byte = %02x\n",cbm_k_readst());
  cbm_k_clrch();
  return bytesread;

}

/* to load a piece o' file, and not the whole enchillada:
  cbm_open(LFN,DEVICE,SA,FILENAME); // use LFN = SA = 2 or 6
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
    // update offset pointer to point at the start of data
    // and not the size-of-data U16 value.
    if (chunks.size[index] > 0) chunks.offset[index]+=2;
  }
  cbm_close(LFN);
}

uint16_t load_chunk(uint8_t index, void* buffer) {
  uint8_t b;

  //char success=1; // in it to win it, baby!
  uint16_t bytesRead = 0;

  b=RAM_BANK;
  cbm_open(LFN,DEVICE,SA,AUDIOT);
  cx16_fseek(LFN,chunks.offset[index]);
  bytesRead = cx16_read(LFN,buffer,chunks.size[index]); // I bet this fails spectacularly!!!
  //bytesRead = cbm_read(LFN,buffer,chunks.size[index]);
  RAM_BANK=b;
  return bytesRead;
}

#ifdef TESTLOADER
// wrapper to uint test the above code. Will delete when done.
int main() {
  uint8_t i;
  cbm_k_bsout(CH_FONT_UPPER);
  build_song_index();
  for (i=0 ; i<NUMSONGS ; i++) {
    printf("%-30s : %08lx %5u bytes\n",SONGNAME[i],chunks.offset[i],chunks.size[i]);
  }
  RAM_BANK = 1;
  while (1) {
    printf("loading... ");
    printf( "\nLoaded song %s (%u bytes)\n\n", SONGNAME[CORNER_MUS],
      load_chunk(CORNER_MUS,(char*)0xa000)
    );
    while(!kbhit()) {}
    cgetc();
  }
  return 0;
}
#endif
