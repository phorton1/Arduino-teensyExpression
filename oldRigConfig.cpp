#include "oldRigConfig.h"

#include "myLeds.h"

//----------------
// synth
//----------------

#define SYNTH_PROGRAM_CHANNEL       7
    // program change come on specific channels
    
#define SYNTH_PATCH_PIANO1          0       // Mellow Grand 2
#define SYNTH_PATCH_ORGAN1          1       // Ballad B Pad
#define SYNTH_PATCH_BRASS1          2       // Drama Brass
#define SYNTH_PATCH_STRINGS1        3       // String Orch 1
#define SYNTH_PATCH_BASS1           4       // MM Bass Finger + Jazz Kit on ch10
#define SYNTH_PATCH_FLUTE1          5       // Orch Flute
#define SYNTH_PATCH_VOICES1         6       // Vocal Oh
#define SYNTH_PATCH_FX1             7       // Mega Motion 4

#define SYNTH_PATCH_EPIANO          8       // PIANO2 - Smooth FM Piano
#define SYNTH_PATCH_ORGAN2          9       // Drawbars Bow
#define SYNTH_PATCH_SPACE1          10      // BRASS2 = Mega Motion 3
#define SYNTH_PATCH_SPACE3          11      // STRINGS2 = Whispering Pad
#define SYNTH_PATCH_BASS2           12      // Chorus Fretless
#define SYNTH_PATCH_FLUTE2          13      // Psych Flute
#define SYNTH_PATCH_BASS_MINUS      14      // P Bass Finger (should be MM Bass Finger)
#define SYNTH_PATCH_FX2             15      // SFX Collection


int synth_pcs[NUM_BUTTON_COLS * 3] = {
    
    SYNTH_PATCH_BASS_MINUS,     // P Bass Finger  (should be MM Bass Finger)
    SYNTH_PATCH_BRASS1,         // Drama Brass
    SYNTH_PATCH_VOICES1,        // Vocal Oh - i
    SYNTH_PATCH_SPACE1,         // BRASS2 = Mega Motion 3
    SYNTH_PATCH_SPACE3,         // STRINGS2 = Whispering Pad
    
    SYNTH_PATCH_BASS2,          // Chorus Fretless
    SYNTH_PATCH_ORGAN2,         // Drawbars Bow
    SYNTH_PATCH_EPIANO,         // Smooth FM Piano
    SYNTH_PATCH_FLUTE2,         // Psych Flute
    SYNTH_PATCH_FX1,            // Mega Motion 4
    
    SYNTH_PATCH_BASS1,          // MM Bass Finger + Jazz Kit on ch10
    SYNTH_PATCH_ORGAN1,         // Ballad B Pad
    SYNTH_PATCH_PIANO1,         // Mellow Grand 2
    SYNTH_PATCH_FLUTE1,         // Orch Flute
    SYNTH_PATCH_FX2,            // SFX Collection
};


//----------------
// toneStack
//----------------

#define GUITAR_EFFECTS_CHANNEL  9

#define GUITAR_WAH_CONTROL_CC              11
#define GUITAR_REVERB_DEPTH_CC             20

#define GUITAR_DISTORTION_EFFECT_CC        26
#define GUITAR_WAH_EFFECT_CC               27
#define GUITAR_FLANGER_EFFECT_CC           28
#define GUITAR_CHORUS_EFFECT_CC            29
#define GUITAR_ECHO_EFFECT_CC              30

int guitar_effect_ccs[NUM_BUTTON_COLS] = {
    GUITAR_DISTORTION_EFFECT_CC,
    GUITAR_WAH_EFFECT_CC,       
    GUITAR_FLANGER_EFFECT_CC,
    GUITAR_CHORUS_EFFECT_CC,    
    GUITAR_ECHO_EFFECT_CC,      
};


//----------------
// Quantiloop
//----------------

#define LOOP_CONTROL_CHANNEL        9

#define LOOP_CONTROL_TRACK1         21
#define LOOP_CONTROL_TRACK2         22
#define LOOP_CONTROL_TRACK3         23
#define LOOP_CONTROL_TRACK4         31      // ADDED!!
    // There are three of these buttons assigned to tracks 1-3
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Rec-Play-Dub TrackN
    // Hold Action: Undo/Redo/Clear
    // Double Tap Action: Stop Track
#define LOOP_CONTROL_CLEAR_ALL      24
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Clear All
#define LOOP_STOP_START_IMMEDIATE   25
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Stop/Start Immediate
    // Hold Action: Clear All / End Song
    // Double Tap Action: Clear All / End Song
    
#define LOOP_CONTROL_VOLUME        7
    // Midi Command: CC     (not Note On/Off)
    // Type: Continous   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Momentary Action
    // 4 separate control assignments in Quantiloop
    // Action: Track Volume: Track 1
    // Action: Track Volume: Track 2
    // Action: Track Volume: Track 3
    // Action: Track Volume: Track 4
