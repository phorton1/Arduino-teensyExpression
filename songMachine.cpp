#include "songMachine.h"
#include "patchNewRig.h"
#include "myDebug.h"

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


void songMachine::clear()
   // clear the currently running program, if any
{
    display(0,"songMachine::clear()",0);
}

bool songMachine::load()
    // load the test song, parse it, and prepare machine to run
    // will eventually have a UI for filenames and load any given song file
{
    display(0,"songMachine::load()",0);
    return true;
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
}



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
#endif



