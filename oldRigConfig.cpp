#include "oldRigConfig.h"
#include <myDebug.h>
#include "defines.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"


#define SHOW_SENT_MIDI  0



//----------------
// synth
//----------------

#define SYNTH_PROGRAM_CHANNEL       7       // 1 based
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

#define SYNTH_VOLUME_CHANNEL        1
#define SYNTH_VOLUME_CC             7


typedef struct
{
    int prog_num;
    const char *short_name;
    const char *long_name;
}   synthPatch_t;


synthPatch_t synth_patch[NUM_BUTTON_COLS * 3] = {
    {SYNTH_PATCH_BASS_MINUS,     "BASS_MINUS",  "P Bass Finger"},  // should be MM Bass Finger
    {SYNTH_PATCH_BRASS1,         "BRASS1",      "Drama Brass"},
    {SYNTH_PATCH_VOICES1,        "VOICES1",     "Vocal Oh"},
    {SYNTH_PATCH_SPACE1,         "SPACE1",      "Mega Motion 3"},   // was BRASS2
    {SYNTH_PATCH_SPACE3,         "SPACE3",      "Whispering Pad"},  // was STRINGS2
                                  
    {SYNTH_PATCH_BASS2,          "BASS2",       "Chorus Fretless"},
    {SYNTH_PATCH_ORGAN2,         "ORGAN2",      "Drawbars Bow"},
    {SYNTH_PATCH_EPIANO,         "EPIANO",      "Smooth FM Piano"},
    {SYNTH_PATCH_FLUTE2,         "FLUTE2",      "Psych Flute"},
    {SYNTH_PATCH_FX1,            "SPACE2",      "Mega Motion 4"},
                                  
    {SYNTH_PATCH_BASS1,          "BASS1",       "MM Bass Finger"},  //  + Jazz Kit
    {SYNTH_PATCH_ORGAN1,         "ORGAN1",      "Ballad B Pad"},
    {SYNTH_PATCH_PIANO1,         "PIANO1",      "Mellow Grand 2"},
    {SYNTH_PATCH_FLUTE1,         "FLUTE1",      "Orch Flute"},
    {SYNTH_PATCH_FX2,            "FX2",         "SFX Collection"},
};  

int last_displayed_patch_num = -1;


//----------------
// toneStack
//----------------

#define GUITAR_EFFECTS_CHANNEL  9   // one based

#define GUITAR_WAH_CONTROL_CC              11
#define GUITAR_REVERB_DEPTH_CC             20

#define GUITAR_DISTORTION_EFFECT_CC        26
#define GUITAR_WAH_EFFECT_CC               27
#define GUITAR_FLANGER_EFFECT_CC           28
#define GUITAR_CHORUS_EFFECT_CC            29
#define GUITAR_ECHO_EFFECT_CC              30

#define GUITAR_VOLUME_CHANNEL               7       // one based
#define GUITAR_VOLUME_CC                    7


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

#define LOOP_CONTROL_CHANNEL        9   // one based

#define LOOP_CONTROL_TRACK1         21
#define LOOP_CONTROL_TRACK2         22
#define LOOP_CONTROL_TRACK3         23
#define LOOP_CONTROL_TRACK4         31      // ADDED!!
    // There are three of these buttons assigned to tracks 1-3
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Rec-Play-Dub TrackN
    // Hold Action: Undo/Red
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
    
#define LOOP_VOLUME_CC        7
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


//--------------------------
// NOTES
//--------------------------
// MPD218
//
//      Buttons 0-15 (from bottom left) are Channel 7 "PROGRAM messages 0..15
//      The unused knobs are assigned t0 Channel 1 CCs: 3,9   12,13   14,15
//
// SOFTSTEP
//
//      The guitar pedals are programmed on the Softstep as
//           Data Source: "FootOn"  (Gain 1.00 offset 0)
//           Table: Toggle (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (26 thru 30) Channel 9
//           With "LED" Settings
//              Key Name: DIST, WAH, PHAS, CHOR, DLY  (display mode: Always)
//              Green Led: True, Red Led: None (on the assigned layer)
        
//      The Quantiloop Buttons are setup on the Softstep as 
//           Data Source: "FootOn"  (Gain 1.00 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (21 thru 25) Channel 9, 
//           With "LED" Settings
//              Key Name: DIST, WAH, PHAS, CHOR, DLY (display mode: Always)
//              Green Led: None, Red True: None (on the assigned layer)
//      
//      The weird programming for the Arrow Button is
//      Line1
//           Data Source: "Pedal"  (Gain 1.18 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (channel 11, 21 thru 25)
//      Line2 - controls Reverb Depth
//           Data Source: "NavY Inc/Dec"  (Gain 1.00 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC 20 Channel 9
//           With "LED" Settings
//              Prefix: V   Display Mode: Immed Para   Key Name: Exp
//              Key Name: DIST, WAH, PHAS, CHOR, DLY (display mode: Always)
//              Green Led: None, Red True: None (on the assigned layer)


//------------------------------------
// Pedals
//------------------------------------

