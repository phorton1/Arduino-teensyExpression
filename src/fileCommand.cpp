//----------------------------------------------------------
// fileCommand.cpp
//----------------------------------------------------------
// contains fileCommand() and associated methods
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.

#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>
#include <TeensyThreads.h>
#include <Base64.h>


#define dbg_hdr	  0
	//  0 = show a msg after parsing and at end of command
	// -1 = show msg at top of doCommand
#define dbg_cmd	  -1
	//  0 = show file commands
	// -1 = show command details
#define dbg_entries  1
	//  0 = show command entries as parsed


#define TEST_DELAY    0		// 2000
	// delay certain operations to test progress dialog etc
	// set to 1000 or 2000 ms to slow things down

#define WITH_LOCAL_PROGRESS  	1
	// i envision a scheme where servers send minimal
	// progress messags. This defines those that could
	// be done on the other side with knowlege of both sessions.


#define FILE_TIMEOUT  15000	  // ms
	// time to wait for next BASE64 packet in FILE command

#define MAX_RECURSION_DEPTH  8
	// see notes in fileUtils.cpp

#define SIZE_TIMESTAMP	   20
	// YYYY-MM-DD HH:MM:SS plus nul terminator
#define MAX_FILENAME    255
	// maximum length of fully qualified filename supported by fileCommands
#define MAX_DIRECTORY_BUF   4096
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

//---------------------------------
// common utilities
//---------------------------------

static bool makePath(Stream *fsd, int req_num, char *buf, const char *dir, const char *entry)
{
	if (strlen(dir) + strlen(entry) + 1 >= MAX_FILENAME)
	{
		fileReplyError(fsd, req_num, "path too long(%d)!!",strlen(dir) + strlen(entry) + 1);
		return 0;
	}
	buf[0] = 0;
	strcat(buf,dir);
	if (strcmp(dir,"/"))
		strcat(buf,"/");
	strcat(buf,entry);
	return 1;
}

static uint32_t calcChecksum(const uint8_t *buf)
{
	uint32_t cs = 0;
	while (*buf) { cs += *buf++; }
	return cs;
}

static char *waitReply(int req_num, const char *command_name)
{
	uint32_t wait = millis();
	char *buf = getCommandQueue(req_num);
	while (!buf && millis() - wait < FILE_TIMEOUT)
	{
		threads.delay(100);		// yield thread for 100 ms
		buf = getCommandQueue(req_num);
	}
	if (!buf)
		my_error("fileCommand::waitReply(%s) timeout",command_name);
	else if (!strncmp(buf,"BASE64",6))
		display_level(dbg_cmd+2,0,"waitReply got: BASE64 packet_len(%d)",strlen(buf))
	else
		display_level(dbg_cmd+2,0,"waitReply got: %s",buf)
	return buf;
}


//---------------------------------------------
// simple commands
//---------------------------------------------

static void _list(Stream *fsd, int req_num, const char *dir)
{
	display_level(dbg_cmd,2,"LIST(%d,%s)",req_num,dir);
	// mem_check("_list");

	char *buffer = (char *) malloc(MAX_DIRECTORY_BUF);
	if (!buffer)
	{
		my_error("could not allocate memory",0);
		delay(1000);
		fileReplyError(fsd,req_num,"LIST(%d,%s) could not allocate buffer",req_num,dir);
		return;
	}

	myFile_t the_dir = SD.open(dir);
	if (!the_dir)
	{
		my_error("could not opendir(%s)",dir);
		delay(1000);
		fileReplyError(fsd,req_num,"LIST(%d,%s) could not open directory",req_num,dir);
		free(buffer);
		return;
	}

	const char *ts = getTimeStamp(&the_dir);

	// prh 2025-01-12  fileClient expects SIZE,ts,MODE,OWNER,GROUP,entry
	
	sprintf(buffer,"file_reply(%d):0\t%s\t0\t0\t0\t%s%s\r",
		req_num,
		ts,
		dir,
		strcmp(dir,"/")?"/":"");

	unsigned int at = strlen(buffer);
	char *out = &buffer[at];

	myFile_t entry = the_dir.openNextFile();
	while (entry)
	{
		char name[MAX_FILENAME + 1];
		entry.getName(name, sizeof(name));
		display_level(dbg_cmd+2,4,"got name(%s)",name);

		const char *ts = getTimeStamp(&entry);
		int32_t size = entry.size();
		bool is_dir = entry.isDirectory();

		#define MAX_FILE_SIZE_CHARS   10	// 9GB

		display_level(dbg_cmd+1,3,"_list at(%d) is_dir(%d) size(%ld) ts(%d) name(%s)",at,is_dir,size,ts,name);

		if (at > MAX_DIRECTORY_BUF - MAX_FILE_SIZE_CHARS - 1 - strlen(ts) - 1 - strlen(name) - 2)
		{
			// report this as an error here, but not to client
			my_error("LIST(%d,%s) not reporting directory buffer overflow at %d!",req_num,dir,at);
			break;
		}

		// prh 2025-01-12  fileClient expects SIZE,ts,MODE,OWNER,GROUP,entry

		sprintf(out,"%ld\t%s\t0\t0\t0\t%s%s\r",size,ts,name,is_dir?"/":"");
		at += strlen(out);
		out = &buffer[at];
		entry = the_dir.openNextFile();

	}   // while (entry)

	the_dir.close();
	sprintf(out,"\n");
	fsd->printf(buffer);
	free(buffer);
}


