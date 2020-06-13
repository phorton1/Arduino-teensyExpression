#include <myDebug.h>
#include "midiHostConfig.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"

#define SHOW_MIDI_EVENTS   1


#if WITH_MIDI_HOST
	#define MIDI_PASS_THRU  1
	#include "myMidiHost.h"
#endif


#define BUTTON_ONE   20
#define BUTTON_TWO   21


bool showOnTFT = 0;
void showBufferedDisplay();
void printBuffered(const char *format, ...);



//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

midiHostConfig::midiHostConfig() 
{
    draw_needed = 0;
    redraw_needed = 0;
}


// virtual
void midiHostConfig::begin()
{
	expConfig::begin();	
    draw_needed = 1;

    theButtons.setButtonEventMask(BUTTON_ONE, BUTTON_EVENT_CLICK);
    theButtons.setButtonEventMask(BUTTON_TWO, BUTTON_EVENT_CLICK);
	setLED(BUTTON_ONE,LED_BLUE);
	setLED(BUTTON_TWO,LED_BLUE);
	
	showLEDs();
}




//------------------------------------------------------------
// events
//------------------------------------------------------------

uint32_t send_it = 0;



// virtual
void midiHostConfig::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	if (num == BUTTON_ONE)		// sysex test
	{
		// SYSEX DEVICE ID REQUEST
		// Send:  F0 7E 7F 06 01 F7
		// Expected:           F0 7E 10 06 02 00 01 6E 00 01 00 01 02 21 01 00 F7
		// Got (same for both dongles) from HOST :
		//		SysEx Message: F0 7E 00 06 02 00 01 6E 00 01 00 02 01 55 01 00 F7  (end)
		//		SysEx Message: F0 7E 10
		//                     F0 7E 10 06 02 00 06 02 00 01 6E 00 01 6E 00 01 00 01 01 00 01 02 21 01 02 21 01 00 F7  (end)
		//		SysEx Message: 00 F7  (end)
		// Got from iPad:      F0 7E 10 06 02 00 01 6E 00 01 00 01 02 21 01 00 F7
		//      == expected


		#define REQUEST_LEN  6
		uint8_t data[REQUEST_LEN] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};

		showOnTFT = 1;
			// echo the results
		
		#if WITH_MIDI_HOST
			if (midi_host_on)
			{
				display(0,"sending request(%d) to MIDI_HOST",REQUEST_LEN);
				midi1.sendSysEx(REQUEST_LEN,data,true);
			}
			else
		#endif
		{
			display(0,"sending request(%d) as midi device",REQUEST_LEN);
			usbMIDI.sendSysEx(REQUEST_LEN,data,true); 	
		}

		setLED(num,LED_BLUE);
		showLEDs();
	}
	
	else if (num == BUTTON_TWO)		// trying to figure out senstitivy
	{		
		// sensitivity messages, all I had from my previous notes
		// was that sensitivy settings involved "b7 1f 07" where
		// the first nibble is string 0..5, and the second nibble
		// is sensitivity 0..f, but I could not get it to work!

		send_it = 255;
			// a scheme to send out 255 messages

		setLED(num,LED_BLUE);
		showLEDs();
	}
}



//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------
// also used to send repetitive midi events every so often

elapsedMillis send_timer = 0;

// virtual
void midiHostConfig::updateUI()	// draw
{
	if (send_it && send_timer > 200)
	{
		send_timer = 0;

		// I found that i get a response if I write
		// b7 1f 07  followed by b7 3f 03, so this
		// loop tries each with 0..15
		
		uint32_t msg1 = 0x001Fb70b | ((send_it&0xf0)<<20);
		uint32_t msg2 = 0x003Fb70b | ((send_it&0x0f)<<24);
		
		display(0,"Writing msg1 0x1f %02x %02x",(msg1 >> 24),(msg1 >> 16) & 0xff,(msg1 >> 8) & 0xff);
		midi1.write_packed(msg1);
		
		delay(100);
		display(0,"Writing msg2 0x1f %02x %02x",(msg2 >> 24),(msg2 >> 16) & 0xff,(msg2 >> 8) & 0xff);
		midi1.write_packed(msg2);

		send_it--;
	}

	if (draw_needed)
	{
		#if 0
			mylcd.Fill_Screen(0);
			mylcd.setFont(Arial_16_Bold);
			mylcd.Set_Text_Cursor(10,10);
			mylcd.Set_Text_colour(TFT_YELLOW);
			mylcd.print(getCurConfig()->name());
			mylcd.Set_Draw_color(TFT_YELLOW);
			mylcd.Draw_Line(0,36,TFT_WIDTH,36);
		#endif

		draw_needed = false;
		mylcd.setFont(Arial_20);
		mylcd.Set_Text_Cursor(20,50);
		mylcd.Set_Text_colour(TFT_WHITE);
		
		#if WITH_MIDI_HOST
			mylcd.print("MIDI_HOST ");
			mylcd.println(midi_host_on ? "ON" : " is OFF!! it must be on!!");
		#else
			mylcd.println("WITH_MIDI_HOST is not defined!!");
		#endif
	}
	
	#if !FAST_ECHO_MIDI
		myusb.Task();
		midi1.read();
		usbMIDI.read();
	#endif
	
	showBufferedDisplay();
}


