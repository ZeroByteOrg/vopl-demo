; This code is intended to replace the 16K LUT in the main C
; program with on-the-fly computed values using a much smaller
; 256-byte LUT to assist with the log2() computation.

; This is a formula provided by Natt Akuma #9473 on X16 discord server.
; VERY MUCH appreciated!!!
; oct = log2(fnum)+block-8.5132730792261496510681735326167 ; more accurate
; note = oct*12
; kc = floor(note) * 4/3 (the *4/3 comes from Grauw's discussion w/ Vampyrefrog
; kf = rem(note)          ... on vgmrips.net)

; will do *4/3 with a LUT.

NATTSNUMBER_INT   = 8
NATTSNUMBER_FRAC1 = %10000011
NATTSNUMBER_FRAC2 = %01100101

.export _fconvert

.segment "ZEROPAGE"
  KC:   .res 1
  KF:   .res 1
  note_frac: .res 1 ; compute to 8.8 accuracy and see where that goes
  note_int:  .res 1


.segment "CODE"

; Args: OPL BLOCK:FNUM value as .AX  (LSB in A, BLOCK:FNHI in .X)
; Returns: corresponding OPM KC:KF value as uint16_t with KF=high byte.
;        : a.k.a. A=KC, X=KF
block: .byte 0
.proc _fconvert: near
  sta note_frac
  stz note_int
  txa ; prepare to do some shifting to unpack BLOCK from FNUM_HI
  tay ; stash original value in Y for fast retreival after unpacking BLOCK bits.
  lsr
  lsr
  and #$07
  sta block
  tya
  and #$03 ; A now = top 2 bits of the FNUM
  tay      ; Stash result in Y again, so that we can.....
  ora note_frac ; check for FNUM=0 and early exit if it is.
  beq DONE

  ; perform log2 FNUM (integer part) by shifting to find the leftmost
  ; bit that is set. Since this is 10-bit, the high byte shifts right
  ; and the low byte shifts left if the high byte is empty
  ; no shift if leftmost set bit is bit 8. (bit0 of note_int)
  ; Store int portion in note_int, and use note as the .8 frac portion.
  tya      ; un-stash the FNUM hi bits
  beq shift_left
shift_right:
  lsr
  bne log_is_9
log_is_8:
  lda #8
  sta note_int
  lda note_frac
  bra lookup_frac
log_is_9:
  lda #9
  sta note_int
  lda note_frac
  ror   ; take bit 8 and rotate into note low
  bra lookup_frac
shift_left: ;guaranteed non-zero so no need to check X for underflow
  lda note_frac
  ldx #8
check_next_bit:
  dex
  asl
  bcc check_next_bit
  stx note_int ; X has accumulated the int part of the log2 operation.

lookup_frac: ; A should hold the "remainder" of the log2 int part..
  tax
  lda LOGTABLE,x
  sta note_frac

; return the 8.8 log2 instead of the KC for testbed purposes
;  tax
;  lda note_int
;  rts
; ----------------------------------------------------------
  ; first subract Natt's constant
  sec
  lda #0 ; our LUT doesn't store frac bits 8-15. Use zero.
  sbc #NATTSNUMBER_FRAC2

  lda note_frac
  sbc #NATTSNUMBER_FRAC1
  sta note_frac
  lda note_int
  sbc #NATTSNUMBER_INT
  sta note_int
;  lda note_frac
;  rol ; use MSB of frac part of note as "rounding" yes/no bit.
  ; now add block to note_int to get the value for "note" in the first calc step.
  clc
  lda note_int
  adc block
  sta note_int

  ; Step 1 complete. Now do note * 12 to get the KC/KF values.
  ; note<<3 + note<<2 = note*12
  lda note_int
  asl note_frac
  rol
  asl note_frac
  rol
  ldx note_frac
  stx KF
  sta KC
  asl note_frac
  rol
  tax ; stash note_hi << 3 in X
  clc
  lda note_frac ; lo of note<<3
  adc KF   ; lo of note<<2
  sta KF   ; final value
  txa      ; hi of note<<3
  adc KC   ; hi of note<<2
  tay
  cmp #96
  bcc get_answer
  ldy #95  ; clamp to highest note of YM.
get_answer:
  lda FOURTHIRDS,y
  ldx KF   ; .A now holds KC, .X holds KF - as needed for return
DONE:
  rts
.endproc

.segment "RODATA"
;MAGIC_CONST_POSITIVE: ; 8.32 fixed-point rep. of Natt's Constant(tm)
;  .byte 8
;  .byte %10000011
;  .byte %01100101
;  .byte %11011101
;  .byte %01010001

;MAGIC_CONST_NEGATIVE ; 8.32 fixed-point rep. of -Natt's Constant(tm)
;  .byte %11110111
;  .byte %01111100
;  .byte %10011010
;  .byte %00100010
;  .byte %10101111

; Thanks to Scott Robinson for generating this LUT. --RIP
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

FOURTHIRDS: ; for values 0-95 which seems to be the limit of the range.
  .byte $00,$01,$02,$04,$05,$06,$08,$09,$0A,$0C,$0D,$0E,$10,$11,$12,$14
  .byte $15,$16,$18,$19,$1A,$1C,$1D,$1E,$20,$21,$22,$24,$25,$26,$28,$29
  .byte $2A,$2C,$2D,$2E,$30,$31,$32,$34,$35,$36,$38,$39,$3A,$3C,$3D,$3E
  .byte $40,$41,$42,$44,$45,$46,$48,$49,$4A,$4C,$4D,$4E,$50,$51,$52,$54
  .byte $55,$56,$58,$59,$5A,$5C,$5D,$5E,$60,$61,$62,$64,$65,$66,$68,$69
  .byte $6A,$6C,$6D,$6E,$70,$71,$72,$74,$75,$76,$78,$79,$7A,$7C,$7D,$7E
