#include <myDebug.h>
#include "midiHostConfig.h"
#include "defines.h"
#include "myLeds.h"

#if WITH_MIDI_HOST
    #include <USBHost_t36.h>
	extern USBHost myusb;
	extern myMidiHostDevice midi1;
	// extern MIDIDevice midi1;
		// in teensyExpression.ino
#endif




#define BUTTON_ONE   20
#define BUTTON_TWO   21

#define WIDTH           480
#define HEIGHT          320

void registerAllHandlers(bool on);
void showBufferedDisplay();
bool showOnTFT = 0;

void printBuffered(bool host, const char *format, ...);


#if FAST_ECHO_MIDI

	uint32_t myMidiHostDevice::myRead(uint8_t channel)
	{
		uint32_t n, head, tail, avail;
		// uint32_t ch,bl,type1,type2;
	
		head = rx_head;
		tail = rx_tail;
		if (head == tail) return false;
		if (++tail >= RX_QUEUE_SIZE) tail = 0;
		n = rx_queue[tail];
		rx_tail = tail;
		if (!rx_packet_queued && rxpipe)
		{
			avail = (head < tail) ? tail - head - 1 : RX_QUEUE_SIZE - 1 - head + tail;
			if (avail >= (uint32_t)(rx_size>>2))
			{
				__disable_irq();
				queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
				__enable_irq();
			}
		}
		// println("read: ", n, HEX);
		return n;
	}
#endif


//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

midiHostConfig::midiHostConfig(expSystem *pSystem) :
	expConfig(pSystem)
{
    draw_needed = 0;
    redraw_needed = 0;
}


// virtual
void midiHostConfig::begin()
{
	expConfig::begin();	
    draw_needed = 1;

    rawButtonArray *ba = m_pSystem->getRawButtonArray();
    ba->setButtonEventMask(BUTTON_ONE, BUTTON_EVENT_CLICK);
    ba->setButtonEventMask(BUTTON_TWO, BUTTON_EVENT_CLICK);
	setLED(BUTTON_ONE,LED_BLUE);
	setLED(BUTTON_TWO,LED_BLUE);
	
	showLEDs();
	
	#if !FAST_ECHO_MIDI
		registerAllHandlers(1);
	#endif
}


// virtual
void midiHostConfig::end()
{
	#if !FAST_ECHO_MIDI
		registerAllHandlers(0);
	#endif
}



//------------------------------------------------------------
// events
//------------------------------------------------------------

uint32_t send_it = 0;



// virtual
void midiHostConfig::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;
	if (num == BUTTON_ONE ||
		num == BUTTON_TWO)
	{
		// showOnTFT = 1;
		
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

		#if 1
			#define REQUEST_LEN  6
			uint8_t data[REQUEST_LEN] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
	
			display(0,"sending request(%d)",REQUEST_LEN);
					
			// if (num == BUTTON_ONE)
			//	usbMIDI.sendSysEx(REQUEST_LEN,data,true); 	// true=hasTerm  default cable=0

			if (num == BUTTON_ONE)
				midi1.sendSysEx(REQUEST_LEN,data,true); 	// true=hasTerm  default cable=0
		#endif

		
		// sensitivity messages, all I know is  "b7 1f 07"
		//		first nibble is string 0..5
		//		second nibble is sensitivity 0..f
		//	
		//	that works out to controlMessage(B7) = channel 8
		//      ccNum(0x1f) = 31

		if (num == BUTTON_TWO)
		{
			send_it = 255;
			
			#if 0
				usb_midi_write_packed(msg1);		// write as device
				usb_midi_flush_output();
				delay(100);
				usb_midi_write_packed(msg2);		// write as device
				usb_midi_flush_output();
			#endif
			
			//midi1.sendControlChange(0x1f,0x07,8);		// set string 3 to 10
			//midi1.sendControlChange(0x35,0x03,8);		// set string 3 to 10
			
		}

		setLED(num,LED_BLUE);
		showLEDs();
	}
}


//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------

// virtual

elapsedMillis send_timer = 0;

