#include "songMachine.h"
#include "myDebug.h"
#include "myTFT.h"
#include "myLEDS.h"
#include "commonDefines.h"
#include "songParser.h"
#include "rigBase.h"

// prh - the machine should keep track of what buttons are available
// at any given time and respond to the WHITE presses of unused ones
// by setting them back to BLACK.

// prh - Add CLIP_VOLUME layer_num, volume [,delay]
// and change LOOPER_CLIP to CLIP_MUTE


#define dbg_machine   0
#define dbg_vols   1

#define MIN_TIME_TWEEN_VOL_CMD_MILLIS    40         // at least 3 buffers on the looper


songMachine *theSongMachine = 0;


songMachine::songMachine()
{
    m_pBaseRig = 0;
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
    theSongMachine->m_show_msg[1] = error_display_buffer;
    theSongMachine->m_show_color[1] = TFT_ORANGE;
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
            int display_num = songParser::getCode(song_ptr++);
            const char *pstr = songParser::getCodeString(song_ptr);
            while (songParser::getCode(song_ptr)) song_ptr++; // skip to zero
            song_ptr++;     // skip the zero
            int color = songParser::getCode(song_ptr++);

            display(0,"%-5d:   %s %d,'%s',%s",
                start_offset,
                tname,
                display_num,
                pstr,
                color ? songParser::tokenToString(color) : "0");
        }

        // those with long integer params

        else if (ttype == TOKEN_GOTO ||
                 ttype == TOKEN_CALL)
        {
            int offset = songParser::getCodeInteger(song_ptr);
            song_ptr += 2;
            display(0,"%-5d:   %s %d",start_offset,tname,offset);
        }

        // those with single integer params

        else if (ttype == TOKEN_GUITAR_EFFECT_DISTORT ||
                 ttype == TOKEN_GUITAR_EFFECT_WAH     ||
                 ttype == TOKEN_GUITAR_EFFECT_CHORUS  ||
                 ttype == TOKEN_GUITAR_EFFECT_ECHO    ||
                 ttype == TOKEN_LOOPER_TRACK          ||
                 ttype == TOKEN_SYNTH_PATCH           ||
                 ttype == TOKEN_DELAY)
        {
            int val = songParser::getCode(song_ptr++);
            display(0,"%-5d:   %s %d",start_offset,tname,val);
        }

        // two integer params

        else if (ttype == TOKEN_LOOP_VOLUME ||
                 ttype == TOKEN_SYNTH_VOLUME ||
                 ttype == TOKEN_GUITAR_VOLUME ||
                 ttype == TOKEN_LOOPER_CLIP)
        {
            int p1 = songParser::getCode(song_ptr++);
            int p2 = songParser::getCode(song_ptr++);
            display(0,"%-5d:   %s %d,%d",start_offset,tname,p1,p2);
        }

        // others

        else if (ttype == TOKEN_BUTTON_COLOR)
        {
            int button_num = songParser::getCode(song_ptr++);
            int color = songParser::getCode(song_ptr++);
            int flash = songParser::getCode(song_ptr++);
            display(0,"%-5d:   %s %d,%s%s",
                start_offset,
                tname,
                button_num,
                songParser::tokenToString(color),
                flash ? ",FLASH" : "");
        }

        // monadic outdented

        else if (ttype == TOKEN_METHOD ||
                 ttype == TOKEN_BUTTON1 ||
                 ttype == TOKEN_BUTTON2 ||
                 ttype == TOKEN_BUTTON3 ||
                 ttype == TOKEN_BUTTON4 ||
                 ttype == TOKEN_LOOP)
        {
            display(0,"%-5d: %s:",start_offset,tname);
        }


        else if (ttype == TOKEN_END_METHOD ||
                 ttype == TOKEN_GUITAR_EFFECT_NONE ||
                 ttype == TOKEN_CLEAR_LOOPER ||
                 ttype == TOKEN_LOOPER_STOP ||
                 ttype == TOKEN_LOOPER_STOP_IMMEDIATE  ||
                 ttype == TOKEN_LOOP_IMMEDIATE  ||
                 ttype == TOKEN_DUB_MODE ||
                 ttype == TOKEN_LOOPER_SET_START_MARK)
        {
            display(0,"%-5d:   %s",start_offset,tname);
        }
        else    // anything else is bogus
        {
            my_error("unexpected token at code_len(%d): %s",song_ptr-1,tname);
        }

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
    display(dbg_machine,"songMachine::load()",0);

    init();

    mylcd.setFont(Arial_18_Bold);
    mylcd.Set_Text_colour(TFT_CYAN);
    mylcd.Print_String(song_name,song_title_rect.xs+5,song_title_rect.ys+4);

    if (songParser::openSongFile(song_name) &&
        songParser::parseSongText(m_pBaseRig))
    {
        if (dbg_machine <= 0)
        {
            dumpCode();
        }

        setMachineState(SONG_STATE_RUNNING);
        return true;
    }
    return false;
}


