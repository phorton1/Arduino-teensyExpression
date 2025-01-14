//----------------------------------------------------------
// fileUtils.cpp
//----------------------------------------------------------
// contains command, queue, reply, and parse methods
// used by fileCommand.cpp

#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>
#include <TeensyThreads.h>


#define dbg_queue  0
	//  0 = show queue actions
#define dbg_parse  -1
	//  0 = show command parse header
	// -1 = show parameters found and entries
	// -2 = show every character in parseCommand

#define MAX_ACTIVE_COMMANDS 10
	// maximum number of simultaneously active commands

#define QUEUE_TIMEOUT    5		// ms
	// timeout for semaphore to get access to queue

#define THREAD_STACK_SIZE   8192
	// stack for doCommand()
	// MAX_RECURSION_DEPTH=8 in PUT
	// fileReplies use stack buffer of MAX_FILE_REPLY=1024
	// and each PUT recursion uses at least 2*MAX_FILENAME=512 buffers
	// on the stack, so with these defines, the absolute minimum
	// THREAD_STACK_SIZE is 5K

#define MAX_DIRECTORY_BUF   4096
	// maximum size of a directory listing returned by _list
#define MAX_FILE_REPLY		1024
	// allocated on stack!


//---------------------------------------------
// commands and queues
//---------------------------------------------


static volatile int command_sem;
static fileCommand_t *commands[MAX_ACTIVE_COMMANDS];
	// pointer to a command remains valid through it's lifetime

static bool missing_command_reported;
	// implentation only reports one per program invocation
	// for debugging.


static bool waitCommandSem(int sem_level)
{
	uint32_t started = millis();
	while (command_sem > sem_level && millis() - started < QUEUE_TIMEOUT)
	{
		delay(1);
	}
	if (command_sem <= sem_level)
	{
		command_sem++;
		return true;
	}
	my_error("timed out in waitCommandSem(%d)",sem_level);
	return false;
}


fileCommand_t *getCommand(int req_num, int sem_level /* = 0*/)
{
	fileCommand_t *retval = NULL;
	if (waitCommandSem(sem_level))
	{
		for (int i=0; i<MAX_ACTIVE_COMMANDS; i++)
		{
			if (commands[i] && commands[i]->req_num == req_num)
			{
				retval = commands[i];
				break;
			}
		}
		command_sem--;
	}
	return retval;
}



bool startCommand(int req_num, char *initial_buffer)
{
	display_level(dbg_queue,1,"startCommand(%d) buf_len(%d)",
		req_num,
		strlen(initial_buffer));

	bool ok = 0;
	if (waitCommandSem(0))
	{
		if (getCommand(req_num,command_sem))
		{
			my_error("startCommand(%d) already active in addCommand()",req_num);
		}
		else
		{
			int cmd_num = -1;
			for (int i=0; i<MAX_ACTIVE_COMMANDS; i++)
			{
				if (!commands[i])
				{
					cmd_num = i;
					break;
				}
			}

			if (cmd_num == -1)
			{
				my_error("startCommand(%d) number commands overflow",req_num);
			}
			else
			{
				// mem_check("before new fileCommand_t");
				fileCommand_t *cmd = (fileCommand_t *) malloc(sizeof(fileCommand_t));
				// mem_check("after new fileCommand_t");

				if (!cmd)
				{
					my_error("unable to allocate new fileCommand_t(%d)",req_num);
				}
				else
				{
					memset(cmd,0,sizeof(fileCommand_t));
					cmd->req_num = req_num;
					cmd->queue[cmd->tail++] = initial_buffer;
					commands[cmd_num] = cmd;
					ok = 1;
				}
			}	// found an empty slot for command
		}	// command doesn't already exist

		command_sem--;
	}

	if (ok)
	{
		threads.addThread(fileCommand,req_num,THREAD_STACK_SIZE);
	}

	return ok;
}


void endCommand(int req_num)
	// only called by the threaded fileCommand()
{
	display_level(dbg_queue,1,"endCommand(%d)",req_num);

	if (waitCommandSem(0))
	{
		for (int i=0; i<MAX_ACTIVE_COMMANDS; i++)
		{
			fileCommand_t *cmd = commands[i];
			if (cmd && cmd->req_num == req_num)
			{
				int head = cmd->head;
				int tail = cmd->head;
				char **queue = commands[i]->queue;
				while (head != tail)
				{
					free(queue[head++]);
					if (head >= MAX_QUEUED_BUFFERS)
						head = 0;
				}
				free(cmd);
				commands[i] = 0;
				command_sem--;
				return;
			}
		}
		my_error("Could not endCommand(%d)",req_num);
		command_sem--;
	}
}


