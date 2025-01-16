//-------------------------------------------------
// fileSerial.cpp
//-------------------------------------------------
// Common handleSerial() method for TE1 and TE2.
// This source code fileis the same in TE1 and TE2.
// It has NOT been made into a submodule yet,
// so must be manually normalized.
//
//

#include <myDebug.h>
#include "fileSystem.h"

// different, but same named includes for TE1 and TE2

#include "defines.h"
	// contains SERIAL_DEVICE macro
#include "prefs.h"
	// contains ACTIVE_FILE_SYS_DEVICE macro



#define dbg_raw_midi 1
	// 0 = show received usb and serial midi
	// may create timing problems
#define dbg_file_command -1
	// 0 = show thread starting
	// -1 = show parse steps
	// -2 = show each character




//--------------------------------------------------------
// Command Parse
//--------------------------------------------------------
// File text start with file_command or file_message,
// followed by the req_num, size, and data
//
// file_command|file_message	\t req_num \t size \t data


#define SERIAL_TIMEOUT  200

#define FILE_COMMAND_SIG		"file_command"
#define FILE_MESSAGE_SIG		"file_message"

#define FILE_COMMAND_SIG_LEN	12
#define MAX_NUM_LENGTH			10

#define MIN_COMMAND_BUF_LEN		3   	// OK
#define MAX_COMMAND_BUF_LEN		15000   // BASE64 size offset ENCODED_CONTENT
	// probably have to tune this last one


typedef struct
{
	int state;
		// 0 = looking for SIG
		// 1 = got SIG, parsing req_num
		// 2 = got req_num parsing size
		// 3 = got size, created buf, adding bytes
	int off;
		// offset within the item being parsed, depending on state

	uint32_t timeout;

	// parsed contents

	int type;
		// 0 == file_command
		// 1 == file_message

	char req_num[MAX_NUM_LENGTH+1];
	char size[MAX_NUM_LENGTH+1];

	int  len;   // length of allocated buffer which is size + 1
	char *buf;

} parseCommand_t;


static parseCommand_t parse_command[2];


static void initParseCommand(parseCommand_t *pcmd)
{
	if (pcmd->buf)
	{
		warning(dbg_malloc,"initParseCommand() freeing buffer(%d) at 0x%08x",pcmd->len + 1,(uint32_t) pcmd->buf);
		free(pcmd->buf);
	}

	memset(pcmd,0,sizeof(parseCommand_t));
}


