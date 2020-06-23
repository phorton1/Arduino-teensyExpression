#include <myDebug.h>
#include "expDialogs.h"
#include "myLeds.h"
#include "buttons.h"
#include "myTFT.h"


#define BUTTON_NO    16
#define BUTTON_YES   18


// virtual
void yesNoDialog::begin(bool warm)
{
    theButtons.setButtonType(BUTTON_NO,  BUTTON_EVENT_CLICK, LED_RED);
    theButtons.setButtonType(BUTTON_YES, BUTTON_EVENT_CLICK, LED_GREEN);
    showLEDs();
}


// virtual
void yesNoDialog::onButtonEvent(int row, int col, int event)
{
    int num = row * NUM_BUTTON_COLS + col;
    endModal(num == BUTTON_YES ? 1 : 0);
}


// virtual
void yesNoDialog::updateUI()
{
    if (m_draw_needed)
    {
        m_draw_needed = 0;
        mylcd.setFont(Arial_16_Bold);
        mylcd.printf_justified(
            60,
            100,
            360,
            260,
            LCD_JUST_CENTER,
            TFT_WHITE,
            TFT_BLACK,
            false,
            "%s",
            m_text);
    }
}



