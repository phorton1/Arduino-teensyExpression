#ifndef _ftpSysexDefs_h_
#define _ftpSysexDefs_h_

const UCHAR FTP_CODE_READ_PATCH = 0x01;		// get patch from controller (short message)

UCHAR  ftpRequestPatch[]	= { 0xF0, 0x00, 0x01, 0x6E, 0x01, FTP_CODE_READ_PATCH, 0x00, 0x00, 0xf7 };
         // bytes 6 and 7 (0 based) are the bank, and patch, respectively
         
         
typedef struct split_section
    // Each data patch has an array of five of these substructures in it.
    // Unlike the UI, which puts the "pedal" split at the end, the "pedal"
    // is the 0th element in the array, and the splits 1-4 follow in slots
    // 1-4 in the array.
{
	UCHAR pgm_change;   // 0..127
	UCHAR bank_lsb;         // 0..127
	UCHAR bank_msb;         // 0..127
	UCHAR pitch_bend;       // 0=auto, 1=smooth, 2=stepped, 3=trigger
	UCHAR transpose;        // 24=none, plus or minus 24 1/2 steps (up or down 2 octaves)
	UCHAR midi_volume;      // 0=unchecked, 1..126, weirdness at 127
					      // goes to zero and sets split bit in main patch max_volume
	UCHAR dyn_sens;         // Dynamics Sensitivity 0x0A..0x14 (10..20) weird
	UCHAR dyn_offs;         // Dynamic Offset 0..20
	UCHAR midi_reverb;      // 0=unchecked, 1..126, weirdness at 127
					     // goes to zero and sets split bit in main patch max_reverbe
};


typedef struct patch_buffer0 // 142 byte "data" packet (subpatch)
    // This structure is stored in banks 0 and 1.  The "hardware poly" and
    // "hardware mono" patches match this structure.  For each of these, there is
    // another structure patch_buffer1, which I call the "name" patch, which is
    // just the header, the name length byte, the name, then  the checksum in a
    // similarly sized (142 byte) packet stored in banks 2 and 3.
    //
    // So the "data" for "Poly Program n" is in bank(0) patch(n-1) whereas it's "name"
    // is bank(2) patch(n-1) ... "Mono Program n" is in (1,n-1) and (3,n-1)
{
	UCHAR header1[6];	// F0 00 01 6E 01 21
	UCHAR bank_num;         // 0=hardware poly, 1=hardware mono
	UCHAR patch_num;        // patch number within bank (0..127 .. only to 112 for mono bank?!?)
	UCHAR pedal_mode; 	// HoldUp(2), HoldDown(3), Alternate(4), Loop(6), DontBlockNewNotes(1), BlockNewNotes(0)
	UCHAR fret_range_low_up_12;     // the range of frets, from the lowest fret (open position) for hardware patches 1 & 2
	UCHAR fret_range_high_down_34;  // the range of frets from 1f down to this number, for hardware patches 3 and 4.
	UCHAR string_range_12; 		// the strings covered by the red hardware patch1 (as opposed to yellow hardware patch 2)
	UCHAR string_range_34;   		// the strings covered by the blue hardware patch3 (as opposed to green hardware patch 4)
	UCHAR azero0;                   // arpeggio mode which is "SEQUENTIAL" in all csv file examples
	UCHAR arp_speed;             // arpeggio speed gleened from csv file
	UCHAR arp_length;	  // arp. length gleened from csv file (arpegio length?)
	UCHAR touch_sens;   // 0..4 Default(4)
	UCHAR poly_mode;    // 0=mono, 1=poly  (Default 1 for bank0, 0 for bank1)
   
	struct split_section split[5];

	UCHAR azero1;       // a zero .. I have never seen this byte change
	UCHAR seqbytes[64]; // 64 bytes from 0..0x3f .. I have never seen these bytes change
	UCHAR program[8];	// the word "Program " with trailing space ... I have never seen these bytes change
	UCHAR azero2;       // a zero .. I have never seen this byte change
	UCHAR max_volume;   // bitwise bits set if split midi_volume *would be* 127
						// split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10
	UCHAR max_reverb;   // bitwise bits set if split midi_reverbe *would be* 127
						// split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10
	UCHAR checksum[2];  // two byte checksum
	UCHAR endmidi;      // 0xf7
};


const UCHAR FTP_CODE_READ_PATCH = 0x01;	// get patch from controller (short message)
const UCHAR FTP_CODE_ACK = 0x11;			// ack from the controller
const UCHAR FTP_CODE_WRITE_PATCH = 0x41;	// write patch to controller
const UCHAR FTP_CODE_PATCH_REPLY = 0x21;	// patch request reply from controller

// const UCHAR FTP_CODE_UNKNOWN = 0x02;		// ? clear the patch ?
// const UCHAR FTP_CODE_ERROR1 = 0x12;			// ? error ?
// const UCHAR FTP_CODE_ERROR2 = 0x22;			// ? Error from controller ?



#endif  // !_ftpSysexDefs_h_