static void _mkdir(Stream *fsd, int req_num, const char *path, const char *ts,  const char *may_exist)
{
	display_level(dbg_cmd,2,"MKDIR(%s,%s,%s)",path,ts,may_exist);
	bool use_exist = *may_exist && *may_exist != '0';
	if (SD.exists(path))
	{
		if (use_exist)
		{
			myFile_t check_file = SD.open(path);
			if (!check_file || !check_file.isDirectory())
				fileReplyError(fsd,req_num,"MKDIR %s is not a directory",path);
			else
				fileReply(fsd,req_num,"OK");
			return;
		}

		fileReplyError(fsd,req_num,"MKDIR %s already exists",path);
		return;
	}

	if (mkDirTS(path,ts))
	{
		if (use_exist)
			fileReply(fsd,req_num,"OK");
		else
		{
			// override the const on path so we
			// can list the directory portion only

			char *pppp = (char *) path;
			int i = strlen(pppp)-1;
			while (i>0)
			{
				if (pppp[i] == '/')
				{
					pppp[i] = 0;
					break;
				}
				i--;
			}
			if (!i)
				pppp[1] = 0;
			_list(fsd,req_num,pppp);
		}
	}
	else
		fileReplyError(fsd,req_num,"MKDIR could not make directory %s",path);
}


static void _rename(Stream *fsd, int req_num, const char *dir, const char *name1, const char *name2)
{
	// names already have trailing /'s removed

	char path1[MAX_FILENAME + 1];
	char path2[MAX_FILENAME + 1];
	if (!makePath(fsd,req_num,path1,dir,name1) ||
		!makePath(fsd,req_num,path2,dir,name2))
		return;

	myFile_t file = SD.open(path1);
	if (!file)
	{
		fileReplyError(fsd,req_num,"RENAME Could not open %s",name1);
		return;
	}
	bool is_dir = file.isDirectory();
	const char *ts = getTimeStamp(&file);
	uint32_t size = file.size();
	file.close();

	display_level(dbg_cmd,2,"RENAME(%s,%s,%s) path1=%s path2=%s",dir,name1,name2,path1,path2);

	if (SD.rename(path1,path2))
		fileReply(fsd,req_num,"%d\t%s\t%s%s",size,ts,name2,is_dir?"/":"");
	else
		fileReplyError(fsd,req_num,"Could not RENAME %s to %s",name1,name2);
}



//---------------------------------------------
// session-like DELETE command
//---------------------------------------------

static bool abortPending(Stream *fsd, int req_num, const char *command)
{
	char *pending = getCommandQueue(req_num);
	if (pending)
	{
		if (!strncmp(pending,"ABORT",5))
		{
			display_level(dbg_cmd,3,"ABORTING fileCommand(%d,%s)!!",req_num,command);
			fileReply(fsd,req_num,"ABORTED");
		}
		free(pending);
		return true;
	}
	return false;
}


