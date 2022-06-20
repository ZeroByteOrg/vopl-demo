#!/usr/bin/php
<?php

/* Script for extracting the AdLib SFX/Music data
 * from the WL1 audio files
 *
 * AUDIOHED.WL1 = 32bit LE pointers into AUDIOT.WL1 as "chunks"
 * AUDIOT.WL1   = the actual data file
 *
 * "chunks" are the way iD stored resources for games back then.
 * Fortunately for the X16 programming community, the chunks are
 * stored uncompressed for Wolf3d - otherwise, any loader would
 * need to also implement a Huffman decoding engine. Fortunately,
 * each asset can be loaded directly from the AUDIOT file.
 *
 * Neither file contains any info as to resource types, names, etc.
 * (well, the AdLib songs have some filename meta data as footers)
 * The dev tool (MUSE) would generate a .H header with the
 * resource counts / indexes as ENUMs. The game would use those
 * ENUMs to reference the resources. I.e. nothing says "E1M3 BGM"
 * in AUDIOHED or AUDIOT - only the game itself defines which
 * song plays on which level / menu / etc. It uses the .H file
 * to know which offests the songs use.
 *
 * ::AUDIOHED.WL1::
 *
 * The header format is fairly simple, but has a twist.
 * Each pointer is a simple offset into AUDIOT.WL1 so the first one
 * points to chunk 0, the second points to chunk 1, etc....
 *
 * Thus index(X+1)-index(X) = the size of the chunk. If size = 0,
 * Then proceed to the next index without incrementing the "chunks"
 * counter. For WL1, These "null chunks" are confusing at
 * first glance because the AUDIOWL1.H definitions set the Adlib
 * music to begin at chunk 207. If you count non-null chunks from the
 * beginning of AudioHED, it is really index 236 or something similar
 * but if you skip counting nulls, it's something like 198...
 * The game engine just starts counting at zero from whichever index
 * is the "beginning of chunks for audio type X."
 *
 * AUDIOWL1.H defines music = 207, and AUDIOHED index 207 happens to
 * be in the middle of a long string of null chunks. So these are
 * skipped until the first nonzero chunk is encountered, which
 * is how the numbers don't add up with the actual placement in
 * the data files.
 *
 * The last chunk index in AUDIOHED.WL1 points to EOF(AUDIOT.WL1) + 1
 *
 * ::AUDIOT.WL1::
 *
 * The data file contains all of the various audio formats supported
 * by Wolf3d for SFX and for music. The SFX come first. iD used
 * 3 types of audio data: PC Speaker, AdLib, and 'Digi' (PCM sfx).
 * The 'Digi' SFX are indexed in AUDIOHED.WL1, but stored in a
 * different file than AUDIOT.WL1
 * iD's tool inserts the string !ID! between each set of chunk types
 * but the pointers point around them.
 *
 * The AdLib music data comes last in the file. Each music chunk begins
 * with a 2-byte header (little endian) which specifies the size of
 * the audio data (not including this header). The chunk itself will
 * be a little bit larger than this because the MUSE tool also wrote
 * some information at the end of the music chunks, but it doesn't
 * get used by the game, and certainly it's not valid AdLib music.
 */

$args = getopt('ms'); // <=========== TODO = make this do something

error_reporting(E_PARSE);
define('res',"res/");
define('audiohed',res."AUDIOHED.WL1");
define('audiot',res."AUDIOT.WL1");
define('outfile',res."song.imf");

function dienow($error) {
	if (strlen($error) > 0) { print "$error\n"; };
	if (isset($df)) { if ($df) { fclose($df); }; };
	if (isset($hf)) { if ($hf) { fclose($hf); }; };
	exit(1);
};

// First, open the Wolf3d Audio files
$df = fopen(audiot, "rb");
if (!$df) { dienow("unable to open ".audiot."\n"); };

$hf = fopen(audiohed, "rb");
if (!$hf) { dienow("unable to open ".audiohed."\n"); };

