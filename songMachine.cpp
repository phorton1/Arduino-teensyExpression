#include "songMachine.h"
#include "patchNewRig.h"
#include "myDebug.h"
#include "myTFT.h"

// following in songMachineParse.cpp

extern const char *tokenToString(int token_num);
extern bool openSongFile(const char *name);
extern bool parseSongText();

extern int song_text_len;
extern int song_code_len;
extern uint8_t song_code[MAX_SONG_CODE];

// globals here

char song_name[80] = {0};
char display_song_name[80] = {0};


//----------------------------------
// API
//----------------------------------

void songMachine::clear()
   // clear the currently running program, if any
{
    display(0,"songMachine::clear()",0);
    song_text_len = 0;
    song_code_len = 0;
    song_name[0] = 0;
}



bool songMachine::load()
    // load the test song, parse it, and prepare machine to run
    // will eventually have a UI for filenames and load any given song file
{
    display(0,"songMachine::load()",0);
    if (openSongFile("BigRiver") &&
        parseSongText())
    {
        // debug decode song_code

        #if 1
            int song_ptr = 0;
            while (song_ptr < song_code_len)
            {
                int ttype = song_code[song_ptr++];
                const char *tname = tokenToString(ttype);

                // those with string params

                if (ttype == TOKEN_DISPLAY)
                {
                    char buf[MAX_SONG_TOKEN+1];
                    int len = song_code[song_ptr++];
                    for (int i=0; i<len; i++)
                        buf[i] = song_code[song_ptr++];
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
                    int val = song_code[song_ptr++];
                    display(0,"   %s %d",tname,val);
                }

                // multiple parameters

                else if (ttype == TOKEN_LOOPER_CLIP)
                {
                    int clip_num = song_code[song_ptr++];
                    int mute = song_code[song_ptr++];
                    display(0,"   %s %d %d",tname,clip_num,mute);
                }

                // monadic outdented

                else if (ttype == TOKEN_BUTTON1 ||
                         ttype == TOKEN_LOOP)
                {
                    display(0,"%s",tname);
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
            }

            if (song_ptr != song_code_len)
            {
                my_error("ended at %d but song_code_len is %d",song_ptr,song_code_len);
            }

        #endif

        return true;
    }
    return false;
}


void songMachine::notifyPress()
    // for now we are overusing the songMachine button
    // long_click = load and start, or stop
{
    display(0,"songMachine::notifyPress()",0);
}

void songMachine::notifyLoop()
    // notify the songMachine that a loop has taken place
{
    display(0,"songMachine::notifyLoop()",0);
}


void songMachine::task()
    // called approx 30 times per second from patchNewRig::updateUI()
{
    if (strcmp(display_song_name,song_name))
    {
        strcpy(display_song_name,song_name);

        fillRect(song_rect,TFT_BLACK);
        mylcd.setFont(Arial_18_Bold);
        mylcd.Set_Text_colour(TFT_WHITE);
		mylcd.Print_String(song_name,song_rect.xs+5,song_rect.ys+4);
    }

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