bool addCommandQueue(int req_num, char *buf)
{
	bool retval = false;
	display_level(dbg_queue,1,"addCommandQueue(%d) buf_len(%d)",
		req_num,
		strlen(buf));

	if (waitCommandSem(0))
	{
		fileCommand_t *cmd = getCommand(req_num,command_sem);
		if (!cmd)
		{
			my_error("addCommandQueue() could not find command(%d)",req_num);
		}
		else
		{
			int next_tail = cmd->tail + 1;
			if (next_tail >= MAX_QUEUED_BUFFERS)
				next_tail = 0;

			if (next_tail == cmd->head)
			{
				my_error("commands(%d) overflow",req_num);
			}
			else
			{
				display(dbg_queue+1,"    adding at %d",next_tail);
				cmd->queue[cmd->tail++] = buf;
				if (cmd->tail >= MAX_QUEUED_BUFFERS)
					cmd->tail = 0;
				retval = true;
			}
		}
		command_sem--;
	}
	return retval;
}


char *getCommandQueue(int req_num)
	// returns buffer which must be freed by caller
	// and pointer to the command for passing to _methods
{
	char *retval = NULL;
	if (waitCommandSem(0))
	{
		fileCommand_t *cmd = getCommand(req_num,command_sem);
		if (!cmd)
		{
			if (!missing_command_reported)
				my_error("getCommandQueue() could not find commands(%d)",req_num);
			missing_command_reported = 1;
		}
		else if (cmd->head != cmd->tail)
		{
			retval = cmd->queue[cmd->head];
			cmd->queue[cmd->head] = 0;
			cmd->head++;
			if (cmd->head >= MAX_QUEUED_BUFFERS)
				cmd->head = 0;

			display_level(dbg_queue,2,"getCommandQueue(%d) returning buf_len(%d)",
				req_num,
				strlen(retval));
		}
		command_sem--;
	}
	return retval;
}


//---------------------------------------
// reply methods
//---------------------------------------

void fileReplyError(Stream *fsd, int req_num, const char *format, ...)
{
	char buffer[255];
	char format_buffer[255];
	sprintf(format_buffer,"file_reply(%d):ERROR - %s\r\n",req_num,format);

	va_list var;
	va_start(var, format);
	vsprintf(buffer,format_buffer,var);
	my_error(buffer,0);
	fsd->print(buffer);
}


void fileReply(Stream *fsd, int req_num, const char *format, ...)
{
	char buffer[255];
	char format_buffer[255];
	sprintf(format_buffer,"file_reply(%d):%s\r\n",req_num,format);

	va_list var;
	va_start(var, format);
	vsprintf(buffer,format_buffer,var);
	fsd->print(buffer);
}



//-------------------------------------------------------
// Command and Entry Parsers
//-------------------------------------------------------

int parseCommand(			// returns num_params
	char *buf,				// buffer to parse == the command
	const char **params,	// null terminates command, fills in param pointers (will null terminate BASE64 CONTENT)
	const char **entries)	// optional returns pointer to entries which may be NULL_ptr
{
	char *in = buf;
	int num_params = 0;
	while (*in && *in != '\r')
	{
		display_level(dbg_parse+2,4,"parseCommand() in='%c' 0x%02x",*in>=' '?*in:'.',*in);
		if (*in == '\t')
		{
			*in++ = 0;
			if (num_params < MAX_FILE_PARAMS)
			{
				params[num_params++] = in;
				display_level(dbg_parse+1,3,"parseCommand() num_params=%d",num_params);
			}
		}
		else
		{
			in++;
		}
	}
	if (*in == '\r')
		*in++ = 0;
	if (entries)
		*entries = in;
	for (int i=num_params; i<MAX_FILE_PARAMS; i++)
	{
		params[i] = "";
	}
	return num_params;
}


int getNextEntry(Stream *fsd, int req_num, textEntry_t *the_entry, const char **ptr)
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

	// we set all 3 fields or fail
	// there is a tab after each entry

	int num_params = 0;
	char *out = the_entry->size;

	while (**ptr)
	{
		if (**ptr == '\t' ||
			**ptr == '\r')
		{
			*out = 0;
			num_params++;
			if (num_params == 1)
				out  = the_entry->ts;
			else if (num_params == 2)
				out = the_entry->entry;
			else if (num_params == 3 && *(out-1) == '/')
				// get rid of terminating '/' on dir entries
			{
				the_entry->is_dir = 1;
				*(out-1) = 0;
			}

			bool is_cr = (**ptr == '\r');
			(*ptr)++;
			if (is_cr)
				break;
		}
		else
		{
			*out++ = *(*ptr)++;
		}
	}

	if (num_params != 3)
	{
		fileReplyError(fsd,req_num,"Incorrect number of fields(%d) in fileEntry",num_params);
		return -1;
	}

	return 1;
}


// end of fileutils.cpp
