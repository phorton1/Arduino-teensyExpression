//-------------------------------------------------------------
// fileSystem.h
//-------------------------------------------------------------
// Abstracted file system for use with serial IO protocol.
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.
//
// The fileSystem, as per it's usage as a fileServer, is limited to a two
// active commands at a time.
//
// We need teensyThreads to allow for re-entrancy for session like commands
// and abort/cancel functions.
//
// Due to implementation details in teensyThreads, we cannot, generally, use
// malloc() from within a thresd, because each thread allocates its stack on the heap,
// and, within the thread, the thread specific stack pointer is WITHIN the heap, so
// malloc does not work corrctly.
//


#pragma once

#define USE_OLD_FAT		1


#include "Arduino.h"
#include <SdFat.h>

#if USE_OLD_FAT
	#define myFile_t File
	#define myDir_t		 dir_t
	extern SdFatSdio SD;
#else
	#define myFile_t File32
	#define myDir_t		 DirFat_t
	extern SdFat32 SD;
#endif


#define BYTES_PER_MB    (1024*1024)


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

extern uint32_t getFreeMB();
extern uint32_t getTotalMB();
extern uint64_t getFreeBytes();
	// These are fileSystem values

extern const char *getTimeStamp(myFile_t *file);
extern const char *getTimeStamp(const char *path);
extern void setTimeStamp(myFile_t the_file, const char *ts);
extern bool mkDirTS(const char *path, const char *ts);


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
