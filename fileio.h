#ifndef _fileio_h_
#define _fileio_h_

#include <stdint.h>

typedef struct chunk_end_s {
  void*   addr; // next memory location after the last byte read
  uint8_t bank; // bank of the above address
  uint8_t ok;   // whether or not the load completed successfully
} chunk_end_t;

extern chunk_end_t chunkEnd;

extern void build_song_index();
extern uint8_t load_chunk(uint8_t index, void* buffer);


#endif
