//-------------------------------------------------------------
// fileSystem.h
//-------------------------------------------------------------
// 2025-03-08 POSSIBLE BREAKTHROUGH
//
// While working on TE3, I noticed that I could generally use
// the regular SD "File", and apart from the unused and probably
// now bogus fileFormat.cpp, could compile TE1 against the
// included teensy SD API, possibly doing away with the need
// for the old SDFat that I kept in my libraries folder.
// I removed the TE2 
//
// In other words, it is very close to possible to have a single
// set of source code that works on both the old TE1/2 teensy3.6
// and the new TE3 teensy 4.1.
//
// The only aberations I noticed were
//
//	(a) the root file directory TS is returning as "" and
//      Pub::FS::FileINfo::fromText() was balking, so I
//		commented that error check out, and
//  (b) In order to use rmRfStar(), which I need to remove
//		populated directories, in the fileCommand.cpp::_delete()
//		method I had to explicitly open a "FatFile" O_RDONLY
//      and it seemed to work.
//
// Otherwise the changes appear to have come down to the
// fact that the old library File had a getName() function that
// required a buffer, whereas the new one has a name() function
// that returns a const char *, which is better anyways, and
// the old File had an isDirectory() method that appears to have
// been replaced with an isDir() method.
//
// So, as of this writing I am removeing "myFile_t File" and
// the USE_OLD_FAT define and associated complexity.


#pragma once

#include "Arduino.h"
#include <SD.h>

//----------------------------------------------
// general SD/fileSystem API in fileSystem.cpp
//----------------------------------------------

extern bool initFileSystem();
extern bool hasFileSystem();
	// returns true if there is both an SDCard
	// and it has a file system.
extern bool hasSDCard();
	// there may be an SDCard() with no valid fileSystem
	// that can still be formatted.


// These are currently the MODIFY timestamps,
// but (once I figure this mess out) should be
// easily parameterized to do CREATE timestamps as well.

extern const char *getTimeStamp(File *file);
extern const char *getTimeStamp(const char *path);
extern void setTimeStamp(File *file, const char *ts);
extern bool mkDirTS(const char *path, const char *ts);





#if 1
	//------------------------------------
	// memory debugging
	//------------------------------------

	#define dbg_malloc  0

	#define DO_MEM_CHECKS   1

	extern void print_mem_info(const char *where = 0);
	extern void print_long_mem_info(const char *where = 0);

	#if DO_MEM_CHECKS
		#define MEM_INFO(s)			print_mem_info(s)
		#define LONG_MEM_INFO(s)	print_long_mem_info(s)
	#else
		#define MEM_INFO(s)
		#define LONG_MEM_INFO(s)
	#endif
#endif






//-------------------------------------------------
// fileServer specific API to TE1 and TE2
//-------------------------------------------------
// fileFormat.cpp currently not shown here

extern void freeFileCommands();
	// in fileUtils.cpp called from loop()
extern void handleSerialData();
	// in fileSerial.cpp
	// THIS METHOD MAKES NON-ORTHOGONAL CALLS for Serial Midi.
	// Each program must implement the below method to handle
	// the midi data.
extern void handleCommonMidiSerial(uint8_t *midi_buf);
	// THIS MUST BE IMPLEMENTED ON BOTH SYSTEMS.







//----------------------------------------------
// fileUtils.cpp API
//----------------------------------------------
// from here down are 'private' to fileXXXX files

#define MAX_QUEUED_BUFFERS  5
	// Maximum number of subcommands that can be pending for a command.
	// In practice we never use more than two or three.
#define MAX_DIRECTORY_BUF 4096
	// maximum size of a directory listing returned by _list
#define MAX_FILE_BUF	10000
	// 10000 is agreed upon limit in Perl
#define MAX_DECODED_BUF    (MAX_FILE_BUF + 5)
	// allows 10000 + 4 byte checksum + null terminator
	// invariantly allocated in PUT
	// decoded buffer is allocated to size in BASE64
#define MAX_ENCODED_BUF	   14000
	// must be big enough to encode MAX_DECODED_BUF
	// invariantly allocated in PUT


typedef struct
{
	int req_num;		// 0 indicates that it is not in use
	int cmd_num;
	bool done;

	// result of initisl parseCommand
	// happens before thread started

	const char *command;
	int num_params;
	const char *params[3];
	const char *entries;

	// once we know the command, we can malloc any needed
	// large buffers before starting the thread

	char *dir_buffer;
	char *decoded_buffer;
	char *encoded_buffer;
	
	// the entries themselves are parsed in doCommand
	// and finally, we have the buffers that get added
	// by addCommandQueue

	int head;
	int tail;
	char *queue[MAX_QUEUED_BUFFERS];


} fileCommand_t;



extern void initFileCommands();
	// called from initFileSystem()
extern bool startCommand(int req_num, char *initial_buffer);
	// called from handleSerial()
	// Starts a file_command for a req_num, parses the initial
	// buffer, allocates any larger buffer then fires off the
	// fileCommand() thread.
extern bool addCommandQueue(int req_num, char *buf);
	// called from handleSerial()
	// for messages WITHIN a command (req_num)
	// Adds an asynchronous buffer to process like
	// commands that are waiting for them.not active.
extern char *getCommandQueue(fileCommand_t *cmd);
	// called from wait loops in fileCommand.cpp


extern void  fileReplyError(Stream *fsd, fileCommand_t *cmd, const char *format, ...);
extern void  fileReply(Stream *fsd, fileCommand_t *cmd, const char *format, ...);
	// Methods to send replies directly to the fileServer stream

extern bool parseCommand(fileCommand_t *cmd, int req_num, char *buf);
	// parse the command and three params from a buffer into a fileCommand_t


//----------------------------------------------
// fileCommand.cpp API
//----------------------------------------------

extern void fileCommand(void *vptr);
	// this is the thread method
	// called from startCommand


// end of fileSystem.h
