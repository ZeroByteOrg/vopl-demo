ALL_OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
ALL_ASM  = $(patsubst %.c,%.s,$(wildcard *.c))
ASM_SRCS = $(wildcard asm/*.asm)
ALL_HDRS = $(wildcard *.h)
ALL_C    = $(wildcard *.c)
TARGET   = VOPLDEMO.PRG
ASSETS   = TITLE.BIN PAL.BIN SPR.BIN

RES		= ./res
TOOLS	= ./tools

imgtool	= $(TOOLS)/img2tileset.py
bin2c   = $(TOOLS)/bin2c.py

.PHONY: all
all: $(TARGET) $(ASSETS)
	@echo "Enjoy the demo."

$(TARGET): $(ALL_C) $(ASM_SRCS) song.h ymlookup.h
	cl65 -t cx16 -O -g -Ln vopldemo.sym -o $(TARGET) $(ALL_C) $(ASM_SRCS)

TITLE.BIN: $(RES)/wolf3dtitle.png
	$(imgtool) -s 320x240 -b 8 $< $@

PAL.BIN: $(TOOLS)/mkpal.php
	$(TOOLS)/mkpal.php

SPR.BIN: $(RES)/pc13.png
	$(imgtool) -s 32x64 -b 4 $< $@

song.h: $(RES)/song.imf
	$(bin2c) song $< $@

song.imf:
	$(TOOLS)/chunks.php

ymlookup.h:
	$(TOOLS)/mklut.php > $@

playimf.o: playimf.s
	ca65 -t cx16 $<

%.s: %.c
	cc65 -t cx16 -O -o $@ $<

%.o: %.s
	ca65 -t cx16 -o $@ $<

clean:
	rm -f *.o *.s *.PRG *.BIN song.h ymlookup.h

cleansd:
	mdel x:*.*

load: all
	x16emu -sdcard sdcard.img -prg $(TARGET)

run: all
	x16emu -sdcard sdcard.img -prg $(TARGET) -run

runbox16: all
	box16 -sdcard sdcard.img -prg $(TARGET) -run

sdcard: all
# You need to have the following in ~/.mtoolsrc for this to work:
# mtools_skip_check=1
# drive x: file="<path>/<to>/playimf/sdcard.img" exclusive partition=1
	mcopy -D o $(TARGET) x:
	mcopy -D o TITLE.BIN x:
	mcopy -D o PAL.BIN x:
