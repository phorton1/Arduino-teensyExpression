//----------------------------------------------------------
// fileUtils.cpp
//----------------------------------------------------------
// contains command, queue, reply, and parse methods
// used by fileCommand.cpp

#include <stdlib.h>

#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>
#include <TeensyThreads.h>


#define dbg_start  -1
	//  0 = show start commands and freed commands
	// -1 = show initial parsing details
#define dbg_queue  -1
	//  0 = show main queue actions
	// -1 = show detailed queue actions
	// -2 = show null getCommandQueue results (used in command wait loops)

#define QUEUE_TIMEOUT    5		// ms
	// timeout for semaphore to get access to queue
#define THREAD_STACK_SIZE   8192
	// stack for doCommand()
	// The most intense stack is for recursive PUT, which
	// uses about 1K per directory level, and we always need
	// at least 1K additional for any fileReply, and some slop.
	// Thus with 8192, we set MAX_RECURSION_DEPTH to 6.
#define MAX_ACTIVE_COMMANDS 2
	// Number of simultaneous "sessions" that may exist at a time






static fileCommand_t commands[MAX_ACTIVE_COMMANDS];
	// pointer to a command remains valid through it's lifetime


//----------------------------------
// init and free methods
//----------------------------------

void initFileCommands()
{
	for (int cmd_num=0; cmd_num<MAX_ACTIVE_COMMANDS; cmd_num++)
	{
		memset(&commands[cmd_num],0,sizeof(fileCommand_t));
		commands[cmd_num].cmd_num = cmd_num;
	}
}

void freeFileCommands()
{
	for (int cmd_num=0; cmd_num<MAX_ACTIVE_COMMANDS; cmd_num++)
	{
		fileCommand_t *cmd = &commands[cmd_num];
		if (cmd->done)
		{
			display_level(dbg_start,0,"freeFileCommand cmd_num(%d) req_num(%d)",cmd_num,cmd->req_num);

			if (cmd->dir_buffer)
			{
				warning(dbg_malloc,"free dir_buffer at 0x%08x",(uint32_t) cmd->dir_buffer);
				free(cmd->dir_buffer);
			}
			if (cmd->decoded_buffer)
			{
				warning(dbg_malloc,"free decoded_buffer at 0x%08x",(uint32_t) cmd->decoded_buffer);
				free(cmd->decoded_buffer);
			}
			if (cmd->encoded_buffer)
			{
				warning(dbg_malloc,"free decoded_buffer at 0x%08x",(uint32_t) cmd->encoded_buffer);
				free(cmd->encoded_buffer);
			}

			for (int i=0; i<MAX_QUEUED_BUFFERS; i++)
			{
				if (cmd->queue[i])
				{
					warning(dbg_malloc,"free queue[%d] at 0x%08x",i,(uint32_t) cmd->queue[i]);
					free(cmd->queue[i]);
				}
			}

			memset(cmd,0,sizeof(fileCommand_t));
			cmd->cmd_num = cmd_num;
			MEM_INFO("after freeing cmd");
		}
	}
}



//---------------------------------------
// reply methods
//---------------------------------------

void fileReplyError(Stream *fsd, fileCommand_t *cmd, const char *format, ...)
{
	char buffer[255];
	char format_buffer[255];
	sprintf(format_buffer,"file_reply(%d):ERROR - %s\r\n",cmd->req_num,format);

	va_list var;
	va_start(var, format);
	vsprintf(buffer,format_buffer,var);
	my_error(buffer,0);
	fsd->print(buffer);
}


void fileReply(Stream *fsd, fileCommand_t *cmd, const char *format, ...)
{
	char buffer[255];
	char format_buffer[255];
	sprintf(format_buffer,"file_reply(%d):%s\r\n",cmd->req_num,format);

	va_list var;
	va_start(var, format);
	vsprintf(buffer,format_buffer,var);
	fsd->print(buffer);
}



//----------------------------
// semaphore
//----------------------------

static volatile int command_sem;

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



//-------------------------------------------
// parseCommand() & startCommand()
//-------------------------------------------

