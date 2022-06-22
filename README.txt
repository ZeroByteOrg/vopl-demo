VOPL Demo - Virtual OPL engine for Commander X16.

This program loads the iDsoft Music File (IMF) data directly from the
shareware assets files of Wolfenstein3d and plays them back on the
YM2151, doing the register translations in real time, on-the-fly.

The demo allows you to cycle through the game's various tunes using the
left/right cursor keys, or just hit spacebar for 'next' as well.

Enter stops playback.

F1 toggles the console overlay.
Q  quits to BASIC (but leaves the screen this way - kinda cool, I thought)

This must be run from an SD file, as the emulators' hostFS implementations
do not work with file IO beyond simple load/save operations, or at least not
at the low level required to perform the task of retreiving the IMF data
from AUDIOT.WL1.

Runs on emulator versions R39 and up. If you use Box16, you should make sure
to use the latest repo updates, as there was a bug in the VIA timer emulation
which caused the timer not to function properly in this demo.

------------------------------------------------------

On AdLib (OPL) -> YM2151 (OPM) translation:

Currently, the routine makes use of a very large (16k) lookup table to avoid
having to compute the frequency conversions from OPL (linear freq.) into
OPM (linear pitch).

The program is built in mostly C at the moment. The goal is to migrate the
VOPL portion of the code into pure assembly for improved performance and
reduced RAM footprint - as well as eliminating the 16K LUT.

Once this is done, it should be possible to play all kinds of AdLib tunes
and SFX on the Commander X16.

There are several aspects of the OPL chip that are not workable on OPM. Most
notably, the OPL has 9 voices while the OPM has 8. Fortunately for this
project, iD reserved channel 0 for SFX, so the music fits nicely into the
8 voices of the OPM chip on X16. If SFX were to be played, there would need
to be a priority system implemented.

Other OPL features not supported:
- Percussion mode
- alternative operator waveforms (OPM is sine-only)
- split-key mode
- vibrato mode (can be approximated with OPM LFO, but with limitations because
  OPL has independent vibrato while OPM LFO is shared amongst all voices.
- Others I might not recall at this time.

Cheers!

--ZeroByte

