#!/usr/bin/php
<?php

/* Script for extracting the AdLib SFX/Music data
 * from the WL1 audio files
 * 
 * AUDIOHED.WL1 = 32bit LE pointers into AUDIOT.WL1 as "chunks"
 * AUDIOT.WL1   = the actual data file
 *
 * "chunks" are the way Id stored resources for games back then.
 * Fortunately for the X16 programming community, the chunks are
 * stored uncompressed for Wolf3d - otherwise, any loader would
 * need to also implement a Huffman decoding engine. Fortunately,
 * each asset can be loaded directly from the AUDIOT file.
 * 
 * Neither file contains any info as to resource types, names, etc
 * (well, the AdLib songs have some filename meta data as footers)
 * as the dev tool (MUSE) would generate a C header with the
 * resource counts / indexes as ENUMs. The game would use those
 * ENUMs to reference the resources. I.e. nothing says "E1M3 BGM"
 * here - only in the .h file.
 * 
 * ::AUDIOHED.WL1::
 * 
 * The header format is fairly simple, but has a twist.
 * Each pointer is a simple offset into AUDIOT.WL1 so the first one
 * points to chunk 1, the second points to chunk 2, etc....
 * EXCEPT....
 * It is valid for two or more successive offsets to be the same.
 * In this case, the chunk ID does not increase.
 * E.g.: If the first 4 values are unique, then they represent
 * chunks 1-4. If the 5th,6th, and 7th are the same as each other
 * and the 8th is different, then the 5th and 6th "don't count"
 * and so chunk 5 is pointed at by the 7th pointer, chunk 6 is
 * the 8th, etc.
 * 
 * The last value should point to the end of the AUDIOT.WL1 file
 *
 * ::AUDIOT.WL1::
 * 
 * The data file contains all of the various audio formats supported
 * by Wolf3d for SFX and for music. The SFX come first. Id used
 * 3 types of audio data: PC Speaker, AdLib, and 'Digi' (PCM sfx).
 * The 'Digi' SFX are indexed in AUDIOHED.WL1, but stored in a
 * different file than AUDIOT.WL1
 * Id's tool inserts the string !ID! between each set of chunk types
 * but the pointers point around them.
 * 
 * The AdLib music data comes last in the file. Each music chunk begins
 * with a 2-byte header (little endian) which specifies the size of
 * the audio data (not including this header). The chunk itself will
 * be a little bit larger than this because the MUSE tool also wrote
 * some information at the end of the music chunks, but it doesn't
 * get used by the game, and certainly it's not valid AdLib music.
 */ 

/* SFX/Songs from AUDIOWL1.H */

// check that we can open AUDIOT.WL1
$df = fopen("AUDIOT.WL1", "rb");
if (!$df) {
	print "unable to open AUDIOT.WL1\n";
	exit(1);
}
fclose($df); // will re-open later when time to extract something

$sfx = array (
		'HITWALLSND' => 0,
		'SELECTWPNSND' => 1,
		'SELECTITEMSND' => 2,
		'HEARTBEATSND' => 3,
		'MOVEGUN2SND' => 4,
		'MOVEGUN1SND' => 5,
		'NOWAYSND' => 6,
		'NAZIHITPLAYERSND' => 7,
		'NAZIMISSSND' => 8,
		'PLAYERDEATHSND' => 9,
		'DOGDEATHSND' => 10,
		'ATKGATLINGSND' => 11,
		'GETKEYSND' => 12,
		'NOITEMSND' => 13,
		'WALK1SND' => 14,
		'WALK2SND' => 15,
		'TAKEDAMAGESND' => 16,
		'GAMEOVERSND' => 17,
		'OPENDOORSND' => 18,
		'CLOSEDOORSND' => 19,
		'DONOTHINGSND' => 20,
		'HALTSND' => 21,
		'DEATHSCREAM2SND' => 22,
		'ATKKNIFESND' => 23,
		'ATKPISTOLSND' => 24,
		'DEATHSCREAM3SND' => 25,
		'ATKMACHINEGUNSND' => 26,
		'HITENEMYSND' => 27,
		'SHOOTDOORSND' => 28,
		'DEATHSCREAM1SND' => 29,
		'GETMACHINESND' => 30,
		'GETAMMOSND' => 31,
		'SHOOTSND' => 32,
		'HEALTH1SND' => 33,
		'HEALTH2SND' => 34,
		'BONUS1SND' => 35,
		'BONUS2SND' => 36,
		'BONUS3SND' => 37,
		'GETGATLINGSND' => 38,
		'ESCPRESSEDSND' => 39,
		'LEVELDONESND' => 40,
		'DOGBARKSND' => 41,
		'ENDBONUS1SND' => 42,
		'ENDBONUS2SND' => 43,
		'BONUS1UPSND' => 44,
		'BONUS4SND' => 45,
		'PUSHWALLSND' => 46,
		'NOBONUSSND' => 47,
		'PERCENT100SND' => 48,
		'BOSSACTIVESND' => 49,
		'BOSSDIESSND' => 50,
		'SCHUTZADSND' => 51,
		'AHHHGSND' => 52,
		'DIESND' => 53,
		'EVASND' => 54,
		'GUTENTAGSND' => 55,
		'LEBENSND' => 56,
		'MUTTISND' => 57,
		'NAZIFIRESND' => 58,
		'BOSSFIRESND' => 59,
		'SSFIRESND' => 60,
		'SLURPIESND' => 61,
		'TOT_HUNDSND' => 62,
		'MEINGOTTSND' => 63,
		'SCHABBSHASND' => 64,
		'HILTERHASND' => 65,
		'SPIONSND' => 66,
		'NEINSOVASSND' => 67,
		'DOGATTACKSND' => 68
		);
		