void songMachine::setMachineState(int state)
{
    m_state = state;
    display(dbg_machine,"songMachine::setMachineState(%d)",state);
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
    display(dbg_machine,"songMachine::notifyPress(%d)",button_num);

    // Find the next occurance of the button, if any
    // give an warning message if not found

    int find_op = TOKEN_BUTTON1 + button_num - 1;
    int found_offset = -1;
    int find_ptr = m_code_ptr;
    int code_len = songParser::codeLen();
    while (find_ptr < code_len)
    {
        int c = songParser::getCode(find_ptr);
        if (c == find_op)
        {
            found_offset = find_ptr;
            break;
        }
        else if (c == TOKEN_METHOD || c == TOKEN_END_METHOD)
        {
            // no given buttons in scope
            break;
        }
        find_ptr++;
    }

    if (found_offset >= 0)
    {
        m_code_ptr = found_offset+1;            // skip the opcode
        m_state &= ~SONG_STATE_WAITING_BUTTON;  // clear the wait state
        m_delay = 0;        // loop or button clears any current delay
    }
    else
    {
        m_show_msg[1] = "warning: could not find button";
        m_show_color[1] = TFT_RED;
    }
}


void songMachine::notifyLoop()
{
    display(dbg_machine,"songMachine::notifyLoop()",0);
    if (m_state & SONG_STATE_WAITING_LOOP)
    {
        if (songParser::getCode(m_code_ptr) == TOKEN_LOOP)
        {
            m_delay = 0;        // loop or button clears any current delay
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

#define BUTTON_FLASH_TIME   300


void songMachine::updateUI()
    // called approx 30 times per second from rigNew::updateUI()
{
    bool redraw = m_redraw;
    m_redraw = false;

    // run the machine

    if (m_state && !(m_state & (SONG_STATE_FINISHED | SONG_STATE_ERROR)))
    {
        // handle any pedal automations

        for (int pedal=0; pedal<NUM_PEDALS; pedal++)
        {
            if (pedal == PEDAL_WAH) continue;       // not in songMachine

            pedal_volume_t *pv = &pedal_volumes[pedal];

            if (pv->last_val != pv->to_val)
            {
                uint32_t now = millis();
                if (now >= pv->last_cmd_time +
                    MIN_TIME_TWEEN_VOL_CMD_MILLIS)
                {
                    int32_t elapsed = now - pv->event_time;
                    int32_t delay_millis = pv->delay_tenths * 100;
                    int32_t range = pv->to_val - pv->from_val;
                    int32_t inc = (range * elapsed) / delay_millis;
                    int32_t new_val = pv->from_val + inc;

                    if (range > 0 && new_val > pv->to_val)
                        new_val = pv->to_val;
                    if (range < 0 && new_val < pv->to_val)
                        new_val = pv->to_val;

                    if (new_val < 0) new_val = 0;
                    if (new_val > 127) new_val = 127;

                    if (new_val != pv->last_val)
                    {
                        display(dbg_vols,"updating pedal(%d) new_val=%d  elapsed=%d dmillis=%d range=%d inc=%d",
                            pedal,
                            new_val,
                            elapsed,
                            delay_millis,
                            range,
                            inc);

                        thePedals.getPedal(pedal)->setDisplayValue(new_val);
                        thePedals.pedalEvent(pedal,new_val);
                        pedal_volumes[pedal].last_val = new_val;
                        pedal_volumes[pedal].last_cmd_time = millis();
                    }
                }
            }
        }

        // USER PROGRAMMED DELAY STOPS OPCODES
        // delay is in 10's of a second

        int tenths = m_delay_time/100;
        if (!m_delay || tenths > m_delay)
        {
            m_delay = 0;
            runMachine();
        }
    }

    // don't update the UI if in quick mode

    if (!m_pBaseRig || !m_pBaseRig->songUIAvailable())
        return;

    // invariant flasher


    bool flash_changed = false;
    if (button_flash_time > BUTTON_FLASH_TIME)
    {
        flash_changed = true;
        button_flash_state = !button_flash_state;
        button_flash_time = 0;
    }


    // song name

    if (redraw)
    {
        fillRect(song_title_rect,TFT_BLACK);
        mylcd.setFont(Arial_18_Bold);
        mylcd.Set_Text_colour(TFT_CYAN);
        mylcd.Print_String(songParser::getTheSongName(),song_title_rect.xs+5,song_title_rect.ys+4);
    }

    // state

    if (redraw || m_state != m_last_state)
    {
        m_last_state = m_state;
        mylcd.setFont(Arial_18_Bold);

        int color = m_state & SONG_STATE_ERROR ?
            TFT_RED :
            TFT_YELLOW;

        const char *msg =
            m_state & SONG_STATE_ERROR ? "error" :
            m_state & SONG_STATE_FINISHED ? "done" :
            m_state & SONG_STATE_PAUSED ? "paused" :
            m_state & SONG_STATE_WAITING_BUTTON ? "button" :
            m_state & SONG_STATE_WAITING_LOOP   ? "looping" :
            m_state & SONG_STATE_RUNNING ? "running" : "";

        #if 1
            mylcd.print_justified(
                song_state_rect.xs,
                song_state_rect.ys,
                song_state_rect.width()-5,
                song_state_rect.height(),
                LCD_JUST_RIGHT,
                color,
                TFT_BLACK,
                true,
                (char *)msg);
        #else
            mylcd.Set_Text_colour(color);
            fillRect(song_state_rect,TFT_BLACK);
            mylcd.Print_String(
                msg,
                song_state_rect.xs,
                song_state_rect.ys+4);
        #endif
    }

    // messages

    for (int i=0; i<2; i++)
    {
        if (redraw ||
            m_last_show_msg[i] != m_show_msg[i] ||
            m_last_show_color[i] != m_show_color[i])
        {
            m_last_show_msg[i] = m_show_msg[i];
            m_last_show_color[i] = m_show_color[i];

            mylcd.setFont(i?Arial_24:Arial_20_Bold);
            #if 1
                const char *msg = m_last_show_msg[i] ? m_last_show_msg[i] : "";
                mylcd.print_justified(
                    song_msg_rect[i].xs+5,
                    song_msg_rect[i].ys,
                    song_msg_rect[i].width()-5,
                    song_msg_rect[i].height()-5,
                    i ? LCD_JUST_CENTER : LCD_JUST_LEFT,
                    m_last_show_color[i],
                    TFT_BLACK,
                    true,
                    (char *) msg);
            #else
                mylcd.Set_Text_colour(m_last_show_color[i]);
                fillRect(song_msg_rect[i],TFT_BLACK);
                mylcd.Print_String(
                    m_last_show_msg[i] ? m_last_show_msg[i] : "",
                    song_msg_rect[i].xs+5,
                    song_msg_rect[i].ys+5);
            #endif
        }
    }

    // buttons

    if (m_last_state && !(m_last_state & SONG_STATE_PAUSED))
    {
        bool show_leds = false;
        for (int i=0; i<NUM_SONG_BUTTONS; i++)
        {
            if (redraw ||
                m_last_button_color[i] != m_button_color[i] ||
                (flash_changed && m_button_flash[i]))
            {
                int color = m_button_flash[i] && !button_flash_state ?
                    0 : m_button_color[i];

                setLED(20+i,color);
                m_last_button_color[i] = m_button_color[i];
                show_leds = true;
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



int songMachine::tokenToLEDColor(int ttype)
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

    song_error("unexpected led color token 0x%02x %s at offset %d",
        ttype,
        songParser::tokenToString(ttype),
        m_code_ptr);

    return 0;
}

int songMachine::tokenToTFTColor(int ttype)
{
    if (ttype == TOKEN_RED    ) return TFT_RED;
    if (ttype == TOKEN_GREEN  ) return TFT_GREEN;
    if (ttype == TOKEN_BLUE   ) return TFT_BLUE;
    if (ttype == TOKEN_YELLOW ) return TFT_YELLOW;
    if (ttype == TOKEN_PURPLE ) return TFT_PURPLE;
    if (ttype == TOKEN_ORANGE ) return TFT_ORANGE;
    if (ttype == TOKEN_WHITE  ) return TFT_WHITE;
    if (ttype == TOKEN_CYAN   ) return TFT_CYAN;
    if (ttype == TOKEN_BLACK  ) return 0;

    song_error("unexpected tft color token 0x%02x %s at offset %d",
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
        {
            int display_num = songParser::getCode(m_code_ptr++);
            m_show_msg[display_num-1] = songParser::getCodeString(m_code_ptr);
            while (songParser::getCode(m_code_ptr)) m_code_ptr++;   // skip to zero
            m_code_ptr++;   // skip the zero
            int color = songParser::getCode(m_code_ptr++);
            if (color == 0)
            {
                if (display_num == 1)
                    color = TFT_GREEN;
                else // if (display_num == 2)
                    color = TFT_WHITE;
            }
            else
            {
                color = tokenToTFTColor(color);
            }
            m_show_color[display_num-1] = color;
            break;
        }

        case TOKEN_BUTTON_COLOR:
        {
            int button_num = songParser::getCode(m_code_ptr++) - 1;
            m_button_color[button_num] = tokenToLEDColor(songParser::getCode(m_code_ptr++));
            m_button_flash[button_num] = songParser::getCode(m_code_ptr++);
            break;
        }

        case TOKEN_DELAY:
            m_delay = songParser::getCode(m_code_ptr++);
            m_delay_time = 0;
            break;

        case TOKEN_GOTO:
            m_code_ptr = songParser::getCodeInteger(m_code_ptr);
            break;
        case TOKEN_CALL:
            if (m_num_calls >= MAX_CALL_STACK)
            {
                song_error("too many nested CALLS at offset &d",m_code_ptr);
            }
            else
            {
                // jump to AFTER the TOKEN_METHOD
                int to_ptr = songParser::getCodeInteger(m_code_ptr) + 1;
                m_code_ptr += 2;

                m_call_stack[m_num_calls++] = m_code_ptr;
                display(dbg_machine,"call(%d) from %d to %d",m_num_calls,m_code_ptr,to_ptr);
                m_code_ptr = to_ptr;
            }
            break;

        case TOKEN_METHOD:
        {
            // skip inline methods in execution

            display(dbg_machine,"skipping method at offset %d",m_code_ptr);
            int start_offset = m_code_ptr;
            int len = songParser::codeLen();
            while (m_code_ptr<len && op != TOKEN_END_METHOD)
                op = songParser::getCode(m_code_ptr++);
            if (m_code_ptr >= len)
            {
                song_error("Could not find END_METHOD after offset %d",start_offset);
            }
            else
            {
                display(dbg_machine,"    resuming at offset %d",m_code_ptr);
            }
            break;
        }

        case TOKEN_END_METHOD:
            if (!m_num_calls)
            {
                song_error("call stack underflow for END_METHOD at offset %d",m_code_ptr);
            }
            else
            {
                int ret_loc = m_call_stack[m_num_calls-1];
                display(dbg_machine,"end_method(%d) returning to location %d",m_num_calls,ret_loc);
                m_code_ptr = ret_loc;
                m_num_calls--;
            }
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
            int delay_tenths = songParser::getCode(m_code_ptr++);

            // issue first increment command

            expressionPedal *the_pedal = thePedals.getPedal(pedal);
            int cur_val = the_pedal->getDisplayValue();

            if (!delay_tenths)
            {
                display(dbg_vols,"IMMEDIATE TOKEN_VOLUME[%d] new_value=%d  cur_val=%d  delay=%d tenths",pedal,value,cur_val,delay_tenths);
                if (cur_val != value)
                {
                    the_pedal->setDisplayValue(value);
                    thePedals.pedalEvent(pedal,value);
                }
                pedal_volumes[pedal].last_val = value;
            }
            else
            {
                display(dbg_vols,"TOKEN_VOLUME[%d] new_value=%d  cur_val=%d  delay=%d tenths",pedal,value,cur_val,delay_tenths);
                pedal_volumes[pedal].last_val = cur_val;
            }

            pedal_volumes[pedal].from_val = cur_val;
            pedal_volumes[pedal].to_val = value;
            pedal_volumes[pedal].delay_tenths = delay_tenths;
            pedal_volumes[pedal].event_time = millis();
            pedal_volumes[pedal].last_cmd_time = 0;
            break;
        }

        case TOKEN_GUITAR_EFFECT_NONE:
            if (m_pBaseRig)
                m_pBaseRig->clearGuitarEffects(false);
            break;

        case TOKEN_GUITAR_EFFECT_DISTORT:
        case TOKEN_GUITAR_EFFECT_WAH:
        case TOKEN_GUITAR_EFFECT_CHORUS:
        case TOKEN_GUITAR_EFFECT_ECHO:
        {
            int effect_num = op - TOKEN_GUITAR_EFFECT_DISTORT;
            int value = songParser::getCode(m_code_ptr++);
            if (m_pBaseRig)
                m_pBaseRig->setGuitarEffect(effect_num, value);
            break;
        }

        case TOKEN_SYNTH_PATCH:
            if (m_pBaseRig)
                m_pBaseRig->setPatchNumber(songParser::getCode(m_code_ptr++));
            break;

        case TOKEN_CLEAR_LOOPER:
            if (m_pBaseRig)
                m_pBaseRig->clearLooper(false);
            break;

        case TOKEN_LOOPER_STOP:
            if (m_pBaseRig)
                m_pBaseRig->stopLooper();
            break;
        case TOKEN_LOOPER_STOP_IMMEDIATE:
            if (m_pBaseRig)
                m_pBaseRig->stopLooperImmediate();
            break;
        case TOKEN_LOOP_IMMEDIATE:
            if (m_pBaseRig)
                m_pBaseRig->loopImmediate();
            break;
        case TOKEN_DUB_MODE:
            if (m_pBaseRig)
                m_pBaseRig->toggleDubMode();
            break;
        case TOKEN_LOOPER_SET_START_MARK:
            if (m_pBaseRig)
                m_pBaseRig->setStartMark();
            break;

        case TOKEN_LOOPER_TRACK:
        {
			int track_num = songParser::getCode(m_code_ptr++)-1;
            if (m_pBaseRig)
                m_pBaseRig->selectTrack(track_num);
            break;
        }
        case TOKEN_LOOPER_CLIP:
        {
            int layer_num = songParser::getCode(m_code_ptr++)-1;
            int mute_value = songParser::getCode(m_code_ptr++);
            if (m_pBaseRig)
                m_pBaseRig->setClipMute(layer_num,mute_value);
            break;
        }


        default:
            song_error("UNEXPECTED OPCODE: %d: 0x%02x-%s",m_code_ptr,op,songParser::tokenToString(op));
    }
}
