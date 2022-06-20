#ifndef _wolf3d_resources_h_
#define _wolf3d_resources_h_

#define AUDIOHED "audiohed.wl1"
#define AUDIOT "audiot.wl1"

#define MUSIC_START_RECORD 207 * 4
#define NUMSONGS 27

enum songs {
  CORNER_MUS,    //
  DUNGEON_MUS,	 //////////////////blank
  GETOUT_MUS,		 // called "warmarch" in the data
  GETTHEM_MUS,	 // E1M1
  HEADACHE_MUS,	 //////////////////blank
  HITLWLTZ_MUS,	 //////////////////blank
  INTROCW3_MUS,	 //////////////////blank
  NAZI_NOR_MUS,	 //title song (Horst Wessel Lied)
  NAZI_OMI_MUS,	 //////////////////blank
  POW_MUS,		   //
  SALUTE_MUS,	   //////////////////blank
  SEARCHN_MUS,	 //my favorite stage music
  SUSPENSE_MUS,
  VICTORS_MUS,	 //////////////////blank
  WONDERIN_MUS,	 // menu music
  FUNKYOU_MUS,	 //////////////////blank
  ENDLEVEL_MUS,
  GOINGAFT_MUS,	 //////////////////blank
  PREGNANT_MUS,	 //////////////////blank WTF was this???
  ULTIMATE_MUS,	 //////////////////blank
  NAZI_RAP_MUS,	 //////////////////blank
  ZEROHOUR_MUS,	 //////////////////blank
  TWELFTH_MUS,	 //////////////////blank
  ROSTER_MUS,
  URAHERO_MUS,
  VICMARCH_MUS,	 //////////////////blank
  WARMRCH1_MUS   //////////////////blank
};

const char* SONGNAME[NUMSONGS] = {
  "corner music", "dungeon music", "war march", "get them", "headache",
  "hitler waltz", "introcw3?", "hors wessel lied", "nazi omi music",
  "pow", "salute", "searching for...", "suspense", "victor",
  "wondering", "funk you", "end level", "going aft", "pregnant (really)",
  "ultimate", "nazi rap", "zero hour", "twelfth", "roster", "you're a hero",
  "victory march", "war march 1"
};

#endif