#define LOOP_DUB_MODE       17
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Dub Mode
#define LOOP_TAP_TEMPO      88
    // NOTE CONFLICT WITH WAH_EFFECT_CC, also on ch9
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Tap Tempo
    
int loop_ccs[NUM_BUTTON_COLS] =
{
    LOOP_CONTROL_TRACK1,
    LOOP_CONTROL_TRACK2,
    LOOP_CONTROL_TRACK3,
    LOOP_CONTROL_TRACK4,
    LOOP_STOP_START_IMMEDIATE,
};




oldRigConfig::oldRigConfig(expSystem *pSystem) :
    expConfig(pSystem)
{
    m_cur_patch_num = -1;    // 0..14
    for (int col=0; col<NUM_BUTTON_COLS; col++)
    {
        m_effect_toggle[col] = 0;
        m_loop_touched[col] = 0;
    }
    m_loop_last_touched = -1;
    m_loop_event_cleared = 0;

}
    

// virtual
void oldRigConfig::begin()
{
    expConfig::begin();

    rawButtonArray *ba = m_pSystem->getRawButtonArray();
    
    for (int col=0; col<NUM_BUTTON_COLS; col++)
    {
        ba->setButtonEventMask(0,col,BUTTON_EVENT_CLICK);
        ba->setButtonEventMask(1,col,BUTTON_EVENT_CLICK);
        ba->setButtonEventMask(2,col,BUTTON_EVENT_CLICK);
        ba->setButtonEventMask(3,col,BUTTON_EVENT_CLICK);
        ba->setButtonEventMask(4,col,  
            col == 4 ?
             (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK) : 
             (BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE) );
        
        setLED(3,col,m_effect_toggle[col] ? GREEN : 0);
        setLED(4,col,m_loop_touched[col] ? m_loop_last_touched==col ? RED : YELLOW : 0);
    }

    if (m_cur_patch_num >= 0)
    {
        int row = m_cur_patch_num / NUM_BUTTON_COLS;
        int col = m_cur_patch_num % NUM_BUTTON_COLS;
        setLED(row,col,BLUE);
    }

    showLEDs();
}


// virtual
void oldRigConfig::buttonEventHandler(int row, int col, int event)
{
    if (row < 3)
    {
        int new_patch_num = row * NUM_BUTTON_COLS + col;
        usbMIDI.sendProgramChange(synth_pcs[new_patch_num], SYNTH_PROGRAM_CHANNEL);
        if (new_patch_num != m_cur_patch_num)
        {
            int r = m_cur_patch_num / NUM_BUTTON_COLS;
            int c = m_cur_patch_num % NUM_BUTTON_COLS;
            setLED(r,c,0);
            setLED(row,col,BLUE);
            m_cur_patch_num = new_patch_num;
        }
    }
    else if (row == 3)
    {
        m_effect_toggle[col] = !m_effect_toggle[col];
        usbMIDI.sendControlChange(
            guitar_effect_ccs[col],
            m_effect_toggle[col] ? 0x7f : 0x00,
            GUITAR_EFFECTS_CHANNEL);
        setLED(row,col,m_effect_toggle[col] ? GREEN : 0);
    }
    else if (row == 4)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
            usbMIDI.sendControlChange(
                LOOP_CONTROL_CLEAR_ALL,
                0x7f,
                LOOP_CONTROL_CHANNEL);
            usbMIDI.sendControlChange(
                LOOP_CONTROL_CLEAR_ALL,
                0x00,
                LOOP_CONTROL_CHANNEL);
            
            for (int c=0; c<NUM_BUTTON_COLS; c++)
            {
                setLED(4,c,0);
                m_loop_touched[c] = 0;
                m_loop_last_touched = -1;
                m_loop_event_cleared = 1;
            }
        }
        else if (m_loop_event_cleared)
        {
            m_loop_event_cleared = 0;
        }
        else
        {
            if (event == BUTTON_EVENT_PRESS)
            {
                usbMIDI.sendControlChange(
                    loop_ccs[col],
                    0x7f,
                    LOOP_CONTROL_CHANNEL);
            }
            else // RELEASE or CLICK
            {
                // special handling for the right button
                // "CLICK" event to send the press ...
                
                if (event == BUTTON_EVENT_CLICK)
                    usbMIDI.sendControlChange(
                        loop_ccs[col],
                        0x7f,
                        LOOP_CONTROL_CHANNEL);
                
                usbMIDI.sendControlChange(
                    loop_ccs[col],
                    0x00,
                    LOOP_CONTROL_CHANNEL);


                if (m_loop_last_touched != -1)
                    setLED(row,m_loop_last_touched,YELLOW);
                setLED(row,col,RED);
                m_loop_touched[col] = 1;
                m_loop_last_touched = col;
            }
        }
    }
    showLEDs();
}