typedef struct
{
    const char *name;
    int     last_displayed;
    int     channel;
    int     cc_num;
}   oldRigPedal_t;


oldRigPedal_t rig_pedal[NUM_PEDALS] = {
    {"SYNTH",     -1,  SYNTH_VOLUME_CHANNEL,      SYNTH_VOLUME_CC}, 
    {"LOOP",      -1,  LOOP_CONTROL_CHANNEL,      LOOP_VOLUME_CC}, 
    {"WAH",       -1,  GUITAR_EFFECTS_CHANNEL,    GUITAR_WAH_CONTROL_CC}, 
    {"GUITAR",    -1,  GUITAR_VOLUME_CHANNEL,     GUITAR_VOLUME_CC}};     







//====================================================================
// oldRigConfig
//====================================================================

bool full_redraw = -0;


oldRigConfig::oldRigConfig()
{
    for (int col=0; col<NUM_BUTTON_COLS; col++)
    {
        m_effect_toggle[col] = 0;
        m_loop_touched[col] = 0;
    }
    m_cur_patch_num = -1;    // 0..14
    m_loop_last_touched = -1;
}
    

// virtual
void oldRigConfig::begin()
{
    expConfig::begin();
    full_redraw = 1;
    last_displayed_patch_num = -1;

    for (int col=0; col<NUM_BUTTON_COLS; col++)
    {
        theButtons.setButtonEventMask(0,col,BUTTON_EVENT_CLICK);
        theButtons.setButtonEventMask(1,col,BUTTON_EVENT_CLICK);
        theButtons.setButtonEventMask(2,col,BUTTON_EVENT_CLICK);
        theButtons.setButtonEventMask(3,col,
            col == 4 ?
             (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK) :
             BUTTON_EVENT_CLICK);
        
        theButtons.setButtonEventMask(4,col,  
            col == 4 ?
             (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK) : 
             (BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE) );
        
        setLED(3,col,m_effect_toggle[col] ? LED_GREEN : 0);
        setLED(4,col,m_loop_touched[col] ? m_loop_last_touched==col ? LED_RED : LED_YELLOW : 0);
    }

    if (m_cur_patch_num >= 0)
    {
        int row = m_cur_patch_num / NUM_BUTTON_COLS;
        int col = m_cur_patch_num % NUM_BUTTON_COLS;
        setLED(row,col,LED_BLUE);
    }

    showLEDs();
}

        


// virtual
void oldRigConfig::onButtonEvent(int row, int col, int event)
{
    if (row < 3)
    {
        int new_patch_num = row * NUM_BUTTON_COLS + col;
        
        usbMIDI.sendProgramChange(synth_patch[new_patch_num].prog_num, SYNTH_PROGRAM_CHANNEL);
        
        #if SHOW_SENT_MIDI
            display(0,"sent MIDI PC(%d,%d)",
                SYNTH_PROGRAM_CHANNEL,
                synth_patch[new_patch_num].prog_num);
        #endif
        
        int r = m_cur_patch_num / NUM_BUTTON_COLS;
        int c = m_cur_patch_num % NUM_BUTTON_COLS;
        setLED(r,c,0);
        setLED(row,col,LED_BLUE);
        m_cur_patch_num = new_patch_num;
    }
    else if (row == 3)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)           // turn off all effects on long click
        {
            for (int c=0; c<NUM_BUTTON_COLS; c++)
            {
                m_effect_toggle[c] = 0;
                
                usbMIDI.sendControlChange(
                    guitar_effect_ccs[c],
                    0x00,
                    GUITAR_EFFECTS_CHANNEL);
                #if SHOW_SENT_MIDI
                    display(0,"sent MIDI CC(%d,%d,%d)",
                        GUITAR_EFFECTS_CHANNEL,
                        guitar_effect_ccs[c],
                        0x00);
                #endif
        
                setLED(row,c,0);
            }
        }
        else
        {
            m_effect_toggle[col] = !m_effect_toggle[col];
            
            usbMIDI.sendControlChange(
                guitar_effect_ccs[col],
                m_effect_toggle[col] ? 0x7f : 0x00,
                GUITAR_EFFECTS_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    GUITAR_EFFECTS_CHANNEL,
                    guitar_effect_ccs[col],
                    m_effect_toggle[col] ? 0x7f : 0x00);
            #endif
    
            setLED(row,col,m_effect_toggle[col] ? LED_GREEN : 0);
        }
    }
    else if (row == 4)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
            usbMIDI.sendControlChange(
                LOOP_CONTROL_CLEAR_ALL,
                0x7f,
                LOOP_CONTROL_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    LOOP_CONTROL_CHANNEL,
                    LOOP_CONTROL_CLEAR_ALL,
                    0x7f);
            #endif
            
            usbMIDI.sendControlChange(
                LOOP_CONTROL_CLEAR_ALL,
                0x00,
                LOOP_CONTROL_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    LOOP_CONTROL_CHANNEL,
                    LOOP_CONTROL_CLEAR_ALL,
                    0x00);
            #endif
            
            for (int c=0; c<NUM_BUTTON_COLS; c++)
            {
                setLED(4,c,0);
                m_loop_touched[c] = 0;
                m_loop_last_touched = -1;
            }
        }
        else if (event == BUTTON_EVENT_PRESS)
        {
            usbMIDI.sendControlChange(
                loop_ccs[col],
                0x7f,
                LOOP_CONTROL_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    LOOP_CONTROL_CHANNEL,
                    loop_ccs[col],
                    0x7f);
            #endif
        }
        else // RELEASE or CLICK
        {
            // special handling for the right button
            // "CLICK" event to send the press ...
            
            if (event == BUTTON_EVENT_CLICK)
            {
                usbMIDI.sendControlChange(
                    loop_ccs[col],
                    0x7f,
                    LOOP_CONTROL_CHANNEL);
                #if SHOW_SENT_MIDI
                    display(0,"sent MIDI CC(%d,%d,%d)",
                        LOOP_CONTROL_CHANNEL,
                        loop_ccs[col],
                        0x7f);
                #endif
            }
            
            usbMIDI.sendControlChange(
                loop_ccs[col],
                0x00,
                LOOP_CONTROL_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    LOOP_CONTROL_CHANNEL,
                    loop_ccs[col],
                    0);
            #endif

            if (m_loop_last_touched != -1)
                setLED(row,m_loop_last_touched,LED_YELLOW);
            setLED(row,col,LED_RED);
            m_loop_touched[col] = 1;
            m_loop_last_touched = col;
        }
    }
    showLEDs();
}



