#include "songMachine.h"
#include "rigNew.h"
#include "myDebug.h"
#include "myTFT.h"
#include "myLEDS.h"
#include "songParser.h"

songMachine *theSongMachine = 0;


songMachine::songMachine()
{
    init();
    theSongMachine = this;
}


// static utils


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
    mylcd.setFont(Arial_18);
    mylcd.Set_Text_colour(TFT_RED);
    mylcd.Print_String("ERROR!",song_rect.xe-105,song_rect.ys+5);

    va_list var;
    va_start(var, format);
    #define DISPLAY_BUFFER_SIZE  255
    char display_buffer[DISPLAY_BUFFER_SIZE];
    vsprintf(display_buffer,format,var);

    mylcd.setFont(Arial_16);
    mylcd.Set_Text_colour(TFT_ORANGE);
    fillRect(song_msg_rect,TFT_BLACK);
    mylcd.Print_String(display_buffer,song_msg_rect.xs,song_msg_rect.ys);
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



//----------------------------------
// API
//----------------------------------


bool songMachine::load(const char *song_name)
    // load the test song, parse it, and prepare machine to run
    // will eventually have a UI for filenames and load any given song file
{
    display(0,"songMachine::load()",0);

    init();

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
    return false;
}


void songMachine::setMachineState(int state)
{
    m_state = state;
    display(0,"songMachine::setMachineState(%d)",state);
    if (m_state == SONG_STATE_EMPTY)
    {
        songParser::clear();
        init();
    }
    else
    {
        m_redraw = 1;
    }
}



//--------------------------
// events
//--------------------------

void songMachine::notifyPress(int button_num)
{
    display(0,"songMachine::notifyPress(%d)",button_num);
}

void songMachine::notifyLoop()
{
    display(0,"songMachine::notifyLoop()",0);
}




//------------------------------------------
// updateUI
//------------------------------------------

void songMachine::updateUI()
    // called approx 30 times per second from rigNew::updateUI()
{
    bool redraw = m_redraw;
    m_redraw = false;

    // song name

    if (redraw)
    {
        fillRect(song_rect,TFT_BLACK);
        mylcd.setFont(Arial_18_Bold);
        mylcd.Set_Text_colour(TFT_CYAN);
        mylcd.Print_String(songParser::getTheSongName(),song_rect.xs+5,song_rect.ys+4);
    }

    // state

    if (redraw || m_state != m_last_state)
    {
        m_last_state = m_state;
        mylcd.setFont(Arial_18_Bold);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.Fill_Rect(
            song_rect.xe-105,
            song_rect.ys+5,
            100,25,0);
        mylcd.Print_String(
            m_state == SONG_STATE_PAUSED ? "paused" :
            m_state == SONG_STATE_RUNNING ? "running" : "",
            song_rect.xe-105,
            song_rect.ys+5);
    }

    // buttons

    if (m_last_state == SONG_STATE_RUNNING)
    {
        bool show_leds = false;
        for (int i=0; i<NUM_SONG_BUTTONS; i++)
        {
            if (redraw ||
                m_last_button_color[i] != m_button_color[i])
            {
                show_leds = true;
                setLED(20+i,m_button_color[i]);
                m_last_button_color[i] = m_button_color[i];
            }
        }
        if (show_leds)
            showLEDs();
    }

}
