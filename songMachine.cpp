#include "songMachine.h"
#include "rigNew.h"
#include "myDebug.h"
#include "myTFT.h"
#include "myLEDS.h"
#include "pedals.h"
#include "commonDefines.h"
#include "songParser.h"
#include "midiQueue.h"  // for sendSerialControlChange

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


#define DISPLAY_BUFFER_SIZE  255
char error_display_buffer[DISPLAY_BUFFER_SIZE];


void songMachine::error_msg(const char *format, ...)
{
    va_list var;
    va_start(var, format);
    vsprintf(error_display_buffer,format,var);
    theSongMachine->m_show_msg = error_display_buffer;
    theSongMachine->m_state |= SONG_STATE_ERROR;
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
        int start_offset = song_ptr;
        int ttype = songParser::getCode(song_ptr++);
        const char *tname = songParser::tokenToString(ttype);

        // those with string/identifier params

        if (ttype == TOKEN_DISPLAY)
        {
            const char *pstr = songParser::getCodeString(song_ptr);
            while (songParser::getCode(song_ptr)) song_ptr++; // skip to zero
            song_ptr++;     // skip the zero
            display(0,"%-5d:   %s '%s'",start_offset,tname,pstr);
        }

        // those with long integer params

        else if (ttype == TOKEN_GOTO)
        {
            int offset = songParser::getCodeInteger(song_ptr);
            song_ptr += 2;
            display(0,"%-5d:   %s %d",start_offset,tname,offset);
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
            display(0,"%-5d:   %s %d",start_offset,tname,val);
        }

        // multiple parameters

        else if (ttype == TOKEN_LOOPER_CLIP)
        {
            int clip_num = songParser::getCode(song_ptr++);
            int mute = songParser::getCode(song_ptr++);
            display(0,"%-5d:   %s %d,%d",start_offset,tname,clip_num,mute);
        }
        else if (ttype == TOKEN_BUTTON_COLOR)
        {
            int button_num = songParser::getCode(song_ptr++);
            int color = songParser::getCode(song_ptr++);
            display(0,"%-5d:   %s %d,%s",start_offset,tname,button_num,songParser::tokenToString(color));
        }

        // monadic outdented

        else if (ttype == TOKEN_BUTTON1 ||
                 ttype == TOKEN_BUTTON2 ||
                 ttype == TOKEN_BUTTON3 ||
                 ttype == TOKEN_BUTTON4 ||
                 ttype == TOKEN_LOOP)
        {
            display(0,"%-5d: %s:",start_offset,tname);
        }

        //  other monadic

        else if (ttype == TOKEN_GUITAR_EFFECT_NONE ||
                 ttype == TOKEN_CLEAR_LOOPER ||
                 ttype == TOKEN_LOOPER_STOP ||
                 ttype == TOKEN_DUB_MODE ||
                 ttype == TOKEN_LOOPER_SET_START_MARK)
        {
            display(0,"%-5d:   %s",start_offset,tname);
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
    // one based
{
    display(0,"songMachine::notifyPress(%d)",button_num);

    // Find the next occurance of the button, if any
    // give an warning message if not found

    int find_op = TOKEN_BUTTON1 + button_num - 1;
    int found_offset = -1;
    int find_ptr = m_code_ptr;
    int code_len = songParser::codeLen();
    while (find_ptr < code_len)
    {
        if (songParser::getCode(find_ptr) == find_op)
        {
            found_offset = find_ptr;
            break;
        }
        find_ptr++;
    }

    if (found_offset >= 0)
    {
        m_code_ptr = found_offset+1;            // skip the opcode
        m_state &= ~SONG_STATE_WAITING_BUTTON;  // clear the wait state
    }
    else
    {
        m_show_msg = "warning: could not find button";
    }
}


void songMachine::notifyLoop()
{
    display(0,"songMachine::notifyLoop()",0);
    if (m_state & SONG_STATE_WAITING_LOOP)
    {
        if (songParser::getCode(m_code_ptr) == TOKEN_LOOP)
        {
            m_state &= ~SONG_STATE_WAITING_LOOP;
            m_code_ptr++;
        }
        else
        {
            song_error("notifyLoop expected TOKEN_LOOP at offset %d",m_code_ptr);
        }
    }
}




//------------------------------------------
// updateUI
//------------------------------------------

void songMachine::updateUI()
    // called approx 30 times per second from rigNew::updateUI()
{
    bool redraw = m_redraw;
    m_redraw = false;

    // run the machine

    if (m_state && !(m_state & (SONG_STATE_FINISHED | SONG_STATE_ERROR)))
        runMachine();

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

        int color = m_state & SONG_STATE_ERROR ?
            TFT_RED :
            TFT_YELLOW;

        mylcd.Set_Text_colour(color);
        mylcd.Fill_Rect(
            song_rect.xe-105,
            song_rect.ys+5,
            100,25,0);
        mylcd.Print_String(
            m_state & SONG_STATE_ERROR ? "ERROR" :
            m_state & SONG_STATE_FINISHED ? "DONE" :
            m_state & SONG_STATE_PAUSED ? "paused" :
            m_state & SONG_STATE_WAITING_BUTTON ? "button" :
            m_state & SONG_STATE_WAITING_LOOP   ? "looping" :
            m_state & SONG_STATE_RUNNING ? "running" : "",
            song_rect.xe-105,
            song_rect.ys+5);
    }

    // message

    if (redraw || m_last_show_msg != m_show_msg)
    {
        m_last_show_msg = m_show_msg;

        int color = m_state & SONG_STATE_ERROR ?
            TFT_ORANGE : TFT_WHITE;

        mylcd.setFont(Arial_16);
        mylcd.Set_Text_colour(color);
        fillRect(song_msg_rect,TFT_BLACK);
        mylcd.Print_String(m_last_show_msg?m_last_show_msg:"",song_msg_rect.xs,song_msg_rect.ys);
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


//================================================
// the machine
//================================================

void songMachine::runMachine()
{
    if (m_state && !(m_state & (SONG_STATE_PAUSED | SONG_STATE_FINISHED | SONG_STATE_ERROR)))
    {
        if (m_code_ptr >= songParser::codeLen())
        {
            m_state |= SONG_STATE_FINISHED;
        }
        else
        {
            int c = songParser::getCode(m_code_ptr);
            if (c >= TOKEN_BUTTON1 && c <= TOKEN_BUTTON4)
            {
                m_state |= SONG_STATE_WAITING_BUTTON;
            }
            else if (c == TOKEN_LOOP)
            {
                m_state |= SONG_STATE_WAITING_LOOP;
            }
            else
            {
                m_code_ptr++;
                doSongOp(c);

            }   // not waiting for a button or loop
        }   //  code left to run
    }   // running and not finished
}



int songMachine::tokenToColor(int ttype)
{
    if (ttype == TOKEN_RED    ) return LED_RED;
    if (ttype == TOKEN_GREEN  ) return LED_GREEN;
    if (ttype == TOKEN_BLUE   ) return LED_BLUE;
    if (ttype == TOKEN_YELLOW ) return LED_YELLOW;
    if (ttype == TOKEN_PURPLE ) return LED_PURPLE;
    if (ttype == TOKEN_ORANGE ) return LED_ORANGE;
    if (ttype == TOKEN_WHITE  ) return LED_WHITE;
    if (ttype == TOKEN_CYAN   ) return LED_CYAN;
    if (ttype == TOKEN_BLACK  ) return 0;

    song_error("unexpected color token 0x%02x %s at offset %d",
        ttype,
        songParser::tokenToString(ttype),
        m_code_ptr);

    return 0;
}







void songMachine::doSongOp(int op)
{
    switch (op)
    {
        case TOKEN_DISPLAY:
            m_show_msg = songParser::getCodeString(m_code_ptr);
            while (songParser::getCode(m_code_ptr)) m_code_ptr++;   // skip to zero
            m_code_ptr++;   // skip the zero
            break;

        case TOKEN_BUTTON_COLOR:
        {
            int button_num = songParser::getCode(m_code_ptr++) - 1;
            m_button_color[button_num] = tokenToColor(songParser::getCode(m_code_ptr++));
            break;
        }

        case TOKEN_GOTO:
            m_code_ptr = songParser::getCodeInteger(m_code_ptr);
            break;

        case TOKEN_LOOP_VOLUME:
        case TOKEN_SYNTH_VOLUME:
        case TOKEN_GUITAR_VOLUME:
        {
            int pedal =
                op == TOKEN_LOOP_VOLUME ? PEDAL_LOOP :
                op == TOKEN_SYNTH_VOLUME ? PEDAL_SYNTH :
                PEDAL_GUITAR;
            int value = songParser::getCode(m_code_ptr++);
            thePedals.getPedal(pedal)->setDisplayValue(value);
            thePedals.pedalEvent(pedal,value);
            break;
        }

        case TOKEN_GUITAR_EFFECT_NONE:
            theNewRig->clearGuitarEffects(false);
            break;

        case TOKEN_GUITAR_EFFECT_DISTORT:
        case TOKEN_GUITAR_EFFECT_WAH:
        case TOKEN_GUITAR_EFFECT_CHORUS:
        case TOKEN_GUITAR_EFFECT_ECHO:
        {
            int effect_num = op - TOKEN_GUITAR_EFFECT_DISTORT;
            int value = songParser::getCode(m_code_ptr++);
            theNewRig->setGuitarEffect(effect_num, value);
            break;
        }

        case TOKEN_SYNTH_PATCH:
            theNewRig->setPatchNumber(songParser::getCode(m_code_ptr++));
            break;

        case TOKEN_CLEAR_LOOPER:
            theNewRig->clearLooper(false);
            break;

        case TOKEN_LOOPER_STOP:
			sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_STOP,"songMachine STOP");
            break;
        case TOKEN_DUB_MODE:
			sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_DUB_MODE,"songMachine DUB");
            break;

        case TOKEN_LOOPER_SET_START_MARK:
            // not implemented yet
            break;

        case TOKEN_LOOPER_TRACK:
        {
			int track_num = songParser::getCode(m_code_ptr++)-1;
            theNewRig->selectTrack(track_num);
            break;
        }
        case TOKEN_LOOPER_CLIP:
        {
            int layer_num = songParser::getCode(m_code_ptr++)-1;
            int mute_value = songParser::getCode(m_code_ptr++);
            theNewRig->setClipMute(layer_num,mute_value);
            break;
        }


        default:
            song_error("UNEXPECTED OPCODE: %d: 0x%02x-%s",m_code_ptr,op,songParser::tokenToString(op));
    }
}
