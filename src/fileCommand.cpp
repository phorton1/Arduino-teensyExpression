//----------------------------------------------------------
// fileCommand.cpp
//----------------------------------------------------------
// contains fileCommand() and associated methods
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.

#include <Arduino.h>

#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>
#include <TeensyThreads.h>
#include <Base64.h>


#define dbg_hdr	  -1
	//  0 = show a msg after parsing and at end of command
	// -1 = show msg at top of doCommand
#define dbg_cmd	  -1
	//  0 = show file commands
	// -1 = show command details
#define dbg_entries  0
	//  0 = show command entries as parsed
#define dbg_wait  0
	//  0 = show waitReply details (and slow it down)

#define TEST_DELAY    0		// 2000
	// delay certain operations to test progress dialog etc
	// set to 1000 or 2000 ms to slow things down

#define WITH_LOCAL_PROGRESS  	1
	// i envision a scheme where servers send minimal
	// progress messags. This defines those that could
	// be done on the other side with knowlege of both sessions.


#define FILE_TIMEOUT  15000	  // ms
	// time to wait for next BASE64 packet in FILE command

#define MAX_RECURSION_DEPTH  6
	// see notes in on thread stack size in fileUtils.cpp

#define SIZE_TIMESTAMP	   20
	// YYYY-MM-DD HH:MM:SS plus nul terminator
#define MAX_FILENAME    255
	// maximum length of fully qualified filename supported by fileCommands






//---------------------------------
// common utilities
//---------------------------------

static bool makePath(Stream *fsd, fileCommand_t *cmd, char *buf, const char *dir, const char *entry)
{
	if (strlen(dir) + strlen(entry) + 1 >= MAX_FILENAME)
	{
		fileReplyError(fsd, cmd, "path too long(%d)!!",strlen(dir) + strlen(entry) + 1);
		return 0;
	}
	buf[0] = 0;
	strcat(buf,dir);
	if (strcmp(dir,"/"))
		strcat(buf,"/");
	strcat(buf,entry);
	return 1;
}

static uint32_t calcChecksum(const uint8_t *buf,int len)
{
	uint32_t cs = 0;
	while (len--) { cs += *buf++; }
	return cs;
}


static char *waitReply(fileCommand_t *cmd, const char *command_name)
{
	display_level(dbg_cmd,1,"waitReply(%d,%s)",cmd->req_num,command_name);
	uint32_t wait = millis();
	char *buf = getCommandQueue(cmd);
	while (!buf && millis() - wait < FILE_TIMEOUT)
	{
		threads.delay(100);		// yield thread for 100 ms
		buf = getCommandQueue(cmd);
	}
	if (!buf)
		my_error("fileCommand::waitReply(%s) timeout",command_name);
	else
	{
		warning(dbg_malloc,"waitReply() returning buf at 0x%08x",(uint32_t) buf);
		if (!strncmp(buf,"BASE64",6))
			display_level(dbg_cmd+2,0,"waitReply got: BASE64 packet_len(%d)",strlen(buf))
		else
			display_level(dbg_cmd+2,0,"waitReply got: %s",buf)
	}
	display_level(dbg_cmd,1,"waitReply(%d,%s) returning 0x%08x",cmd->req_num,command_name,(uint32_t) buf);
	return buf;
}


//---------------------------------------------
// simple commands
//---------------------------------------------

static void _list(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *dir)
{
	display_level(dbg_cmd,2,"LIST(%d,%s)",cmd->req_num,dir);

	myFile_t the_dir = SD.open(dir);
	if (!the_dir)
	{
		my_error("could not opendir(%s)",dir);
		delay(1000);
		fileReplyError(fsd,cmd,"LIST(%s) could not open directory",dir);
		return;
	}

	char *dir_buffer = cmd->dir_buffer;
	const char *ts = getTimeStamp(&the_dir);

	sprintf(dir_buffer,"file_reply(%d):0\t%s\t0\t0\t0\t%s%s\r",
		cmd->req_num,
		ts,
		dir,
		strcmp(dir,"/")?"/":"");

	unsigned int at = strlen(dir_buffer);
	char *out = &dir_buffer[at];

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
			my_error("LIST(%s) not reporting directory buffer overflow at %d!",dir,at);
			break;
		}

		sprintf(out,"%ld\t%s\t0\t0\t0\t%s%s\r",size,ts,name,is_dir?"/":"");
		at += strlen(out);
		out = &dir_buffer[at];
		entry = the_dir.openNextFile();

	}   // while (entry)

	the_dir.close();
	sprintf(out,"\n");
	fsd->printf(dir_buffer);
}