static bool _delete(Stream *fsd, int req_num, const char *dir, const char *entry)
{
	char path[MAX_FILENAME];
	if (!makePath(fsd,req_num,path,dir,entry))
		return 0;
    display_level(dbg_cmd,2,"DELETE(%s)",path);
	fileReply(fsd,req_num,"PROGRESS\tENTRY\t%s",entry);

	#if TEST_DELAY
		delay(TEST_DELAY);
	#endif

	myFile_t the_file = SD.open(path);

	bool ok = 1;
	if (the_file)
	{
		bool is_dir = the_file.isDirectory();
		if (is_dir)
		{
			ok = the_file.rmRfStar();		// remove directory and contents
		}
		else
		{
			the_file.close();
			ok = SD.remove(path);
		}

		#if TEST_DELAY
			delay(TEST_DELAY);
		#endif

		if (ok)
			fileReply(fsd,req_num,"PROGRESS\tDONE\t%d",is_dir);
		else
			fileReplyError(fsd,req_num,"could not DELETE %s",path);
	}
	else
	{
		ok = 0;
		fileReplyError(fsd,req_num,"DELETE could not open %s",path);
	}

	return ok;
}




//-------------------------------------------------------
// session-like FILE command
//-------------------------------------------------------
// contains BASE64 loop



bool makeSubdirs(Stream *fsd, int req_num, const char *in)
	// probably not needed now that PUT protocol includes MKDIR
	// expects a fully qualified path name starting with /
	// with a leaf terminal filename
	// makes any needed subdirectories for the file
{
	if (!in || !*in || *in != '/')
	{
		fileReplyError(fsd,req_num,"filename(%s) must be fully qualified",in);
		return 0;
	}

	char path[MAX_FILENAME+1];
	char *out = path;
	*out++ = *in++;

	while (*in)
	{
		if (*in == '/')
		{
			*out = 0;
			File check_file = SD.open(path);
			if (check_file)
			{
				if (!check_file.isDirectory())
				{
					fileReplyError(fsd,req_num,"attempt to overwrite file(%s) with a subdirectory",path);
					return 0;
				}
			}
			else
			{
				display_level(dbg_cmd+1,3,"FILE making subdir(%s)",path);
				if (!SD.mkdir(path))
				{
					fileReplyError(fsd,req_num,"could not create subdirectory(%s)",path);
					return 0;
				}
			}
		}
		*out++ = *in++;
	}

	return 1;

}