bool parseCommand(
	fileCommand_t *cmd,
	int req_num,
	char *buf)
{
	display_level(dbg_start+1,2,"parseICommand req_num(%d)",req_num);

	cmd->req_num = req_num;
	cmd->command = buf;

	int num_params = 0;
	while (*buf)
	{
		char c = *buf;
		if (c == '\t' || c == '\r')
		{
			*buf++ = 0;
			if (!num_params)
			{
				display_level(dbg_start+1,2,"command=%s",cmd->command);
			}
			else if (num_params <= 3)
			{
				if (num_params==3 && !strcmp(cmd->command,"BASE64"))
				{
					display_level(dbg_start+1,2,"param[%d]=base64 len=%d",num_params-1,strlen(cmd->params[num_params-1]));
				}
				else
				{
					display_level(dbg_start+1,2,"param[%d]=%s",num_params-1,cmd->params[num_params-1]);
				}
			}
			if (num_params < 3)
				cmd->params[num_params] = buf;
			if (c == '\r')
				break;
			num_params++;
		}
		else
		{
			buf++;
		}
	}

	display_level(dbg_start+1,2,"num_params=%d",num_params);
	if (num_params>3)
	{
		my_error("illegal number of params: %d",num_params);
		return 0;
	}

	// null out any dangling parameters
	
	for (int i=num_params; i<3; i++)
	{
		cmd->params[i] = "";
	}

	cmd->num_params = num_params;
	cmd->entries = buf;
	return 1;
}


static bool allocDirBuffer(fileCommand_t *cmd)
{
	cmd->dir_buffer = (char *)malloc(MAX_DIRECTORY_BUF);
	if (!cmd->dir_buffer)
		my_error("Could not allocate dir_buffer",0);
	else
		warning(dbg_malloc,"malloc dir_buffer(%d) at 0x%08x",MAX_DIRECTORY_BUF,(uint32_t) cmd->dir_buffer);
	return cmd->dir_buffer;
}
static bool allocDecodedBuffer(fileCommand_t *cmd)
{
	cmd->decoded_buffer = (char *) malloc(MAX_DECODED_BUF);
	if (!cmd->decoded_buffer)
		my_error("Could not allocate decodrd_buf",0);
	else
		warning(dbg_malloc,"malloc decodrd_buf(%d) at 0x%08x",MAX_DECODED_BUF,(uint32_t) cmd->decoded_buffer);
	return cmd->decoded_buffer;
}
static bool allocEncodedBuffer(fileCommand_t *cmd)
{
	cmd->encoded_buffer = (char *) malloc(MAX_ENCODED_BUF);
	if (!cmd->encoded_buffer)
		my_error("Could not allocate encodrd_buf",0);
	else
		warning(dbg_malloc,"malloc encodrd_buf(%d) at 0x%08x",MAX_ENCODED_BUF,(uint32_t) cmd->encoded_buffer);
	return cmd->encoded_buffer;
}



bool startCommand(int req_num, char *initial_buffer)
	// It is useful to parse the command BEFORE we start
	// the thread, because we can then know, ahead of time,
	// what buffers are needed.
{
	display_level(dbg_start,1,"startCommand req_num(%d) buf_len(%d)",
		req_num,
		strlen(initial_buffer));
	MEM_INFO("startCommand");

	bool ok = 0;
	fileCommand_t *cmd = 0;
	if (waitCommandSem(0))
	{
		int cmd_num = -1;
		for (int i=0; i<MAX_ACTIVE_COMMANDS; i++)
		{
			if (!commands[i].req_num)
			{
				cmd_num = i;
				break;
			}
		}

		if (cmd_num == -1)
		{
			ok = 0;
			my_error("startCommand(%d) number commands overflow",req_num);
		}
		else
		{
			cmd = &commands[cmd_num];
			command_sem--;
			display_level(dbg_start+1,2,"using cmd_num(%d)",cmd_num);
			ok = parseCommand(cmd,req_num,initial_buffer);
			if (ok)
			{
				// those commands that get only a dir_buffer

				if (!strcmp(cmd->command,"LIST") ||
					!strcmp(cmd->command,"DELETE") ||
					!strcmp(cmd->command,"MKDIR"))
				{
					ok = allocDirBuffer(cmd);
				}

				// those commands that get only a decoded_buffer

				else if (!strcmp(cmd->command,"FILE"))
				{
					ok = allocDecodedBuffer(cmd);
				}

				// those commands that get all three buffers

				else if (!strcmp(cmd->command,"PUT"))
				{
					ok = ok && allocDirBuffer(cmd);
					ok = ok && allocDecodedBuffer(cmd);
					ok = ok && allocEncodedBuffer(cmd);
				}
			}
		}
	}

	if (ok)
	{
		MEM_INFO("starting thread");
		threads.addThread(fileCommand,(void *)cmd,THREAD_STACK_SIZE);
	}
	else if (cmd)
	{
		display_level(dbg_start,2,"invalidating cmd_num(%d)",cmd->cmd_num);
		cmd->done = 1;
	}

	return ok;
}