static void _mkdir(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *path,
	const char *ts,
	const char *may_exist)
{
	display_level(dbg_cmd,2,"MKDIR(%s,%s,%s)",path,ts,may_exist);
	bool use_exist = *may_exist && *may_exist != '0';
	if (SD.exists(path))
	{
		if (use_exist)
		{
			myFile_t check_file = SD.open(path);
			if (!check_file || !check_file.isDirectory())
				fileReplyError(fsd,cmd,"MKDIR %s is not a directory",path);
			else
				fileReply(fsd,cmd,"OK");
			return;
		}

		fileReplyError(fsd,cmd,"MKDIR %s already exists",path);
		return;
	}

	if (mkDirTS(path,ts))
	{
		if (use_exist)
			fileReply(fsd,cmd,"OK");
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
			_list(fsd,cmd,pppp);
		}
	}
	else
		fileReplyError(fsd,cmd,"MKDIR could not make directory %s",path);
}



static void _rename(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *dir,
	const char *name1,
	const char *name2)
{
	// names already have trailing /'s removed

	char path1[MAX_FILENAME + 1];
	char path2[MAX_FILENAME + 1];
	if (!makePath(fsd,cmd,path1,dir,name1) ||
		!makePath(fsd,cmd,path2,dir,name2))
		return;

	myFile_t file = SD.open(path1);
	if (!file)
	{
		fileReplyError(fsd,cmd,"RENAME Could not open %s",name1);
		return;
	}
	bool is_dir = file.isDirectory();
	const char *ts = getTimeStamp(&file);
	uint32_t size = file.size();
	file.close();

	display_level(dbg_cmd,2,"RENAME(%s,%s,%s) path1=%s path2=%s",dir,name1,name2,path1,path2);

	if (SD.rename(path1,path2))
		fileReply(fsd,cmd,"%d\t%s\t0\t0\t0\t%s%s",size,ts,name2,is_dir?"/":"");
	else
		fileReplyError(fsd,cmd,"Could not RENAME %s to %s",name1,name2);
}



//---------------------------------------------
// session-like DELETE command
//---------------------------------------------

static bool abortPending(Stream *fsd, fileCommand_t *cmd)
{
	char *pending = getCommandQueue(cmd);
	if (pending)
	{
		if (!strncmp(pending,"ABORT",5))
		{
			display_level(dbg_cmd,3,"ABORTING fileCommand(%d,%s)!!",cmd->req_num,cmd->command);
			fileReply(fsd,cmd,"ABORTED");
		}
		warning(dbg_malloc,"abortPending() freeing 'pending' (buffer) at 0x%08x",(uint32_t) pending);
		free(pending);
		return true;
	}
	return false;
}



static bool _delete(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *dir,
	const char *entry)
{
	char path[MAX_FILENAME];
	if (!makePath(fsd,cmd,path,dir,entry))
		return 0;
    display_level(dbg_cmd,2,"DELETE(%s)",path);
	fileReply(fsd,cmd,"PROGRESS\tENTRY\t%s",entry);

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
			fileReply(fsd,cmd,"PROGRESS\tDONE\t%d",is_dir);
		else
			fileReplyError(fsd,cmd,"could not DELETE %s",path);
	}
	else
	{
		ok = 0;
		fileReplyError(fsd,cmd,"DELETE could not open %s",path);
	}

	return ok;
}




//-------------------------------------------------------
// session-like FILE command
//-------------------------------------------------------
// contains BASE64 loop

bool makeSubdirs(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *in)
	// probably not needed now that PUT protocol includes MKDIR
	// expects a fully qualified path name starting with /
	// with a leaf terminal filename
	// makes any needed subdirectories for the file
{
	display_level(dbg_cmd+1,3,"makeSubdirs(%s)",in);
	if (!in || !*in || *in != '/')
	{
		fileReplyError(fsd,cmd,"filename(%s) must be fully qualified",in);
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
					fileReplyError(fsd,cmd,"attempt to overwrite file(%s) with a subdirectory",path);
					return 0;
				}
			}
			else
			{
				display_level(dbg_cmd+1,3,"FILE making subdir(%s)",path);
				if (!SD.mkdir(path))
				{
					fileReplyError(fsd,cmd,"could not create subdirectory(%s)",path);
					return 0;
				}
			}
		}
		*out++ = *in++;
	}

	return 1;

}