static void _file(Stream *fsd, int req_num, const char *sz_size, const char *ts, const char *full_name)
{
	int32_t size = atol(sz_size);
	display_level(dbg_cmd,2,"FILE(%s,%ld,%s)",full_name,size,ts);

	//--------------------------------
	// validate _file request
	//--------------------------------

	int name_len = strlen(full_name);
	if ( name_len >= MAX_FILENAME)
	{
		fileReplyError(fsd,req_num,"FILE file(%s) name(%d) too long",full_name,name_len);
		return;
	}

	#if 0
		// getFreeBytes() is the slow culprit().
		// Instead, we let write() file if there is not room
		#define DISK_FULL_MARGIN	(1024 * 1024)
		// leave at least 1MB free during FILE command
		uint64_t avail = getFreeBytes();
		uint64_t size_64 = size;
		if (avail <= size_64 + DISK_FULL_MARGIN)
		{
			uint64_t mb = size_64 / BYTES_PER_MB;
			uint32_t mb_32 = mb;
			fileReplyError(fsd,req_num,"FILE(%s) size(%lu) too large to fit in remaining MB(%lu)",full_name,size,mb_32);
			return;
		}
	#endif

	const char *use_name = full_name;
	char temp_name[MAX_FILENAME + 6];	// temp_name gets the thread id as an extension
	temp_name[0] = 0;

	if (SD.exists(full_name))
	{
		File check_file = SD.open(full_name);
		if (check_file.isDirectory())
		{
			fileReplyError(fsd,req_num,"FILE(%s) is a directory",full_name);
			check_file.close();
			return;
		}
		check_file.close();
		strcpy(temp_name,full_name);
		sprintf(&temp_name[name_len],".%d",threads.id());
		use_name = temp_name;
	}

	//------------------------------------------
	// allocate buffer and open file
	//------------------------------------------

	char *decoded_buf = (char *) malloc(MAX_DECODED_BUF);
	if (!decoded_buf)
	{
		fileReplyError(fsd,req_num,"FILE could not allocated DECODED_BUF");
		return;
	}

	if (!makeSubdirs(fsd,req_num,use_name))
		return;

	myFile_t the_file = SD.open(use_name,FILE_WRITE);
	if (!the_file)
	{
		fileReplyError(fsd,req_num,"could not open FILE(%s) for output",full_name);
		free(decoded_buf);
		return;
	}

	//------------------------------------------
	// loop sending CONTINE and getting BASE64
	//------------------------------------------

	bool ok = 1;
	int32_t offset = 0;
	while (ok && offset < size)
	{
		// send CONTINUE and wait for the BASE64
		// anything else consitutes an error and stops the transfer

		fileReply(fsd,req_num,"CONTINUE");
		char *buf = waitReply(req_num,"FILE");
		if (!buf)
		{
			ok = 0;
		}
		else
		{
			if (strncmp(buf,"BASE64",6))
			{
				my_error("FILE(%s) offset(%d) got: %s",full_name,offset,buf);
				ok = 0;
			}
			else
			{
				//------------------------------------
				// got a BASE64 packet
				//------------------------------------

				const char *param[MAX_FILE_PARAMS];
				int num_params = parseCommand(buf, &param[0]);

				int32_t got_offset = atol(param[0]);
				int32_t got_size   = atol(param[1]);	// limited to 10000
				int32_t encoded_size = strlen(param[2]);
				display_level(dbg_cmd+1,3,"BASE64(%ld,%ld) encoded=%ld bytes",got_offset,got_size,encoded_size);

				int32_t expected_size = size - offset;
				if (expected_size > MAX_FILE_BUF)
					expected_size = MAX_FILE_BUF;

				if (num_params != 3)
				{
					fileReplyError(fsd,req_num,"BASE64 expects 3 params got(%d)",num_params);
					ok = 0;
				}
				else if (got_size != expected_size)
				{
					fileReplyError(fsd,req_num,"BASE64 got_size(%ld) but expected(%ld)",got_size,expected_size);
					ok = 0;
				}
				else if (got_offset != offset)
				{
					fileReplyError(fsd,req_num,"BASE64 got_offset(%ld) but expected(%ld)",got_offset,offset);
					ok = 0;
				}

				//--------------------------
				// decode && write
				//--------------------------

				else
				{
					int32_t decoded_size = base64_decode(decoded_buf,(char *) param[2],encoded_size);
					if (decoded_size != got_size+4)
					{
						fileReplyError(fsd,req_num,"BASE64 decoded_size(%ld) but expected(%ld)",decoded_size,got_size+4);
						ok = 0;
					}
					else
					{
						uint32_t got_cs = 0;
						uint8_t *cs_ptr = (uint8_t *) &decoded_buf[got_size];
						for (int i=0; i<4; i++)
						{
							got_cs <<= 8;
							got_cs += *cs_ptr++;
						}
						decoded_buf[got_size] = 0;
						uint32_t calc_cs = calcChecksum((uint8_t *) decoded_buf);
						display_level(dbg_cmd+1,4,"got_cs(0x%08x) calc_cs(0x%08x)",got_cs,calc_cs);

						if (got_cs != calc_cs)
						{
							fileReplyError(fsd,req_num,"BASE64 checksum error got_cs(0x%08x) calc_cs(0x%08x)",got_cs,calc_cs);
							ok = 0;
						}
						else
						{
							int32_t bytes_written = the_file.write(decoded_buf,got_size);
							if (bytes_written != got_size)
							{
								fileReplyError(fsd,req_num,"BASE64(%s) file write error at(%ld) wrote(%ld) expected(%ld)",
									use_name,
									got_offset,
									bytes_written,
									got_size);
								ok = 0;

							}	// error writing to file
						}	// write to file

						offset += got_size;

					}	// correct decoded size
				}	// decode & write
			}	// got a BASE64 packet

			free(buf);

		}	// got a buff
	}	// while ok && offset < size

	//-----------------------------------------------------
	// finished
	//-----------------------------------------------------
	// rename temp_name to full_name if needed
	// 		and set the timeStamp()
	// or remove the file on any errors

	free(decoded_buf);

	if (ok)
	{
		setTimeStamp(the_file,ts);

		the_file.close();
		if (temp_name[0])
		{
			if (!SD.remove(full_name))
			{
				fileReplyError(fsd,req_num,"FILE could not remove old(%s)",full_name);
				ok = 0;
			}
			else
			{
				// threads.delay(50);		// yield thread for 100 ms
				delay(50);
					// fixes a bug with certain files not being
					// able to be renamed
				if (!SD.rename(temp_name,full_name))
				{
					fileReplyError(fsd,req_num,"FILE could not rename(%s) to(%s)",
						temp_name,
						full_name);
					ok = 0;
				}
			}
		}
	}
	else
	{
		the_file.close();
		SD.remove(use_name);
	}

	// done ...

	if (ok)
		fileReply(fsd,req_num,"OK");
}