$song = array(
		'CORNER_MUS' =>  0,
		'DUNGEON_MUS' =>  1,
		'GETOUT_MUS' =>  2,
		'GETTHEM_MUS' =>  3,
		'HEADACHE_MUS' =>  4,
		'HITLWLTZ_MUS' =>  5,
		'INTROCW3_MUS' =>  6,
		'NAZI_NOR_MUS' =>  7,
		'NAZI_OMI_MUS' =>  8,
		'POW_MUS' =>  9,
		'SALUTE_MUS' =>  10,
		'SEARCHN_MUS' =>  11,
		'SUSPENSE_MUS' =>  12,
		'VICTORS_MUS' =>  13,
		'WONDERIN_MUS' =>  14,
		'FUNKYOU_MUS' =>  15,
		'ENDLEVEL_MUS' =>  16,
		'GOINGAFT_MUS' =>  17,
		'PREGNANT_MUS' =>  18,
		'ULTIMATE_MUS' =>  19,
		'NAZI_RAP_MUS' =>  20,
		'ZEROHOUR_MUS' =>  21,
		'TWELFTH_MUS' =>  22,
		'ROSTER_MUS' =>  23,
		'URAHERO_MUS' =>  24,
		'VICMARCH_MUS' =>  25,
		'WARMRCH1_MUS' =>  26
);

// adjust Adlib SFX pointers to be relative to the first
// adlib sound chunk # as above
// (69 is defined in AUDIOWL1.H from Wolf3d source as the first sound)
foreach($sfx as $k => $v)
{
	$sfx[$k] = $v + 69;
}

// adjust music pointers to be the actual chunk #s above
// AUDIOWL1.H defines first song as chunk 207
foreach($song as $k => $v)
{
	$song[$k] = $v + 175; // dunno how the H file got 207...
}

$numchunks = 0;
$chunk=array();
$thischunk;
$nextchunk;

$audiohed = array();

$hf = fopen("AUDIOHED.WL1", "rb");
if (!$hf) {
	print "unable to open AUDIOHED.WL1\n";
	exit (1);
}
$h = fread($hf,filesize("AUDIOHED.WL1"));
fclose($hf);
$audiohed = unpack("V*",$h);

$i = 1;
array_push($chunk,$audiohed[$i++]);

while ($i <= sizeof($audiohed))
{
	$thischunk = $audiohed[$i++];
	if ($chunk[$numchunks] == $thischunk) { continue; };
	array_push($chunk,$thischunk);
	$numchunks++;
}

//print_r($audiohed);exit;

$of = fopen("mysong.imf","w+b");
if (!$of) {
	print "error opening output file mysong.imf\n";
	exit(1);
}

$df = fopen("AUDIOT.WL1","rb");
fseek($df,$chunk[$song['SEARCHN_MUS']]);
//$bytes = unpack("v",fread($df,2));
$bytes = fread($df,2);
$bytes = unpack("v*",$bytes);
//var_dump($bytes);
//exit;
$songdata = fread($df,$bytes[1]);
fclose($df);
fwrite($of,$songdata);
fclose($of);