static void _file(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *sz_size,
	const char *ts,
	const char *full_name)
{
	int32_t size = atol(sz_size);
	display_level(dbg_cmd,2,"FILE(%s,%ld,%s)",full_name,size,ts);

	//--------------------------------
	// validate _file request
	//--------------------------------

	int name_len = strlen(full_name);
	if ( name_len >= MAX_FILENAME)
	{
		fileReplyError(fsd,cmd,"FILE file(%s) name(%d) too long",full_name,name_len);
		return;
	}

	const char *use_name = full_name;
	char temp_name[MAX_FILENAME + 6];	// temp_name gets the thread id as an extension
	temp_name[0] = 0;

	if (SD.exists(full_name))
	{
		File check_file = SD.open(full_name);
		if (check_file.isDirectory())
		{
			fileReplyError(fsd,cmd,"FILE(%s) is a directory",full_name);
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

	if (!makeSubdirs(fsd,cmd,use_name))
		return;

	myFile_t the_file = SD.open(use_name,FILE_WRITE);
	if (!the_file)
	{
		fileReplyError(fsd,cmd,"could not open FILE(%s) for output",full_name);
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

		fileReply(fsd,cmd,"CONTINUE");
		char *buf = waitReply(cmd,"FILE");
		warning(dbg_malloc,"_file got waitReply buf 0x%08x",(uint32_t) buf);

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

				fileCommand_t cmd2;
				if (parseCommand(&cmd2,cmd->req_num,buf))
				{
					int32_t got_offset = atol(cmd2.params[0]);
					int32_t got_size   = atol(cmd2.params[1]);	// limited to 10000
					char *encoded_buf = (char *) cmd2.params[2];
					int32_t encoded_size = strlen(encoded_buf);
					display_level(dbg_cmd+1,3,"BASE64(%ld,%ld) encoded=%ld bytes",got_offset,got_size,encoded_size);

					int32_t expected_size = size - offset;
					if (expected_size > MAX_FILE_BUF)
						expected_size = MAX_FILE_BUF;

					if (cmd->num_params != 3)
					{
						fileReplyError(fsd,cmd,"BASE64 expects 3 params got(%d)",cmd2.num_params);
						ok = 0;
					}
					else if (got_size != expected_size)
					{
						fileReplyError(fsd,cmd,"BASE64 got_size(%ld) but expected(%ld)",got_size,expected_size);
						ok = 0;
					}
					else if (got_offset != offset)
					{
						fileReplyError(fsd,cmd,"BASE64 got_offset(%ld) but expected(%ld)",got_offset,offset);
						ok = 0;
					}

					//--------------------------
					// decode && write
					//--------------------------

					else
					{
						char *decoded_buf = cmd->decoded_buffer;
						int32_t decoded_size = base64_decode(decoded_buf,encoded_buf,encoded_size);
						if (decoded_size != got_size+4)
						{
							fileReplyError(fsd,cmd,"BASE64 decoded_size(%ld) but expected(%ld)",decoded_size,got_size+4);
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
							uint32_t calc_cs = calcChecksum((uint8_t *) decoded_buf,got_size);
							display_level(dbg_cmd+1,4,"got_cs(0x%08x) calc_cs(0x%08x)",got_cs,calc_cs);

							if (got_cs != calc_cs)
							{
								fileReplyError(fsd,cmd,"BASE64 checksum error got_cs(0x%08x) calc_cs(0x%08x)",got_cs,calc_cs);
								ok = 0;
							}
							else
							{
								int32_t bytes_written = the_file.write(decoded_buf,got_size);
								if (bytes_written != got_size)
								{
									fileReplyError(fsd,cmd,"BASE64(%s) file write error at(%ld) wrote(%ld) expected(%ld)",
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
				}	// parseCommand(cmd2)
				else
				{
					fileReplyError(fsd,cmd,"ill formed BASE64 packet");
				}

			}	// got a BASE64 packet

			warning(dbg_malloc,"_file freeing buf at 0x%08x",(uint32_t) buf);
			free(buf);

		}	// got a buff
	}	// while ok && offset < size

	//-----------------------------------------------------
	// finished
	//-----------------------------------------------------
	// rename temp_name to full_name if needed
	// 		and set the timeStamp()
	// or remove the file on any errors

	if (ok)
	{
		setTimeStamp(the_file,ts);

		the_file.close();
		if (temp_name[0])
		{
			if (!SD.remove(full_name))
			{
				fileReplyError(fsd,cmd,"FILE could not remove old(%s)",full_name);
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
					fileReplyError(fsd,cmd,"FILE could not rename(%s) to(%s)",
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
		fileReply(fsd,cmd,"OK");
}



//-------------------------------------------------------
// session-like PUT command
//-------------------------------------------------------

static bool _putFile(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *dir,
	const char *target_dir,
	const char *entry,
	int level = 0)
	// note that BASE64 handles the client progress messages
{
	display_level(dbg_cmd,3+level,"_putFile(%s,%s,%s)",dir,target_dir,entry);
	char path[MAX_FILENAME];
	if (!makePath(fsd,cmd,path,dir,entry))
		return 0;

	// open the file for input

	myFile_t the_file = SD.open(path);
	if (!the_file)
	{
		fileReplyError(fsd,cmd,"_putFile could not open %s for input",path);
		return 0;
	}
	else if (the_file.isDirectory())
	{
		fileReplyError(fsd,cmd,"%s is a directory in _putFile",path);
		the_file.close();
		return 0;
	}
	if (!makePath(fsd,cmd,path,target_dir,entry))
	{
		the_file.close();
		return 0;
	}

	// send the FILE command

	bool ok = 0;
	int32_t offset = 0;
	int32_t size = the_file.size();
	const char *ts = getTimeStamp(&the_file);

	fileReply(fsd,cmd,"FILE\t%d\t%s\t%s",size,ts,path);

	#if WITH_LOCAL_PROGRESS
		fileReply(fsd,cmd,"PROGRESS\tENTRY\t%s\t%d",path,size);
	#endif

	while (1)
	{
		char *buf = waitReply(cmd,"_putFile");
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
					fileReplyError(fsd,cmd,"unexpected OK with offset(%ld) and size(%ld)",offset,size);
				break;
			}
			else if (offset >= size)
			{
				fileReplyError(fsd,cmd,"expected OK with offset(%ld) and size(%ld)",offset,size);
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
					fileReply(fsd,cmd,"PROGRESS\tBYTES\t%d",offset+get);
				#endif

				char *decoded_buf = cmd->decoded_buffer;
				int32_t got = the_file.read(decoded_buf,get);
				if (got != get)
				{
					fileReplyError(fsd,cmd,"file read error at(%ld) got(%ld) expected(%ld)",offset,got,get);
					break;
				}
				else
				{
					// checksum

					decoded_buf[get] = 0;
					uint32_t calc_cs = calcChecksum((uint8_t *) decoded_buf,get);
					display_level(dbg_cmd+1,4+level,"calc_cs(0x%08x)",calc_cs);
					uint8_t *cs_ptr = (uint8_t *) &decoded_buf[get];
					for (int i=0; i<4; i++)
					{
						uint8_t byte = (calc_cs >> (3-i)*8) & 0xff;
						*cs_ptr++ = byte;
					}
					*cs_ptr = 0;

					// encode

					char *encoded_buf = cmd->encoded_buffer;
					sprintf(encoded_buf,"file_reply(%d):BASE64\t%ld\t%ld\t",cmd->req_num,offset,get);
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
		fileReply(fsd,cmd,"PROGRESS\tDONE\t0\t1");
	#endif

	display_level(dbg_cmd,3+level,"_putFile(%s,%s,%s) returning %d",dir,target_dir,entry,ok);
	return ok;
}



static bool _putDir(
	Stream *fsd,
	fileCommand_t *cmd,
	const char *dir,
	const char *target_dir,
	const char *entry,
	int level = 0)
{
	display_level(dbg_cmd,2+level,"_putDir(%d,%s,%s,%s)",level,dir,target_dir,entry);

	if (level >= MAX_RECURSION_DEPTH)
	{
		fileReplyError(fsd,cmd,"too many(%d) nested directory levels",level);
		return 0;
	}
	char path[MAX_FILENAME];
	char target_path[MAX_FILENAME];
	if (!makePath(fsd,cmd,path,dir,entry))
		return 0;
	if (!makePath(fsd,cmd,target_path,target_dir,entry))
		return 0;

	myFile_t the_dir = SD.open(path);
	if (!the_dir)
	{
		fileReplyError(fsd,cmd,"_putDir(%s) could not open directory",path);
		return 0;
	}
	if (!the_dir.isDirectory())
	{
		fileReplyError(fsd,cmd,"_putDir(%s) is not a directory",path);
		the_dir.close();
		return 0;
	}

	if (level)
	{
		fileReply(fsd,cmd,"MKDIR\t%s\t%s\t1",
			target_dir,getTimeStamp(&the_dir));
		char *buf = waitReply(cmd,"putDir(MKDIR)");
		if (!buf || strncmp(buf,"OK",2))
		{
			the_dir.close();
			return 0;
		}
	}

	#if WITH_LOCAL_PROGRESS
		fileReply(fsd,cmd,"PROGRESS\tDONE\t1\t0");
	#endif

	// many PROGRESS ADDS

	bool ok = 1;
	myFile_t dir_entry = the_dir.openNextFile();
	while (ok && dir_entry)
	{
		char name[MAX_FILENAME];
		dir_entry.getName(name, sizeof(name));
		bool is_dir = dir_entry.isDirectory();
		display_level(dbg_cmd+2,4+level,"got is_dir(%d) name(%s)",is_dir,name);

		fileReply(fsd,cmd,"PROGRESS\tADD\t%d\t%d",is_dir,!is_dir);

		if (is_dir)
		{
			ok = _putDir(
				fsd,
				cmd,
				path,
				target_path,
				name,
				level+1);
		}
		else
		{
			ok = _putFile(
				fsd,
				cmd,
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



//------------------------------------------------
// textEntry_t parser
//------------------------------------------------

typedef struct
{
	char size[10];
	char ts[22];
	char entry[255];
	bool is_dir;
}   textEntry_t;


static int getNextEntry(Stream *fsd, fileCommand_t *cmd, textEntry_t *the_entry, const char **ptr)
	// parser for commands that have lists of entries
	// pass in ptr, starting at the list of entries
	// returns 0 if no entry, -1 if error, or 1 if entry
{
	the_entry->size[0] = 0;
	the_entry->ts[0] = 0;
	the_entry->entry[0] = 0;
	the_entry->is_dir = 0;

	if (!**ptr)
		return 0;

	// We must get six fields or we fail, but note that
	// we skip the unix MODE,USER,GROUP fields while parsing.
	// There is always a \t or \r after each entry.

	int num_params = 0;
	char *out = the_entry->size;
	while (**ptr)
	{
		char c = *(*ptr)++;

		if (c == '\t' ||
			c == '\r')
		{
			num_params++;
			if (out) *out = 0;
			bool is_cr = (c == '\r');
			// get rid of terminating '/' on dir entries
			if (num_params == 6 && *(out-1) == '/')
			{
				the_entry->is_dir = 1;
				*(out-1) = 0;
			}
			if (is_cr) break;

			out = 0;
			if (num_params == 1)
				out  = the_entry->ts;
			else if (num_params == 5)
				out = the_entry->entry;
		}
		else if (out)
		{
			*out++ = c;
		}
	}

	if (num_params != 6)
	{
		fileReplyError(fsd,cmd,"Incorrect number of fields(%d) in fileEntry",num_params);
		return -1;
	}

	return 1;
}



//=========================================================
// fileCommand()
//=========================================================
// this method is run as a teensy thread that only knows
// the request number it is associated with.  It usees
// that request number to initially get the 0th queue
// element, which is the buffer passed in from the system,
// which it then parses to get the command, parameters, and
// entries.


void fileCommand(void *vptr)
	// the buffer (command) returned by getCommandQueue must be freed by this method,
	// AND endCommand(req_num) must be called at some point
{
	fileCommand_t *cmd = (fileCommand_t *) vptr;
	const char *command = cmd->command;
    display_level(dbg_hdr+1,1,"fileCommand(%d) %s(%s,%s,%s) num_params=%d",
		cmd->req_num,
		command,
		cmd->params[0],
		cmd->params[1],
		cmd->params[2],
		cmd->num_params);
	MEM_INFO("fileCommand");

	Stream *fsd = ACTIVE_FILE_SYS_DEVICE;

	// parse the entries, if any, to get num_dirs and
	// num_files, solely, at this time, for progress messages

	textEntry_t the_entry;
	const char *ptr = cmd->entries;

	int num_dirs = 0;
	int num_files = 0;
	int rslt = getNextEntry(fsd,cmd,&the_entry,&ptr);
	while (rslt == 1)
	{
		if (the_entry.is_dir)
			num_dirs++;
		else
			num_files++;

		display_level(dbg_entries,3,"entry(%s) is_dir(%d) size(%s) ts(%s)",
			the_entry.entry, the_entry.is_dir, the_entry.size, the_entry.ts);
		rslt = getNextEntry(fsd,cmd,&the_entry,&ptr);
	}

	if (!strcmp(command,"PUT") || !strcmp(command,"DELETE"))
		display_level(dbg_hdr+1,1,"parse1 num_dirs(%d) num_files(%d)",num_dirs,num_files);
	
	if (rslt == 0)	// valid end of entries
	{
		//--------------------------------------
		// do the commands
		//--------------------------------------

		if (!strcmp(command,"HELLO"))
		{
			fileReply(fsd,cmd,"WASSUP\t%s",getUSBSerialNum());
		}
		else if (!strcmp(command,"LIST"))
		{
			_list(fsd,cmd,cmd->params[0]);
		}
		else if (!strcmp(command,"MKDIR"))
		{
			_mkdir(fsd,cmd,cmd->params[0],cmd->params[1],cmd->params[2]);
		}
		else if (!strcmp(command,"RENAME"))
		{
			_rename(fsd,cmd,cmd->params[0],cmd->params[1],cmd->params[2]);
		}
		else if (!strcmp(command,"FILE"))
		{
			// FILE contains the BASE64 sub-session
			_file(fsd,cmd,cmd->params[0],cmd->params[1],cmd->params[2]);
		}

		// session like PUT and DELETE commands

		else if (!strcmp(command,"PUT") ||
				 !strcmp(command,"DELETE"))
		{
			bool ok = 1;
			bool is_put = !strcmp(command,"PUT");

			if ((!is_put && cmd->num_params == 2) ||
				(is_put && cmd->num_params == 3))
			{
				ok = is_put ?
					_putFile(fsd,cmd,cmd->params[0],cmd->params[1],cmd->params[2]) :
					_delete(fsd,cmd,cmd->params[0],cmd->params[1]);
			}

			// entries
			// This is where we actually parse the entries
			// for file operations.

			else if (!abortPending(fsd, cmd))
			{
				// process entry list
				fileReply(fsd,cmd,"PROGRESS\tADD\t%d\t%d",num_dirs,num_files);

				ptr = cmd->entries;
				int cont = getNextEntry(fsd,cmd,&the_entry,&ptr);
				while (ok && cont == 1)
				{
					if (abortPending(fsd, cmd))
						break;

					ok = is_put ? the_entry.is_dir ?
						_putDir(fsd,cmd,cmd->params[0],cmd->params[1],the_entry.entry) :
						_putFile(fsd,cmd,cmd->params[0],cmd->params[1],the_entry.entry) :
						_delete(fsd,cmd,cmd->params[0],the_entry.entry);

					if (ok)
						cont = getNextEntry(fsd,cmd,&the_entry,&ptr);
				}
				ok = cont == -1 ? 0 : ok;
			}

			if (ok)
			{
				if (is_put)
					fileReply(fsd,cmd,"OK");
				else
					_list(fsd,cmd,cmd->params[0]);
			}
		}	// PUT or DELETE

		// Unknown Command

		else
		{
			fileReplyError(fsd,cmd,"Unknown Command %s",command);
		}

	}	// valid end of entry list

	display_level(dbg_hdr,1,"fileCommand(%d,%s) done",cmd->req_num,cmd->command);
	cmd->done = 1;

}	// fileCommand


// end of fileCommand.cpp