//----------------------------------------------------
// addCommandQueue() and getCommandQueue()
//----------------------------------------------------

static fileCommand_t *getCommand(int req_num, int sem_level /* = 0*/)
{
	fileCommand_t *retval = NULL;
	if (waitCommandSem(sem_level))
	{
		for (int cmd_num=0; cmd_num<MAX_ACTIVE_COMMANDS; cmd_num++)
		{
			if (commands[cmd_num].req_num == req_num)
			{
				display_level(dbg_queue+1,3,"getCommand req_num(%d) found at cmd_num(%d)",
					req_num,
					cmd_num);
				retval = &commands[cmd_num];
				break;
			}
		}
		command_sem--;
	}
	return retval;
}


bool addCommandQueue(int req_num, char *buf)
{
	bool retval = false;
	display_level(dbg_queue,1,"addCommandQueue req_num(%d) buf_len(%d)",
		req_num,
		strlen(buf));

	if (waitCommandSem(0))
	{
		fileCommand_t *cmd = getCommand(req_num,command_sem);
		if (!cmd)
		{
			my_error("addCommandQueue() could not find req_num(%d)",req_num);
		}
		else
		{
			display_level(dbg_queue+1,2,"addCommandQueue req_num(%d) found cmd_num(%d)",
				req_num,
				cmd->cmd_num);

			int next_tail = cmd->tail + 1;
			if (next_tail >= MAX_QUEUED_BUFFERS)
				next_tail = 0;

			if (next_tail == cmd->head)
			{
				my_error("commands req_num(%d) cmd_num(%d) overflow",req_num,cmd->cmd_num);
			}
			else
			{
				display(dbg_queue+1,"    adding buf at queue[%d]",cmd->tail);
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


char *getCommandQueue(fileCommand_t *cmd)
	// returns buffer which must be freed by caller
	// and pointer to the command for passing to _methods.
	// used in wait loops.
{
	display_level(dbg_queue+2,2,"getCommandQueue req_num(%d) head=%d tail=%d",cmd->req_num,cmd->head,cmd->tail);
	char *retval = NULL;
	if (waitCommandSem(0))
	{
		if (cmd->head != cmd->tail)
		{
			int at = cmd->head;
			retval = cmd->queue[at];
			cmd->queue[at] = 0;
			cmd->head++;
			if (cmd->head >= MAX_QUEUED_BUFFERS)
				cmd->head = 0;

			display_level(dbg_queue+2,3,"getCommandQueue req_num(%d) cmd_num(%d) queue[%d] buf_len(%d) ptr=0x%08x",
				cmd->req_num,
				cmd->cmd_num,
				at,
				strlen(retval),
				(uint32_t) retval);
		}
		command_sem--;
	}

	// show actual return values (for wait loops) at
	// dbg_queue == -1, but only show nulls at dbg_queue==-2
	
	int use_dbg = retval ? dbg_queue+1 : dbg_queue+2;
	display_level(use_dbg,2,"getCommandQueue returning 0x%08x",(uint32_t) retval);
	return retval;
}


// end of fileutils.cpp