// virtual
void oldRigConfig::onPedalEvent(int num, int val)
{
    usbMIDI.sendControlChange(
        rig_pedal[num].cc_num,
        val,
        rig_pedal[num].channel);
    #if SHOW_SENT_MIDI
        display(0,"pedal(%d,%s) sent MIDI CC(%d,%d,%d)",
            num,
            rig_pedal[num].name,
            rig_pedal[num].channel,
            rig_pedal[num].cc_num,
            val);
    #endif
}



// virtual
void oldRigConfig::updateUI()
{
    bool draw_full = false;
    if (full_redraw)
    {
        draw_full = true;
        full_redraw = false;
        mylcd.Fill_Rect(0,230,480,30,TFT_YELLOW);
        mylcd.setFont(Arial_16_Bold);   // Arial_16);
        mylcd.Set_Text_colour(0);
        mylcd.Set_Draw_color(TFT_YELLOW);
        for (int i=0; i<NUM_PEDALS; i++)
        {
            rig_pedal[i].last_displayed = -1;
            
            #if 1
                mylcd.printf_justified(
                    i*120,
                    240,
                    120,
                    30,
                    LCD_JUST_CENTER,
                    TFT_BLACK,
                    TFT_YELLOW,
                    "%s",
                    rig_pedal[i].name);
            #else
                int offset = strlen(rig_pedal[i].name) < 5 ? 12 : 0;
                mylcd.Set_Text_Cursor(25+i*120+offset, 230+6);
                mylcd.print(rig_pedal[i].name);
            #endif            
            
            if (i && i<NUM_PEDALS)
                mylcd.Draw_Line(i*120,260,i*120,mylcd.Get_Display_Height()-1);
        }
    }
    
    #if WITH_PEDALS

        bool font_set = false;
        for (int i=0; i<4; i++)
        {
            expressionPedal *pedal = thePedals.getPedal(i);
            if (draw_full || pedal->displayValueChanged())
            {
                pedal->clearDisplayValueChanged();
                int v = pedal->getValue();

                #if SHOW_SENT_MIDI
                    // shows the frequency of UI vs MIDI messages on pedals
                    display(0,"updateUI pedal(%d) changed to %d",i,v);
                #endif
                
                if (!font_set)
                {
                    mylcd.setFont(Arial_40_Bold);   // Arial_40);
                    mylcd.Set_Text_colour(TFT_WHITE);
                    font_set = 1;                    
                }
               
                mylcd.printf_justified(
                    12+i*120,
                    260+14,
                    100,
                    45,
                    LCD_JUST_CENTER,
                    TFT_WHITE,
                    TFT_BLACK,
                    "%d",
                    v);
            }
        }
        
    #endif  // WITH_PEDALS
        

    if (m_cur_patch_num >= 0 &&
        (draw_full ||
         last_displayed_patch_num != m_cur_patch_num))
    {
        last_displayed_patch_num = m_cur_patch_num;
        mylcd.setFont(Arial_40_Bold);   // Arial_40);
        int y = 75;
        mylcd.printf_justified(
            0,y,mylcd.Get_Display_Width(),mylcd.getFontHeight(),
            LCD_JUST_CENTER,
            TFT_CYAN,
            TFT_BLACK,
            "%s",
            synth_patch[m_cur_patch_num].short_name);

        y += mylcd.getFontHeight();
        mylcd.setFont(Arial_32);   // Arial_40);
        mylcd.printf_justified(
            0,y,mylcd.Get_Display_Width(),mylcd.getFontHeight(),
            LCD_JUST_CENTER,
            TFT_MAGENTA,
            TFT_BLACK,
            "%s",
            synth_patch[m_cur_patch_num].long_name);
    }
    
}
