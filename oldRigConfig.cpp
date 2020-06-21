#include "oldRigConfig.h"
#include <myDebug.h>
#include "defines.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"
#include "oldRig_defs.h"


#define SHOW_SENT_MIDI  1

//--------------------
// SampleTank
//--------------------

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
    
int loop_ccs[NUM_BUTTON_COLS] =
{
    LOOP_CONTROL_TRACK1,
    LOOP_CONTROL_TRACK2,
    LOOP_CONTROL_TRACK3,
    LOOP_CONTROL_TRACK4,
    LOOP_STOP_START_IMMEDIATE,
};







//====================================================================
// oldRigConfig
//====================================================================

bool full_redraw = -0;


oldRigConfig::oldRigConfig()
{
    m_cur_patch_num = -1;    // 0..14
}
    
    
#define BUTTON_TYPE_LOOPER      (BUTTON_EVENT_PRESS | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(7) )
#define BUTTON_TYPE_LOOP_CLEAR  (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(7) )
    

// virtual
void oldRigConfig::begin()
{
    expConfig::begin();
    full_redraw = 1;
    last_displayed_patch_num = -1;
    
    for (int i=0; i<15; i++)
    	theButtons.setButtonType(i,	BUTTON_TYPE_RADIO(1), 0);
    for (int i=15; i<19; i++)
        theButtons.setButtonType(i,	BUTTON_TYPE_TOGGLE | BUTTON_GROUP(2), 0, LED_GREEN);
    theButtons.setButtonType(19,    BUTTON_TYPE_TOGGLE | BUTTON_GROUP(2) | BUTTON_EVENT_LONG_CLICK, 0, LED_GREEN);
    
    for (int i=20; i<24; i++)
        theButtons.setButtonType(i,BUTTON_TYPE_LOOPER, 0, LED_RED, LED_YELLOW);
    theButtons.setButtonType(24,BUTTON_TYPE_LOOP_CLEAR, 0, LED_RED, LED_YELLOW);
    
    if (m_cur_patch_num >= 0)
        theButtons.select(m_cur_patch_num,1);

    showLEDs();
}

        


// virtual
void oldRigConfig::onButtonEvent(int row, int col, int event)
{
    if (row < 3)
    {
        m_cur_patch_num = row * NUM_BUTTON_COLS + col;
        usbMIDI.sendProgramChange(synth_patch[m_cur_patch_num].prog_num, SYNTH_PROGRAM_CHANNEL);
        
        #if SHOW_SENT_MIDI
            display(0,"sent MIDI PC(%d,%d)",
                SYNTH_PROGRAM_CHANNEL,
                synth_patch[m_cur_patch_num].prog_num);
        #endif

    }
    else if (row == 3)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)           // turn off all effects on long click
        {
            theButtons.clearRadioGroup(2);
            for (int c=0; c<NUM_BUTTON_COLS; c++)
            {
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
            }
        }
        else
        {
            int value = theButtons.getButton(row,col)->isSelected() ? 0x7f : 0;
            
            usbMIDI.sendControlChange(
                guitar_effect_ccs[col],
                value,
                GUITAR_EFFECTS_CHANNEL);
            #if SHOW_SENT_MIDI
                display(0,"sent MIDI CC(%d,%d,%d)",
                    GUITAR_EFFECTS_CHANNEL,
                    guitar_effect_ccs[col],
                    value);
            #endif
        }
    }
    else // (row == 4)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
            theButtons.clearRadioGroup(7);
            
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
        }
    }
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
            mylcd.printf_justified(
                i*120,
                240,
                120,
                30,
                LCD_JUST_CENTER,
                TFT_BLACK,
                TFT_YELLOW,
                "%s",
                thePedals.getPedal(i)->getName());
            
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
