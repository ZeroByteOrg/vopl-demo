#!/usr/bin/php -f
<?php

// creates a LUT which maps OPL frequencies to OPM KeyCodes
// eventually this can also include DT values for added accuracy.
// The LUT is a table of 16-bit values where the high byte is
// reserved for the DT, and the low byte is the OPM KC value -
// - that is, it already combines the OCT and KC values into the
// correct format for direct writes into OPM registers 0x28-0x2f

// For R39, use the 3.5MHz table (name it as $note)

// C#,D,D#,x,E,F,F#,x,G,G#,A,x,A#,B,C
$note = array( // values for when YM2151 is at 3.5MHz
	277.18, 293.66, 311.13, 0,  // c# d  d#  x
	329.63, 349.23, 369.99, 0,  // e  f  f#  x
	392.00, 415.30, 440.00, 0,  // g  g# a   x
	466.16, 493.88, 523.25, 0); // a# b  c   x

$note1 = array( // values for when YM2151 is at 4MHz
	311.13, 329.63, 349.23, 0,  // c# d  d#  x
	369.99, 392.00, 415.30, 0,  // e  f  f#  x
	440.00, 466.16, 493.88, 0,  // g  g# a   x
	523.25, 554.37, 587.33, 0); // a# b  c   x

// Magic numbers here are derived from: freq = 49716 * f-num / 2^20-block
// These are essentially the freqs for fnum=1 at all 8 octaves.
$f = array ( 0.047, 0.094, 0.189, 0.379, 0.758, 1.517, 3.034, 6.068 );

function freq($oct,$code) {
	global $note;

	return (round($note[$code] * pow(2,$oct-4),2));
}

function printcode ($freq) {
	global $note;
	$oct = 0;
	$c = 0;
	$min = 9999999;
	$bestOct = 0;
	$bestC   = 0;
	while ($oct < 8) {
		for ($c=0;$c<15;$c++) {
			if ($note[$c]!=0) {
				$dif = abs(freq($oct,$c)-$freq);
				if ($dif < $min) {
					$min = $dif;
					$bestOct = $oct;
					$bestC   = $c;
				};
				if ($dif > $min) { break 2; };
			};
		};
		$oct++;
	};
	printf("0x%04x", ($bestC & 0x0f) | ($bestOct << 4)); // omit DT for now.
};

/*
function printcode1 ($freq) {
	global $note;
	$oct = 0;
	while (($oct < 8) && (freq($oct,14) < $freq)) {
		$oct++;
	};
	$c = 0;
	while ((freq($oct,$c) < $freq) && (freq($oct,$c+1) < $freq) && ($c < 15)) {
		$c++;
	};
	printf("0x%04x", ($c & 0x0f) | ($oct << 4)); // omit DT for now.
};
*/

// main loop
print "uint16_t flut[8][1024] = {\n";
for ($b=0 ; $b<8 ; $b++) {
	print " {  // block=$b";
	$i=0;
	while ($i<1024) {
		if ($i % 8 == 0) {
			print "\n  ";
		};
		printcode($f[$b]*$i);
		if ($i != 1024) { print ", "; };
		$i++;
	};
	printf ("\n }", round($f[$b]*$i,0,PHP_ROUND_HALF_UP));
	if ($b != 7) { print ",\n"; }
	else { print "\n};\n";
  };
};
