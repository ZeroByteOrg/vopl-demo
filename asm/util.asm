;.include "x16.inc" ; for RDTIM

RDTIM := $FFDE

.export _vsync
.export _snooze
.export _rdtim

.importzp sreg

;---------------------------------------------------[ _vsync ]---------------
; Waits for the end of the next VSYNC IRQ by repeatedly calling Kernal API
; RDTIM until the returned jiffies value changes.
;
; Note - this will lock up the system if the Kernal IRQ is not being called,
;        and will not really be a VSYNC if the Kernal's once-per-frame handler
;        is being called at some other time. If you don't want to set up an
;        IRQ that's the price you pay, I guess. ;)
;
; Args: None
; Returns: None
; Affects: A X Y  (because of RDTIM)
;
.code
.proc _vsync: near
  jsr RDTIM
  sta lastjiffy
keep_waiting:
  jsr RDTIM
  cmp #$FF
  lastjiffy=(*-1)
  beq keep_waiting
  rts
.endproc


;---------------------------------------------------[ _snooze ]---------------
; Cretes a snooze() function for C.
; Delays for 350 clock cycles (plus whatever overhead C adds before jumping in.)
;
; Thus: Delays ~43.75usec at 8MHz cpu... roughly 2 scan lines of raster time.
;
; No arguments. No returns. No registers clobbered.

.code
.proc _snooze: near
  phx       ; 3
  ldx #$30  ; 2
: nop       ; 2 * 48 = 96
  dex       ; 2 * 48 = 96
  bne :-    ; 2 + 3 * 47 = 143
  plx       ; 4
  rts       ; 6
.endproc

.code
.proc _rdtim: near
  jsr RDTIM
  sty sreg   ; .A is high byte
  stz sreg+1 ; RDTIM is 24bit so zero out the top byte
;  tya        ; .Y is the low byte - C expects this in A
  rts        ; .X is the mid byte - already in the right place
.endproc