static void handleChar(bool is_serial, char c)
{
	if (!ACTIVE_FILE_SYS_DEVICE)
	{
		my_error("No ACTIVE_FILE_SYS_DEVICE in handleChar()",0);
		return;
	}

	parseCommand_t *pcmd = &parse_command[is_serial];

	display_level(dbg_file_command+2,1,"state(%d) off(%d) char=%c 0x%02x",
		pcmd->state,
		pcmd->off,
		c>=' '?c:'.',c);

	bool ok = 1;
	bool done = 0;

	if (pcmd->state == 0)									// parsing signature
	{
		char expected = pcmd->type ?
			FILE_MESSAGE_SIG[pcmd->off]:
			FILE_COMMAND_SIG[pcmd->off];

		if (c == '\t' && pcmd->off == FILE_COMMAND_SIG_LEN)	// got the signature
		{
			pcmd->off = 0;		// reset for next state
			pcmd->state++;       // advance to next state
			display_level(dbg_file_command+1,2,"handleChar() got %s",
				pcmd->type ? "file_message" : "file_command");
		}
		else if (c == expected)
		{
			pcmd->off++;
		}
		else if (!pcmd->type && pcmd->off == 5 && c == 'm')
		{
			pcmd->type = 1;
			pcmd->off++;
		}
		else
		{
			ok = 0;
			if (pcmd->off)
				my_error("handleChar() type(%d) off(%d) illegal char in SIG '%c'=0x%02x",
					pcmd->type,
					pcmd->off,
					c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 1)						// parsing req_num
	{
		if (c == '\t')								// got length terminator
		{
			pcmd->req_num[pcmd->off] = 0;
			display_level(dbg_file_command+1,2,"handleChar(%s) got req_num(%s)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num);

			pcmd->off = 0;		// reset for next state
			pcmd->state++;		// next state
		}
		else if (pcmd->off >= MAX_NUM_LENGTH)		// too big
		{
			ok = 0;
			my_error("handleChar(%s) off(%d) req_num overflow",
				pcmd->type?"file_message":"file_command",
				pcmd->off);
		}
		else if (c >= '0' && c <= '9')
		{
			pcmd->req_num[pcmd->off++] = c;
		}
		else
		{
			ok = 0;
			my_error("handleChar(%s) off(%d) illegal char in req_num '%c'=0x%02x",
				pcmd->type?"file_message":"file_command",
				pcmd->off,
				c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 2)						// parsing size
	{
		if (c == '\t')								// got length terminator
		{
			pcmd->size[pcmd->off] = 0;
			display_level(dbg_file_command+1,2,"handleChar(%s,%s) got size(%s)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->size);

			pcmd->len = atoi(pcmd->size);
			if (pcmd->len < MIN_COMMAND_BUF_LEN ||
				pcmd->len > MAX_COMMAND_BUF_LEN)
			{
				ok = 0;
				my_error("handleChar(%s,%s) len(%d) must be between %d and %d",
					pcmd->type?"file_message":"file_command",
					pcmd->req_num,
					pcmd->len,
					MIN_COMMAND_BUF_LEN,
					MAX_COMMAND_BUF_LEN);
			}
			else
			{
				pcmd->buf = (char *) malloc(pcmd->len + 1);		// allocate buffer
				warning(dbg_malloc,"handleChar() malloc'd buffer(%d) at 0x%08x",pcmd->len + 1,(uint32_t) pcmd->buf);
				if (!pcmd->buf)
				{
					ok = 0;
					my_error("handleChar(%s,%s) unable to allocate buffer of len(%d)",
						pcmd->type?"file_message":"file_command",
						pcmd->req_num,
						pcmd->len);
				}
				else
				{
					pcmd->off = 0;		// reset for next state
					pcmd->state++;		// next state
				}
			}
		}
		else if (pcmd->off >= MAX_NUM_LENGTH)		// too big
		{
			ok = 0;
			my_error("handleChar(%s,%s) off(%d) size overflow",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off);
		}
		else if (c >= '0' && c <= '9')
		{
			pcmd->size[pcmd->off++] = c;
		}
		else
		{
			ok = 0;
			my_error("handleChar(%s,%s) off(%d) illegal char in size '%c'=0x%02x",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off,
				c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 3)			// adding characters to the file buffer
	{
		if (pcmd->off > pcmd->len)
		{
			ok = 0;
			my_error("handleChar(%s,%s) buffer overflow at off(%d)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off);
		}
		else if (c == '\n')
		{
			if (pcmd->off == pcmd->len)
			{
				done = 1;
				pcmd->buf[pcmd->off] = 0;		// terminate the buffer
			}
			else
			{
				ok = 0;
				my_error("handleChar(%s,%s) length mismatch off(%d) != len(%d)",
					pcmd->type?"file_message":"file_command",
					pcmd->req_num,
					pcmd->off,
					pcmd->len);
			}
		}
		else		// add character to file_command cuffer
		{
			pcmd->buf[pcmd->off++] = c;
		}
	}

	if (done)	// start the command or send the message
	{
		pcmd->timeout = 0;
		display_level(dbg_file_command,0,"handleChar() %s %s(%s) len=%d",
				pcmd->type?"queing":"starting",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->len);
		int req_num = atoi(pcmd->req_num);

		if (!hasFileSystem())
		{
			warning(dbg_file_command,"handleChar() noFileSytem file_message(%d) dropping buffer of len(%d)",
				req_num,
				pcmd->len);
		}
		else if (pcmd->type)
		{
			if (addCommandQueue(req_num,pcmd->buf))
			{
				pcmd->buf = 0;
				warning(dbg_malloc,"handleChar() giving buffer(%d) to addCommandQueue()",pcmd->len + 1);
			}
			else
			{
				warning(dbg_file_command,"handleChar() could not queue file_message(%d) dropping buffer of len(%d)",
					req_num,
					pcmd->len);
			}
		}
		else if (startCommand(req_num,pcmd->buf))	// takes ownersihp of buffer
		{
			pcmd->buf = 0;
			warning(dbg_malloc,"handleChar() giving buffer(%d) to startCommand()",pcmd->len + 1);
		}
		else
		{
			warning(dbg_file_command,"handleChar() could not start file_command(%d) dropping buffer of len(%d)",
				req_num,
				pcmd->len);
		}

		if (pcmd->buf)
		{
			warning(dbg_malloc,"handleChar() freeinng buffer(%d) at 0x%08x",pcmd->len + 1,(uint32_t) pcmd->buf);
			free(pcmd->buf);
			pcmd->buf = 0;
		}
	}

	// !ok or done - init for new parse

	if (done || !ok)
		initParseCommand(pcmd);
	else if (ok)
		pcmd->timeout = millis();

}	// handleChar



//--------------------------------------------------------
// Serial Port Handler
//--------------------------------------------------------
// Polls Serial and SERIAL_DEVICE for data.
//
// At this time TE2 only allows Serial Midi over the SERIAL
// device or file_commands over either USB or SERIAL.
//
// Midi Packets start with 0x0B which should never be in plain text.
//
// File_commands start with file_command \t length \t reqnum \t data
// When a file_command is received, this routine assumes a full
// packet is following (4 bytes for midi, or <cr-lf> for text)
// and reads the whole packet with  a timeout


void handleSerialData()
{
	// The main USB Serial is only expected to contain lines of text
	// SERIAL_DEVICE may contain either text or serial midi data

	for (int i=0; i<2; i++)
	{
		parseCommand_t *pcmd = &parse_command[i];
		if (pcmd->timeout && millis() - pcmd->timeout > SERIAL_TIMEOUT)
		{
			my_error("%s command timeout",i ? "SERIAL" : "USB");
			initParseCommand(pcmd);
		}
	}

	while (Serial.available())
	{
		int c = Serial.read();
		handleChar(0,c);
	}

	// only serialMidi sends 0x0B!!
	// and they can be sent in the middle of regular lines of text

	while (SERIAL_DEVICE.available())
	{
		int c = SERIAL_DEVICE.read();
		if (c == 0x0B)
		{
			uint8_t midi_buf[4];
			midi_buf[0] = c;

			int i = 1;
			uint32_t midi_timeout = millis();
			while (i<4 && millis() - midi_timeout < SERIAL_TIMEOUT)
			{
				if (SERIAL_DEVICE.available())
				{
					midi_buf[i++] = SERIAL_DEVICE.read();
					midi_timeout = millis();
				}
			}

			if (i < 4)
				my_error("serial midi timeout(%d)",i);
			else
			{
				if (dbg_raw_midi <= 0)
					display_level(dbg_raw_midi,0,"serial: 0x%02x%02x%02x%02x",
						midi_buf[3],
						midi_buf[2],
						midi_buf[1],
						midi_buf[0]);
				handleCommonMidiSerial(midi_buf);
				// TD1: theSystem.getCurRig()->onSerialMidiEvent(midi_buf[2],midi_buf[3]);
				// TE2: enqueueMidi(false, MIDI_PORT_SERIAL,midi_buf);
			}
		}
		else
		{
			handleChar(1,c);
		}
	}	// SERIAL_DEVICE.available();
}	// handleSerialData()


