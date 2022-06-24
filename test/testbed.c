#include <stdint.h>
#include <stdio.h>
#include <conio.h> // 'cause why not?
#include <cx16.h>

extern uint16_t __fastcall__ fconvert(uint16_t blockfnum);

uint16_t blockfnum;
uint16_t block;

uint16_t prompt(char* p) {
  uint16_t input;
  while (1){
    printf(p);
    scanf("%u",&input);
    if (input<1024){
        return input;
    }
    else{
        printf("invalid input\n");
    }
  }
}


int main() {
	cputc(CH_FONT_UPPER);
	while (1) {
		block = 256;
		while (block > 7) {
		  block = prompt("enter block no: ");
			if (block >= 8) printf("block must be 0-7.\n");
		}
		blockfnum = 1024;
		while (blockfnum >= 1024) {
			blockfnum = prompt("enter f-number: ");
			if (blockfnum >= 1024) printf("fnum must be 0-1023\n");
		}
		blockfnum = (blockfnum/2) | (block << 10);
		blockfnum = fconvert(blockfnum);
		printf ("\nthat gives an opm equivalent kc:%02x kf:%02x\n",blockfnum&0xff,blockfnum>>8);
	}
	return 0;
}
