#include "midiHostConfig.h"
#include "defines.h"
#include "myLeds.h"


#define WIDTH           480
#define HEIGHT          320


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
	showLEDs();
}


// virtual
void midiHostConfig::updateUI()
{
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
			mylcd.print(midi_host_on ? "ON" : " is OFF!! it must be on!!");
		#else
			mylcd.print("WITH_MIDI_HOST is not defined!!");
		#endif
	}
}