void midiHostConfig::updateUI()	// draw
{
			
	if (send_it && send_timer > 200)
	{
		send_timer = 0;

		// uint32_t msg1 = 0x071Fb70b;
		// uint32_t msg2 = 0x033Fb70b;
		
		uint32_t msg1 = 0x001Fb70b; // | 0x07000000;
		uint32_t msg2 = 0x003Fb70b; // | 0z03000000;
		
		display(0,"BUTTON_TWO ... 1f %02x and 3f %02x",msg1  | ((send_it&0x0f)<<24),msg2 | ((send_it&0xf0)<<16));
		midi1.write_packed(msg1 | ((send_it&0x0f)<<24));
		delay(100);
		midi1.write_packed(msg2 | ((send_it&0xf0)<<16));
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
			mylcd.Draw_Line(0,36,WIDTH,36);
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



// virtual
void midiHostConfig::timer_handler()
{
	// read from host, write to device,
	
	#if FAST_ECHO_MIDI
		#if WITH_MIDI_HOST
			myusb.Task();
			#if 0
				midi1.read();
			#else
				uint32_t msg = midi1.myRead();		// read from host
				if (msg)
				{
					if (msg != 0x0000fe1f)
						printBuffered(1,"<-- msg=%08x",msg);
					usb_midi_write_packed(msg);		// write as device
					usb_midi_flush_output();
				}
			#endif
		#endif
		
		// read from device, write to host,
	
		#if 1
			// usbMIDI.read();
			uint32_t msg2 = usb_midi_read_message();
			if (msg2)
			{
				if (msg != 0x0000fe1f)
					printBuffered(0,"--> msg=%08x",msg2);
				midi1.write_packed(msg2);
			}
		#endif
	#endif	
}





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


void addBufferedString(bool host,char *s)
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


void printBuffered(bool host, const char *format, ...)
{
	va_list var;
	va_start(var, format);
	if (strlen(format) >= MAX_STRING/2)
	{
		my_error("error - MAX_STRING overflow",0);
		return;
	}
	vsprintf(temp_buffer,format,var);
	addBufferedString(host,temp_buffer);
}


void printBytes(bool host, const char *before, const byte *data, unsigned int size, const char *after)
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
	addBufferedString(host,temp_buffer);
}
	

void showTimeCodeQuarterFrame(bool host, byte data)
{
	static char SMPTE[8]={'0','0','0','0','0','0','0','0'};
	static byte fps=0;
	
	byte index = data >> 4;
	byte number = data & 15;
	if (index == 7)
	{
		fps = (number >> 1) & 3;
		number = number & 1;
	}

	if (index < 8 || number < 10)
	{
		SMPTE[index] = number + '0';
		
		printBuffered(host,"TimeCode: %d%d:%d%d:%d%d.%d%d %s",
			SMPTE[7],
			SMPTE[6],
			SMPTE[5],
			SMPTE[4],
			SMPTE[3],
			SMPTE[2],
			SMPTE[1],  // perhaps add 2 to compensate for MIDI latency?
			SMPTE[0],
			fps == 0 ? " 24 fps" :
			fps == 1 ? " 25 fps" :
			fps == 2 ? " 29.97 fps" :
			fps == 3 ? " 30 fps" :
			"unknown fps");
	}
	else
	{
		printBuffered(host,"TimeCode: invalid data = %04x",data);
	}
}	



//-----------------------------------------------
// device midi event handlers
//-----------------------------------------------



void deviceNoteOn(byte channel, byte note, byte velocity)
	{ printBuffered(0,"Note On, ch=%d note=%d velocity=%d",channel,note,velocity); }
void deviceNoteOff(byte channel, byte note, byte velocity)
	{ printBuffered(0,"Note Off, ch=%d note=%d velocity=%d",channel,note,velocity); }
void deviceAfterTouchPoly(byte channel, byte note, byte velocity)
	{ printBuffered(0,"AfterTochPoly, ch=%d note=%d velocity=%d",channel,note,velocity); }
void deviceControlChange(byte channel, byte control, byte value)
{
	printBuffered(0,"Control Change, ch=%d control=%d value=%d",channel,control,value);
	#if PASS_THRU
		midi1.sendControlChange(channel, control, value);
	#endif	
}
void deviceProgramChange(byte channel, byte program)
	{ printBuffered(0,"Program Change, ch=%d program=%d",channel,program); }
void deviceAfterTouchChannel(byte channel, byte pressure)
	{ printBuffered(0,"myAfterTouchChannel, ch=%d pressure=%d",channel,pressure); }
void devicePitchChange(byte channel, int pitch)
	{ printBuffered(0,"Pitch Change, ch=%d, pitch=%d",channel,pitch); }

void deviceSystemExclusiveChunk(const byte *data, uint16_t length, bool last)
{
	printBytes(0,"SysEx Message: ", data, length, last ? " (end)" : " (to be continued)");
	#if PASS_THRU
		midi1.sendSysEx(length,data,last); 	// true=hasTerm  default cable=0
	#endif
}
void deviceSystemExclusive(byte *data, unsigned int length)
{
	printBytes(0,"SysEx Message: ", data, length, 0);
	#if PASS_THRU
		midi1.sendSysEx(length,data,true); 	// true=hasTerm  default cable=0
	#endif
}

void deviceSongPosition(uint16_t beats)
	{ printBuffered(0,"Song Position, beat=%d",beats); }
void deviceSongSelect(byte songNumber)
	{ printBuffered(0,"Song Select, song=%d",songNumber); }
void deviceRealTimeSystem(uint8_t realtimebyte) 
	{ printBuffered(0,"Real Time Message, code=0x%02X",realtimebyte); }
	
void deviceTuneRequest()		{ printBuffered(0,"Tune Request"); }
void deviceClock()    			{ printBuffered(0,"Clock"); }
void deviceStart()				{ printBuffered(0,"Start"); }
void deviceContinue()			{ printBuffered(0,"Continue"); }
void deviceStop()   			{ printBuffered(0,"Stop"); }
void deviceActiveSensing()
{
	/* printBuffered(0, "Actvice Sensing"); */
	#if PASS_THRU
		usbMIDI.sendRealTime(0xFE);
	#endif
	
}
void deviceSystemReset()    	{ printBuffered(0,"System Reset"); }

void deviceTimeCodeQuarterFrame(byte data)
	{ showTimeCodeQuarterFrame(0, data); }



//-----------------------------------------------
// host midi event handlers
//-----------------------------------------------

#if WITH_MIDI_HOST

	// When a USB device with multiple virtual cables is used,
	// midi1.getCable() can be used to read which of the virtual
	// MIDI cables received this message.

	// This 3-input System Exclusive function is more complex, but allows you to
	// process very large messages which do not fully fit within the midi1's
	// internal buffer.  Large messages are given to you in chunks, with the
	// 3rd parameter to tell you which is the last chunk.  This function is
	// a Teensy extension, not available in the Arduino MIDI library.

	// This simpler 2-input System Exclusive function can only receive messages
	// up to the size of the internal buffer.  Larger messages are truncated, with
	// no way to receive the data which did not fit in the buffer.  If both types
	// of SysEx functions are set, the 3-input version will be called by midi1.
	
	void hostNoteOn(byte channel, byte note, byte velocity)
		{ printBuffered(1,"Note On, ch=%d note=%d velocity=%d",channel,note,velocity); }
	void hostNoteOff(byte channel, byte note, byte velocity)
		{ printBuffered(1,"Note Off, ch=%d note=%d velocity=%d",channel,note,velocity); }
	void hostAfterTouchPoly(byte channel, byte note, byte velocity)
		{ printBuffered(1,"AfterTochPoly, ch=%d note=%d velocity=%d",channel,note,velocity); }
	void hostControlChange(byte channel, byte control, byte value)
	{
		printBuffered(1,"Control Change, ch=%d control=%d value=%d",channel,control,value);
		#if PASS_THRU
			
			byte type = 0xB0;
			byte cable = 0;
			
			usb_midi_write_packed((type << 8) | (type >> 4) | ((cable & 0x0F) << 4)
			  | (((channel - 1) & 0x0F) << 8) | ((control & 0x7F) << 16)
			  | ((value & 0x7F) << 24));

			usb_midi_flush_output();
			
			// usbMIDI.sendControlChange(channel, control, value);
		#endif	
	}
	void hostProgramChange(byte channel, byte program)
		{ printBuffered(1,"Program Change, ch=%d program=%d",channel,program); }
	void hostAfterTouchChannel(byte channel, byte pressure)
		{ printBuffered(1,"myAfterTouchChannel, ch=%d pressure=%d",channel,pressure); }
	void hostPitchChange(byte channel, int pitch)
		{
			display(0,"pitch changed %d,%d",channel,pitch);
			printBuffered(1,"Pitch Change, ch=%d, pitch=%d",channel,pitch);
		}

	void hostSystemExclusiveChunk(const byte *data, uint16_t length, bool last)
	{
		printBytes(1,"SysEx Message: ", data, length, last ? " (end)" : " (to be continued)");
		#if PASS_THRU
			if (length > 3 &&
				data[3] == 0xF0 )
				usbMIDI.sendSysEx(length-3,&data[3],last); 	// true=hasTerm  default cable=0
			else
				usbMIDI.sendSysEx(length,data,last); 	// true=hasTerm  default cable=0
		#endif
	}
	void hostSystemExclusive(byte *data, unsigned int length)
	{
		printBytes(1,"SysEx Message: ", data, length, 0);
		#if PASS_THRU
			usbMIDI.sendSysEx(length,data,true); 	// true=hasTerm  default cable=0
		#endif
	}
	
	void hostSongPosition(uint16_t beats)
		{ printBuffered(1,"Song Position, beat=%d",beats); }
	void hostSongSelect(byte songNumber)
		{ printBuffered(1,"Song Select, song=%d",songNumber); }
	void hostRealTimeSystem(uint8_t realtimebyte) 
		{ printBuffered(1,"Real Time Message, code=0x%02X",realtimebyte); }
		
	void hostTuneRequest()		{ printBuffered(1,"Tune Request"); }
	void hostClock()    		{ printBuffered(1,"Clock"); }
	void hostStart()			{ printBuffered(1,"Start"); }
	void hostContinue()			{ printBuffered(1,"Continue"); }
	void hostStop()   			{ printBuffered(1,"Stop"); }
	void hostActiveSensing()
	{
		/* printBuffered(1, "Actvice Sensing"); */
		#if PASS_THRU
			usbMIDI.sendRealTime(0xFE);
		#endif
	}
	void hostSystemReset()    	{ printBuffered(1,"System Reset"); }
	
	void hostTimeCodeQuarterFrame(byte data)
		{ showTimeCodeQuarterFrame(1, data); }
	
#endif	// WITH_MIDI_HOST
	



//-----------------------------------------------
// register midi event handlers
//-----------------------------------------------
	

void registerAllHandlers(bool on)
{
	usbMIDI.setHandleNoteOn(				on ? deviceNoteOn : 0);
	usbMIDI.setHandleNoteOff(				on ? deviceNoteOff : 0);
	usbMIDI.setHandleAfterTouchPoly(		on ? deviceAfterTouchPoly : 0);
	usbMIDI.setHandleControlChange(			on ? deviceControlChange : 0);
	usbMIDI.setHandleProgramChange(			on ? deviceProgramChange : 0);
	usbMIDI.setHandleAfterTouchChannel(		on ? deviceAfterTouchChannel : 0);
	usbMIDI.setHandlePitchChange(			on ? devicePitchChange : 0);
	usbMIDI.setHandleSystemExclusive(		on ? deviceSystemExclusiveChunk : 0);
	usbMIDI.setHandleSystemExclusive(		on ? deviceSystemExclusive : 0);
	usbMIDI.setHandleTimeCodeQuarterFrame(	on ? deviceTimeCodeQuarterFrame : 0);
	usbMIDI.setHandleSongPosition(			on ? deviceSongPosition : 0);
	usbMIDI.setHandleSongSelect(			on ? deviceSongSelect : 0);
	usbMIDI.setHandleTuneRequest(			on ? deviceTuneRequest : 0);
	usbMIDI.setHandleClock(					on ? deviceClock : 0);
	usbMIDI.setHandleStart(					on ? deviceStart : 0);
	usbMIDI.setHandleContinue(				on ? deviceContinue : 0);
	usbMIDI.setHandleStop(					on ? deviceStop : 0);
	usbMIDI.setHandleActiveSensing(			on ? deviceActiveSensing : 0);
	usbMIDI.setHandleSystemReset(			on ? deviceSystemReset : 0);
	usbMIDI.setHandleRealTimeSystem(		on ? deviceRealTimeSystem : 0);

	#if WITH_MIDI_HOST
		// Only one of the System Exclusive handlers will actually be
		// used.  See the comments below for the difference between them.
		// This generic System Real Time handler is only used if the
		// more specific ones are not set.
	
		midi1.setHandleNoteOn(					on ? hostNoteOn : 0);
		midi1.setHandleNoteOff(					on ? hostNoteOff : 0);
		midi1.setHandleAfterTouchPoly(			on ? hostAfterTouchPoly : 0);
		midi1.setHandleControlChange(			on ? hostControlChange : 0);
		midi1.setHandleProgramChange(			on ? hostProgramChange : 0);
		midi1.setHandleAfterTouchChannel(		on ? hostAfterTouchChannel : 0);
		midi1.setHandlePitchChange(				on ? hostPitchChange : 0);
		midi1.setHandleSystemExclusive(			on ? hostSystemExclusiveChunk : 0);
		midi1.setHandleSystemExclusive(			on ? hostSystemExclusive : 0);
		midi1.setHandleTimeCodeQuarterFrame(	on ? hostTimeCodeQuarterFrame : 0);
		midi1.setHandleSongPosition(			on ? hostSongPosition : 0);
		midi1.setHandleSongSelect(				on ? hostSongSelect : 0);
		midi1.setHandleTuneRequest(				on ? hostTuneRequest : 0);
		midi1.setHandleClock(					on ? hostClock : 0);
		midi1.setHandleStart(					on ? hostStart : 0);
		midi1.setHandleContinue(				on ? hostContinue : 0);
		midi1.setHandleStop(					on ? hostStop : 0);
		midi1.setHandleActiveSensing(			on ? hostActiveSensing : 0);
		midi1.setHandleSystemReset(				on ? hostSystemReset : 0);
		midi1.setHandleRealTimeSystem(			on ? hostRealTimeSystem : 0);
	#endif
}