//---------------------------------------------------
// Events
//---------------------------------------------------

// virtual
void midiHostConfig::onMidiEvent(uint32_t msg)
{
	#if SHOW_MIDI_EVENTS
		printBuffered("device <-- %08X",msg);
	#endif
	
	#if WITH_MIDI_HOST
		#if MIDI_PASS_THRU
			midi1.write_packed(msg);
		#endif
	#endif
}



#if WITH_MIDI_HOST
	// virtual
	void midiHostConfig::onMidiHostEvent(uint32_t msg)
	{
		#if SHOW_MIDI_EVENTS
			printBuffered("host  <-- %08X",msg);
		#endif
		
		#if MIDI_PASS_THRU
			usb_midi_write_packed(msg);		// write as device
			usb_midi_flush_output();
		#endif		
	}
#endif




//---------------------------------------------------
// buffered display
//---------------------------------------------------
// CONSOLE.pm serial port buffer overflows if we just send all
// this text from the interrutps, so we buffer it and send it
// out one line at a time, in updateUI() based on a ms timer.

#define MAX_STRING 256
#define MAX_DISPLAY_BUFFER 32768

char temp_buffer[MAX_STRING];
char display_buffer[MAX_DISPLAY_BUFFER];
volatile int buf_write = 0;
volatile int buf_read = 0;


elapsedMillis buffer_timer = 0;


void showBufferedDisplay()
{
	if (buf_read != buf_write && buffer_timer > 8)
	{
		static char work_buffer[MAX_STRING];
		
		int len = 0;
		char *op = work_buffer;
		char *ip = &display_buffer[buf_read];
		
		while (*ip && len<MAX_STRING)
		{
			len ++;
			buf_read++;
			*op++ = *ip ++;
			
			if (buf_read == MAX_DISPLAY_BUFFER)
			{
				buf_read = 0;
				ip = display_buffer;
			}
			if (buf_read == buf_write)
			{
				my_error("read buffer overflow at %d,%d?!?!",buf_read,buf_write);
				buf_read = buf_write = 0;
				return;
			}
		}
		
		*op++ = 0;
		buf_read++;
		if (buf_read == MAX_DISPLAY_BUFFER)
			buf_read = 0;


		// Serial.print(ptr->host ? "\033[95m" : "\033[94m");
		Serial.println(work_buffer);
		
		if (showOnTFT)
		{
			mylcd.setFont(Arial_12);
			mylcd.println(work_buffer);			
		}
		buffer_timer = 0;
	}
}


void addBufferedString(char *s)
{
	// int len = strlen(s);
	
	char *op = &display_buffer[buf_write];
	while (*s)
	{
		*op++ = *s++;
		buf_write++;
		if (buf_write == MAX_DISPLAY_BUFFER)
		{
			buf_write = 0;
			display(0,"WRAP",0);
			op = display_buffer;
		}
		if (buf_write == buf_read)
		{
			my_error("write buffer overflow",0);
			buf_read = buf_write = 0;
			return;
		}
	}
	*op++ = 0;
	buf_write++;
	if (buf_write == MAX_DISPLAY_BUFFER)
		buf_write = 0;
	if (buf_write == buf_read)
	{
		my_error("write buffer overflow2",0);
		buf_read = buf_write = 0;
		return;
	}
}


void printBuffered(const char *format, ...)
{
	va_list var;
	va_start(var, format);
	if (strlen(format) >= MAX_STRING/2)
	{
		my_error("error - MAX_STRING overflow",0);
		return;
	}
	vsprintf(temp_buffer,format,var);
	addBufferedString(temp_buffer);
}


void printBytes(const char *before, const byte *data, unsigned int size, const char *after)
{
	int full_len = 1 + size * 3 +
		(before ? strlen(before) : 0) +
		(after ? strlen(after) : 0);
		
	if (full_len >= MAX_STRING)
	{
		my_error("error - printBytes buffer overflow",0);
		return;
	}
	
	char *p = temp_buffer;
	if (before)
	{
		strcpy(p,before);
		p += strlen(before);
	}
	
	while (size > 0)
	{
		size--;
		byte b = *data++;
		sprintf(p,"%02X ",b);
		p += 3;
	}

	if (after)
	{
		strcpy(p,after);
	}
	addBufferedString(temp_buffer);
}
	


