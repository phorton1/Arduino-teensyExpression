#include "songMachine.h"
#include "rigNew.h"
#include "myDebug.h"
#include "myTFT.h"
#include "songParser.h"

// globals here

int song_machine_state = SONG_STATE_EMPTY;
int songMachine::getMachineState()
{
    return song_machine_state;
}

void songMachine::setMachineState(int state)
{
    song_machine_state = state;
    if (state == SONG_STATE_EMPTY)
    {
        clear();
    }
    else
    {
        mylcd.setFont(Arial_18_Bold);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.Fill_Rect(
            song_rect.xe-105,
            song_rect.ys+5,
            100,25,0);

        mylcd.Print_String(
            song_machine_state==SONG_STATE_PAUSED ?
                "paused" : "running",
            song_rect.xe-105,
            song_rect.ys+5);
    }
}


void songMachine::uc(char *buf)
{
	while (*buf)
	{
		if (*buf >= 'a' && *buf <= 'z')
			*buf = *buf - 'a' + 'A';
		buf++;
	}
}

void songMachine::error_msg(const char *format, ...)
{
    mylcd.setFont(Arial_18_Bold);
    mylcd.Set_Text_colour(TFT_RED);
    mylcd.Print_String("ERROR!",song_rect.xe-105,song_rect.ys+5);

    va_list var;
    va_start(var, format);
    #define DISPLAY_BUFFER_SIZE  255
    char display_buffer[DISPLAY_BUFFER_SIZE];
    vsprintf(display_buffer,format,var);

    fillRect(song_msg_rect,TFT_BLACK);

    mylcd.setFont(Arial_18);
    mylcd.Set_Text_colour(TFT_RED);

    int y = 0;
    int offset = 0;
    int len = strlen(display_buffer);
    while (offset < len && y < song_msg_rect.ye)
    {
        char buf[51];
        int size = len > 50 ? 50 : len;
        strncpy(buf,&display_buffer[offset],size);
        buf[size] = 0;
        offset += size;

		mylcd.Print_String(buf,song_msg_rect.xs+5,song_msg_rect.ys + y);
        y += 20;
    }
}


//----------------------------------
// API
//----------------------------------

void songMachine::clear()
   // clear the currently running program, if any
{
    display(0,"songMachine::clear()",0);
    song_machine_state = SONG_STATE_EMPTY;
    songParser::clear();
    fillRect(song_rect,TFT_BLACK);
    fillRect(song_msg_rect,TFT_BLACK);
}



bool songMachine::load(const char *song_name)
    // load the test song, parse it, and prepare machine to run
    // will eventually have a UI for filenames and load any given song file
{
    display(0,"songMachine::load()",0);
    clear();
    mylcd.setFont(Arial_18_Bold);
    mylcd.Set_Text_colour(TFT_CYAN);
    mylcd.Print_String(song_name,song_rect.xs+5,song_rect.ys+4);

    if (songParser::openSongFile(song_name))
    {
        #if 1
            dumpCode();
        #endif

        setMachineState(SONG_STATE_RUNNING);
        return true;
    }
    clear();
    return false;
}


void songMachine::notifyPress(int button_num)
{
    display(0,"songMachine::notifyPress(%d)",button_num);
}

void songMachine::notifyLoop()
    // notify the songMachine that a loop has taken place
{
    display(0,"songMachine::notifyLoop()",0);
}


void songMachine::task()
    // called approx 30 times per second from rigNew::updateUI()
{
}


//------------------------------------------
// debugging
//------------------------------------------