//-------------------------------------------------------
// session-like PUT command
//-------------------------------------------------------

static bool _putFile(
	Stream *fsd,
	int req_num,
	char *decoded_buf,
	char *encoded_buf,
	const char *dir,
	const char *target_dir,
	const char *entry,
	int level = 0)
	// note that BASE64 handles the client progress messages
{
	display_level(dbg_cmd,3+level,"_putFile(%s,%s,%s)",dir,target_dir,entry);
	char path[MAX_FILENAME];
	if (!makePath(fsd,req_num,path,dir,entry))
		return 0;

	// open the file for input

	myFile_t the_file = SD.open(path);
	if (!the_file)
	{
		fileReplyError(fsd,req_num,"_putFile could not open %s for input",path);
		return 0;
	}
	else if (the_file.isDirectory())
	{
		fileReplyError(fsd,req_num,"%s is a directory in _putFile",path);
		the_file.close();
		return 0;
	}
	if (!makePath(fsd,req_num,path,target_dir,entry))
	{
		the_file.close();
		return 0;
	}

	// send the FILE command

	bool ok = 0;
	int32_t offset = 0;
	int32_t size = the_file.size();
	const char *ts = getTimeStamp(&the_file);

	fileReply(fsd,req_num,"FILE\t%d\t%s\t%s",size,ts,path);

	#if WITH_LOCAL_PROGRESS
		fileReply(fsd,req_num,"PROGRESS\tENTRY\t%s\t%d",path,size);
	#endif

	while (1)
	{
		char *buf = waitReply(req_num,"_putFile");
		if (!buf)
		{
			break;
		}
		else
		{
			bool is_ok = !strncmp(buf,"OK",2);
			bool is_continue = !strncmp(buf,"CONTINUE",8);
			if (!is_ok && !is_continue)
			{
				my_error("FILE(%s) offset(%d) got: %s",path,offset,buf);
				break;
			}
			else if (is_ok)
			{
				if (offset < size)
					fileReplyError(fsd,req_num,"unexpected OK with offset(%ld) and size(%ld)",offset,size);
				break;
			}
			else if (offset >= size)
			{
				fileReplyError(fsd,req_num,"expected OK with offset(%ld) and size(%ld)",offset,size);
				break;
			}

			// read, checksum, encode, and send

			else	// is_continue
			{
				// read

				int32_t get = size - offset;
				if (get > MAX_DECODED_BUF - 5)
					get = MAX_DECODED_BUF - 5;

				#if WITH_LOCAL_PROGRESS
					fileReply(fsd,req_num,"PROGRESS\tBYTES\t%d",offset+get);
				#endif

				int32_t got = the_file.read(decoded_buf,get);
				if (got != get)
				{
					fileReplyError(fsd,req_num,"file read error at(%ld) got(%ld) expected(%ld)",offset,got,get);
					break;
				}
				else
				{
					// checksum

					decoded_buf[get] = 0;
					uint32_t calc_cs = calcChecksum((uint8_t *) decoded_buf);
					display_level(dbg_cmd+1,4+level,"calc_cs(0x%08x)",calc_cs);
					uint8_t *cs_ptr = (uint8_t *) &decoded_buf[get];
					for (int i=0; i<4; i++)
					{
						uint8_t byte = (calc_cs >> (3-i)*8) & 0xff;
						*cs_ptr++ = byte;
					}
					*cs_ptr = 0;

					// encode

					sprintf(encoded_buf,"file_reply(%d):BASE64\t%ld\t%ld\t",req_num,offset,get);
					char *out = &encoded_buf[strlen(encoded_buf)];
					base64_encode(out,decoded_buf,get+4);
						// assume encoding works

					// send the packet

					strcat(encoded_buf,"\r\n");
					fsd->print(encoded_buf);
					offset += get;
					ok = 1;

				}	// got == get
			}	// is_continue
		}	// no timeout waiting for buf
	}	// while (1)

	the_file.close();

	#if WITH_LOCAL_PROGRESS
		fileReply(fsd,req_num,"PROGRESS\tDONE\t0\t1");
	#endif

	display_level(dbg_cmd,3+level,"_putFile(%s,%s,%s) returning %d",dir,target_dir,entry,ok);
	return ok;
}



