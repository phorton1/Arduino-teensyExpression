//-------------------------------------------------------------
// fileSystem.h
//-------------------------------------------------------------
// Abstracted file system for use with serial IO protocol.
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.

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


//----------------------------------------------
// following in fileSystem.cpp
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


//----------------------------------------------
// following in fileUtils.cpp
//----------------------------------------------
// public API to theSystem.cpp

#define MAX_QUEUED_BUFFERS  10
	// maximum number of subcommands that can pending for a command


typedef struct
{
	int req_num;
	int head;
	int tail;
	char *queue[MAX_QUEUED_BUFFERS];
} fileCommand_t;


extern fileCommand_t *getCommand(int req_num, int sem_level = 0);
	// for file_messages, theSystem will blow it off if the
	// this returns NULL, indicating the command is not active
	// anymore, otherwise, it will add the buffer to the given
	// req_num's queue
extern bool startCommand(int req_num, char *initial_buffer);
	// for file_commands, calling this also invokes the thread,
	// it first initializes the command_queue for the command and
	// adds the buffer, then fires off the thread.
	// reports errors for any problems.
extern bool addCommandQueue(int req_num, char *buf);
	// for file_messages, after checking if the command exists,
	// it will call this method with the bufer. This method
	// reports an error if the command is not active.



//----------------------------------------------
// API from fileUtils.cpp to fileCommand.cpp
//----------------------------------------------

#define MAX_FILE_PARAMS	  3
	// maximum number of paremeters per command


typedef struct
{
	char size[10];
	char ts[22];
	char entry[255];
	bool is_dir;
}   textEntry_t;


extern void  endCommand(int req_num);
extern char *getCommandQueue(int req_num);
extern void  fileReplyError(Stream *fsd, int req_num, const char *format, ...);
extern void  fileReply(Stream *fsd, int req_num, const char *format, ...);
extern int   parseCommand(char *buf, const char **params, const char **entries = 0);
extern int   getNextEntry(Stream *fsd, int req_num, textEntry_t *the_entry, const char **ptr);


//----------------------------------------------
// doCommand() method
//----------------------------------------------
// available to fileUtils::startCommand()

extern void fileCommand(int req_num);


// end of fileSystem.h
