
#include "configEditors.h"
#include <myDebug.h>

bool terminal_mode_redraw_needed = false;


void navInteger(configOption *caller, int num)
{
    // display(0,"navInteger(%d)",num);
    
    if (num == BUTTON_MOVE_LEFT)
    {
        in_terminal_mode = false;
        display_menu = 0;
        display_option = 0;        
    }
    if (num == BUTTON_MOVE_UP ||
        num == BUTTON_MOVE_DOWN)
    {
        int inc = num == BUTTON_MOVE_UP ? 1 : -1;
        caller->setValue(caller->getValue() + inc);
    }
    
}
void drawInteger(configOption *caller)
{
    if (terminal_mode_draw_needed)
    {
        terminal_mode_draw_needed = false;
        terminal_mode_redraw_needed = true;
        
        mylcd.Fill_Rect(0,0,WIDTH,HEIGHT,0);
        mylcd.setFont(Arial_32_Bold);
        mylcd.printf_justified(
            30,
            30,
            WIDTH - 60,
            80,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            TFT_BLACK,
            "%s",
            caller->getTitle());
    }
    
    if (terminal_mode_redraw_needed ||
        caller->needsValueDisplay())
    {
        caller->clearDisplayValue();
        terminal_mode_redraw_needed = false;
        mylcd.setFont(Arial_48);
        mylcd.printf_justified(
            180,
            100,
            120,
            80,
            LCD_JUST_CENTER,
            TFT_WHITE,
            TFT_BLACK,
            "%s",
            caller->getValueString());
    }
}