static bool _putDir(
	Stream *fsd,
	int req_num,
	char *decoded_buf,
	char *encoded_buf,
	const char *dir,
	const char *target_dir,
	const char *entry,
	int level = 0)
{
	display_level(dbg_cmd,2+level,"_putDir(%d,%s,%s,%s)",level,dir,target_dir,entry);

	if (level >= MAX_RECURSION_DEPTH)
	{
		fileReplyError(fsd,req_num,"too many(%d) nested directory levels",level);
		return 0;
	}
	char path[MAX_FILENAME];
	char target_path[MAX_FILENAME];
	if (!makePath(fsd,req_num,path,dir,entry))
		return 0;
	if (!makePath(fsd,req_num,target_path,target_dir,entry))
		return 0;

	myFile_t the_dir = SD.open(path);
	if (!the_dir)
	{
		fileReplyError(fsd,req_num,"_putDir(%s) could not open directory",path);
		return 0;
	}
	if (!the_dir.isDirectory())
	{
		fileReplyError(fsd,req_num,"_putDir(%s) is not a directory",path);
		the_dir.close();
		return 0;
	}

	if (level)
	{
		fileReply(fsd,req_num,"MKDIR\t%s\t%s\t1",
			target_dir,getTimeStamp(&the_dir));
		char *buf = waitReply(req_num,"putDir(MKDIR)");
		if (!buf || strncmp(buf,"OK",2))
		{
			the_dir.close();
			return 0;
		}
	}

	#if WITH_LOCAL_PROGRESS
		fileReply(fsd,req_num,"PROGRESS\tDONE\t1\t0");
	#endif

	// many PROGRESS ADDS

	bool ok = 1;
	myFile_t dir_entry = the_dir.openNextFile();
	while (ok && dir_entry)
	{
		char name[255];
		dir_entry.getName(name, sizeof(name));
		bool is_dir = dir_entry.isDirectory();
		display_level(dbg_cmd+2,4+level,"got is_dir(%d) name(%s)",is_dir,name);

		fileReply(fsd,req_num,"PROGRESS\tADD\t%d\t%d",is_dir,!is_dir);

		if (is_dir)
		{
			ok = _putDir(
				fsd,
				req_num,
				decoded_buf,
				encoded_buf,
				path,
				target_path,
				name,
				level+1);
		}
		else
		{
			ok = _putFile(
				fsd,
				req_num,
				decoded_buf,
				encoded_buf,
				path,
				target_path,
				name,
				level);
		}

		dir_entry = the_dir.openNextFile();

	}   // while (entry)

	the_dir.close();
	display_level(dbg_cmd,2+level,"_putDir(%d,%s,%s,%s) returning %d",level,dir,target_dir,entry,ok);
	return ok;
}



//=========================================================
// fileCommand()
//=========================================================