void songMachine::dumpCode()
{
    int song_ptr = 0;
    display(0,"------------------- %s.song -------------------",songParser::getTheSongName());

    while (song_ptr < songParser::codeLen())
    {
        int ttype = songParser::getCode(song_ptr++);
        const char *tname = songParser::tokenToString(ttype);

        // those with string/identifier params

        if (ttype == TOKEN_DISPLAY ||
            ttype == TOKEN_GOTO)
        {
            char buf[MAX_SONG_TOKEN+1];
            int len = songParser::getCode(song_ptr++);
            for (int i=0; i<len; i++)
                buf[i] = songParser::getCode(song_ptr++);
            buf[len] = 0;
            display(0,"   %s %d:'%s'",tname,len,buf);
        }

        // those with single integer params

        else if (ttype == TOKEN_LOOP_VOLUME ||
                 ttype == TOKEN_SYNTH_VOLUME ||
                 ttype == TOKEN_GUITAR_VOLUME ||
                 ttype == TOKEN_GUITAR_EFFECT_DISTORT ||
                 ttype == TOKEN_GUITAR_EFFECT_WAH     ||
                 ttype == TOKEN_GUITAR_EFFECT_CHORUS  ||
                 ttype == TOKEN_GUITAR_EFFECT_ECHO    ||
                 ttype == TOKEN_LOOPER_TRACK          ||
                 ttype == TOKEN_SYNTH_PATCH)
        {
            int val = songParser::getCode(song_ptr++);
            display(0,"   %s %d",tname,val);
        }

        // multiple parameters

        else if (ttype == TOKEN_LOOPER_CLIP)
        {
            int clip_num = songParser::getCode(song_ptr++);
            int mute = songParser::getCode(song_ptr++);
            display(0,"   %s %d,%d",tname,clip_num,mute);
        }
        else if (ttype == TOKEN_BUTTON_COLOR)
        {
            int button_num = songParser::getCode(song_ptr++);
            int color = songParser::getCode(song_ptr++);
            display(0,"   %s %d,%s",tname,button_num,songParser::tokenToString(color));
        }

        // monadic outdented

        else if (ttype == TOKEN_BUTTON1 ||
                 ttype == TOKEN_BUTTON2 ||
                 ttype == TOKEN_BUTTON3 ||
                 ttype == TOKEN_BUTTON4 ||
                 ttype == TOKEN_LOOP)
        {
            display(0,"%s:",tname);
        }

        //  other monadic

        else if (ttype == TOKEN_GUITAR_EFFECT_NONE ||
                 ttype == TOKEN_CLEAR_LOOPER ||
                 ttype == TOKEN_LOOPER_STOP ||
                 ttype == TOKEN_DUB_MODE ||
                 ttype == TOKEN_LOOPER_SET_START_MARK)
        {
            display(0,"   %s",tname);
        }
        else    // anything else is bogus
        {
            my_error("unexpected token at code_len(%d): %s",song_ptr-1,tname);
        }

        // prh GOTO
    }

    if (song_ptr != songParser::codeLen())
    {
        my_error("ended at %d but song_code_len is %d",song_ptr,songParser::codeLen());
    }

    display(0,"------------------- end of %s.song ---------------------",songParser::getTheSongName());
}



//--------------------------------------
// NOTES
//--------------------------------------

#if 0

    int i = findPatchByName("PIANO2");
    if (i != -1)
    {
        setPatchNumber(i);
    }

    // OR

    if (fade_vol_time)
    {
        fade_vol_time = 0;
    }
    else
    {
        fade_vol_time = millis();
        start_fade_vol = thePedals.getPedal(1)->getDisplayValue();
        if (end_fade_vol)
        {
            end_fade_vol = 0;
        }
        else
        {
            end_fade_vol = 127;
        }
    }
    // test a fade over 10 seconds
#endif

#if 0
	int end_fade_vol = 0;
	int start_fade_vol = 0;
	uint32_t fade_vol_time = 0;
#endif


#if 0
    if (fade_vol_time)
    {
        uint32_t now = millis();
        uint32_t elapsed = now - fade_vol_time;
        if (elapsed > 10000)
        {
            fade_vol_time = 0;
        }
        else
        {
            float pct = ((float) elapsed) / 10000.00;
            float range = abs(end_fade_vol - start_fade_vol);

            int use_vol;
            if (end_fade_vol)		// fading up
            {
                use_vol = pct * range;
            }
            else
            {
                use_vol = (1.0-pct) * range;
            }

            if (use_vol != thePedals.getPedal(1)->getDisplayValue())
            {
                thePedals.getPedal(1)->setDisplayValue(use_vol);
                thePedals.pedalEvent(1,use_vol);
            }
        }

    }





// The songMachine sets pedal volumes by calling
//
// 	   thePedals.getPedal(1)->setDisplayValue(use_vol) and
//     thePedals.pedalEvent(1,use_vol) directly.
//
// If the pedal is touched it should stop any volume fading
// in progress for that pedal.  Hence it keeps it's own
// copy of expressionPedal->getValue(), and, if at anytime
// during a fade, that value changes, it should cease and not
// send any more pedal values.

// The song machine compiler calls theNewRig::findPatchByName(identifier)
// to get the patch number, which becomes the opcode operand, and then
// calls theNewRig setPatchNumber() to change it, which not only sends
// out the proper patch change message and polymode, but will cause the
// display to update as the songMachine runs.


#endif