$of = fopen(outfile,"w+b");
if (!$of) { dienow("error opening output file ".outfile."\n"); };


/* SFX/Songs from original source AUDIOWL1.H */
define('startsfx','69');
define('startsongs','207');
define('numsfx','69');
define('numsongs','27');

// sfx is index0 of such tables, and mus is index1 in them...
// i.e. this is NOT a boolean. ;)
define('sfx',0);
define('mus',1);

$label = array (
sfx => array(
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
),
mus => array(
		'CORNER_MUS' =>  0,		//
		'DUNGEON_MUS' =>  1,	//////////////////blank
		'GETOUT_MUS' =>  2,		// called "warmarch" in the data
		'GETTHEM_MUS' =>  3,	// E1M1
		'HEADACHE_MUS' =>  4,	//////////////////blank
		'HITLWLTZ_MUS' =>  5,	//////////////////blank
		'INTROCW3_MUS' =>  6,	//////////////////blank
		'NAZI_NOR_MUS' =>  7,	//title song (Horst Wessel Lied)
		'NAZI_OMI_MUS' =>  8,	//////////////////blank
		'POW_MUS' =>  9,		//
		'SALUTE_MUS' =>  10,	//////////////////blank
		'SEARCHN_MUS' =>  11,	//my favorite stage music
		'SUSPENSE_MUS' =>  12,
		'VICTORS_MUS' =>  13,	//////////////////blank
		'WONDERIN_MUS' =>  14,	// menu music
		'FUNKYOU_MUS' =>  15,	//////////////////blank
		'ENDLEVEL_MUS' =>  16,
		'GOINGAFT_MUS' =>  17,	//////////////////blank
		'PREGNANT_MUS' =>  18,	//////////////////blank WTF was this???
		'ULTIMATE_MUS' =>  19,	//////////////////blank
		'NAZI_RAP_MUS' =>  20,	//////////////////blank
		'ZEROHOUR_MUS' =>  21,	//////////////////blank
		'TWELFTH_MUS' =>  22,	//////////////////blank
		'ROSTER_MUS' =>  23,
		'URAHERO_MUS' =>  24,
		'VICMARCH_MUS' =>  25,	//////////////////blank
		'WARMRCH1_MUS' =>  26	//////////////////blank
)
);

// Default extract type / resource name
define('type',mus);
define('targetname','SEARCHN_MUS');

// just use the defaults for now. Implement cmdline args here.

$index = array(
  sfx => array('start'=>startsfx,'max'=>numsfx),
  mus => array('start'=>startsongs,'max'=>numsongs)
);

$id = -1;
$offset = 4*$index[type]['start'];
$target = $label[type][targetname];
fseek($hf,$offset);
$offset += 4;

$nextone = fread($hf,40);
$nextone = array_shift(unpack("V",$nextone));
$thisone = $nextone;
printf ("Extracting the chunk %02d of type %d for %s\n",$target,type,targetname);
while ($id < $index[type]['max'])
{
	$nextone = fread($hf,4);
	$nextone = array_shift(unpack("V",$nextone));
	$offset += 4;
	if ($thisone == $nextone) {
		continue;
	};
 	if ($nextone - $thisone == 4) {
		// MUSE writes a tag of !ID! between real chunks
		// advance the $thisone pointer but not the ID
		$thisone = $nextone;
		continue;
	};
	;$id++;
//	printf ("%03d: 0x%08x - 0x%08x\n",$id,$thisone,$nextone);
	if ($id == $target) { break; };
	$thisone = $nextone;
}
fclose($hf);

fseek($df,$thisone);
$bytes = array_shift(unpack("v",fread($df,2)));
if ($bytes==0) {
	print "The " .type. " chunk " .targetname. " is blank in AUDIOT.WL1\n";
}
else {
	printf("chunk %s is at offset %08x and size %u bytes\n",targetname,$thisone,$bytes);
}
$songdata = fread($df,$bytes);
fclose($df);
fwrite($of,$songdata);
fclose($of);
