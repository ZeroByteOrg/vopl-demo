; This code is intended to replace the 16K LUT in the main C
; program with on-the-fly computed values using a much smaller
; 256-byte LUT to assist with the log2() computation.

; This is a formula provided by Natt Akuma #9473 on X16 discord server.
; VERY MUCH appreciated!!!
; oct = log2(fnum)+block-8.5132730792261496510681735326167 ; more accurate
; note = oct*12
; kc = floor(note)
; kf = rem(note)

.export _fconvert

.segment "ZEROPAGE"
  KC:   .res 1
  KF:   .res 1
  note: .res 2 ; compute to 8.8 accuracy and see where that goes


.segment "CODE"

; Args: OPL BLOCK:FNUM value as .AX  (LSB in A, BLOCK:FNHI in .X)
; Returns: corresponding OPM KC:KF value as uint16_t with KF=high byte.
;        : a.k.a. A=KC, X=KF
block: .byte 0
.proc _fconvert: near
  sta note
  stz note+1
  txa ; prepare to do some shifting to unpack BLOCK from FNUM_HI
  tay ; stash original value in Y for fast retreival after unpacking BLOCK bits.
  lsr
  lsr
  and #$07
  sta block
  tya
  and #$03 ; A now = top 2 bits of the FNUM
  tay      ; Stash result in Y again, so that we can.....
  ora note ; check for FNUM=0 and early exit if it is.
  beq DONE

  ; perform log2 FNUM (integer part) by shifting to find the leftmost
  ; bit that is set. Since this is 10-bit, the high byte shifts right
  ; and the low byte shifts left if the high byte is empty
  ; no shift if leftmost set bit is bit 8. (bit0 of note+1)
  ; Store int portion in note+1, and use note as the .8 frac portion.
  tya      ; un-stash the FNUM hi bits
  beq shift_left
shift_right:
  lsr
  bne log_is_9
log_is_8:
  lda #8
  sta note+1
  lda note
  bra lookup_frac
log_is_9:
  lda #9
  sta note+1
  lda note
  ror   ; take bit 8 and rotate into note low
  bra lookup_frac
shift_left: ;guaranteed non-zero so no need to check X for underflow
  lda note
  ldx #8
check_next_bit:
  dex
  asl
  bcc check_next_bit
  stx note+1 ; X has accumulated the int part of the log2 operation.

lookup_frac: ; A should hold the "remainder" of the log2 int part..
  tax
  lda LOGTABLE,x
  sta note

  ; first subract Natt's constant
  sec
  lda #0 ; our LUT doesn't store frac bits 8-15. Use zero.
  sbc MAGIC_CONST_POSITIVE+2

  lda note
  sbc MAGIC_CONST_POSITIVE+1
  sta note
  lda note+1
  sbc MAGIC_CONST_POSITIVE
  sta note+1
  lda note
  rol ; use MSB of frac part of note as "rounding" yes/no bit.
  ; now add block to note+1 to get the value for "note" in the first calc step.
  lda note+1
  adc block
  sta note+1

  ; Step 1 complete. Now do note * 12 to get the KC/KF values.
  ; note<<3 + note<<2 = note*12
  lda note+1
  asl note
  rol
  asl note
  rol
  ldx note
  stx KF
  sta KC
  asl note
  rol
  tax ; stash note_hi << 3 in X
  clc
  lda note ; lo of note<<3
  adc KF   ; lo of note<<2
  sta KF   ; final value
  txa      ; hi of note<<3
  adc KC   ; hi of note<<2
  ldx KF   ; .A now holds KC, .X holds KF - as needed for return
DONE:
  rts
.endproc

.segment "RODATA"
MAGIC_CONST_POSITIVE: ; 8.32 fixed-point rep. of Natt's Constant(tm)
  .byte 8
  .byte %10000011
  .byte %01100101
  .byte %11011101
  .byte %01010001

;MAGIC_CONST_NEGATIVE ; 8.32 fixed-point rep. of -Natt's Constant(tm)
;  .byte %11110111
;  .byte %01111100
;  .byte %10011010
;  .byte %00100010
;  .byte %10101111

LOGTABLE:
  .byte 0,  1,  3,  4,  6,  7,  9,  10, 11, 13, 14, 16, 17, 18, 20, 21
  .byte 22, 24, 25, 26, 28, 29, 30, 32, 33, 34, 36, 37, 38, 40, 41, 42
  .byte 44, 45, 46, 47, 49, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 62
  .byte 63, 65, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81
  .byte 82, 84, 85, 86, 87, 88, 89, 90, 92, 93, 94, 95, 96, 97, 98, 99
  .byte 100,102,103,104,105,106,107,108,109,110,111,112,113,114,116,117
  .byte 118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133
  .byte 134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149
  .byte 150,151,152,153,154,155,155,156,157,158,159,160,161,162,163,164
  .byte 165,166,167,168,169,169,170,171,172,173,174,175,176,177,178,178
  .byte 179,180,181,182,183,184,185,185,186,187,188,189,190,191,192,192
  .byte 193,194,195,196,197,198,198,199,200,201,202,203,203,204,205,206
  .byte 207,208,208,209,210,211,212,212,213,214,215,216,216,217,218,219
  .byte 220,220,221,222,223,224,224,225,226,227,228,228,229,230,231,231
  .byte 232,233,234,234,235,236,237,238,238,239,240,241,241,242,243,244
  .byte 244,245,246,247,247,248,249,249,250,251,252,252,253,254,255,255