void fileCommand(int req_num)
	// buf is pointing at req_num \t
	// the buf we are passed must be freed when done!!
{
    display_level(dbg_hdr+1,1,"fileCommand(%d)",req_num);
	Stream *fsd = ACTIVE_FILE_SYS_DEVICE;

	// get and parse the command buffer
	// command must be freed after this


	const char *entries;
	const char *param[MAX_FILE_PARAMS];
	char *command = getCommandQueue(req_num);
	if (!command)
	{
		my_error("no file_command(%d) queue",req_num);
		return;
	}

	int num_params = parseCommand(command, &param[0], &entries);

	char dbg_buf[18];
	const char *dbg_ptr = param[2];
	if (!strcmp(command,"BASE64"))
	{
		sprintf(dbg_buf,"bytes(%d)",strlen(dbg_ptr));
		dbg_ptr = dbg_buf;
	}
    display_level(dbg_hdr,1,"fileCommand(%d) %s(%s,%s,%s)",
		req_num,
		command,
		param[0],
		param[1],
		dbg_ptr);

	//--------------------------
	// parse entries if any
	//--------------------------

	textEntry_t the_entry;
	const char *ptr = entries;

	int num_dirs = 0;
	int num_files = 0;
	int rslt = getNextEntry(fsd,req_num,&the_entry,&ptr);
	while (rslt == 1)
	{
		if (the_entry.is_dir)
			num_dirs++;
		else
			num_files++;

		display_level(dbg_entries,3,"entry(%s) is_dir(%d) size(%s) ts(%s)",
			the_entry.entry, the_entry.is_dir, the_entry.size, the_entry.ts);
		rslt = getNextEntry(fsd,req_num,&the_entry,&ptr);
	}
	if (rslt == -1)	// error already reported
	{
		free(command);
		return;
	}

	//--------------------------------------
	// do the commands
	//--------------------------------------

	if (!strcmp(command,"HELLO"))
	{
		fileReply(fsd,req_num,"WASSUP\t%s",getUSBSerialNum());
	}
	else if (!strcmp(command,"LIST"))
	{
		_list(fsd,req_num,param[0]);
	}
	else if (!strcmp(command,"MKDIR"))
	{
		_mkdir(fsd,req_num,param[0],param[1],param[2]);
	}
	else if (!strcmp(command,"RENAME"))
	{
		_rename(fsd,req_num,param[0],param[1],param[2]);
	}
	else if (!strcmp(command,"FILE"))
	{
		// FILE contains the BASE64 sub-session
		_file(fsd,req_num,param[0],param[1],param[2]);
	}

	// session like PUT and DELETE commands

	else if (!strcmp(command,"PUT") ||
			 !strcmp(command,"DELETE"))
	{
		bool ok = 1;
		bool is_put = !strcmp(command,"PUT");
		char *decoded_buf = 0;
		char *encoded_buf = 0;

		if (is_put)
		{
			decoded_buf = (char *) malloc(MAX_DECODED_BUF);
			if (!decoded_buf)
			{
				ok = 0;
				fileReplyError(fsd,req_num,"Could not allocate decoded_buf");
			}
			encoded_buf = (char *) malloc(MAX_ENCODED_BUF);
			if (!encoded_buf)
			{
				ok = 0;
				fileReplyError(fsd,req_num,"Could not allocate encoded_buf");
			}
		}

		if (ok)
		{
			// single_file item

			if ((!is_put && num_params == 2) ||
				(is_put && num_params == 3))
			{
				ok = is_put ?
					_putFile(fsd,req_num,decoded_buf,encoded_buf,param[0],param[1],param[2]) :
					_delete(fsd,req_num,param[0],param[1]);
			}

			// entries

			else if (ok && !abortPending(fsd, req_num, command))
			{
				// process entry list
				fileReply(fsd,req_num,"PROGRESS\tADD\t%d\t%d",num_dirs,num_files);

				ptr = entries;
				int cont = getNextEntry(fsd,req_num,&the_entry,&ptr);
				while (ok && cont == 1)
				{
					if (abortPending(fsd, req_num, command))
						break;

					ok = is_put ? the_entry.is_dir ?
						_putDir(fsd,req_num,decoded_buf,encoded_buf,param[0],param[1],the_entry.entry) :
						_putFile(fsd,req_num,decoded_buf,encoded_buf,param[0],param[1],the_entry.entry) :
						_delete(fsd,req_num,param[0],the_entry.entry);

					if (ok)
					{
						cont = getNextEntry(fsd,req_num,&the_entry,&ptr);
						display(0,0,"cont=%d",cont);
					}
				}
				ok = cont == -1 ? 0 : ok;
			}

			if (decoded_buf)
				free(decoded_buf);
			if (encoded_buf)
				free(encoded_buf);

			if (ok)
			{
				if (is_put)
					fileReply(fsd,req_num,"OK");
				else
					_list(fsd,req_num,param[0]);
			}

		}	// allocated buffers
	}	// PUT or DELETE

	// Unknown Command

	else
	{
		fileReplyError(fsd,req_num,"Unknown Command %s",command);
	}

	display_level(dbg_hdr,1,"fileCommand(%d,%s) done",req_num,command);
	free(command);
	endCommand(req_num);

}	// fileCommand


// end of fileCommand.cpp
