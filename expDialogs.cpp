#include <myDebug.h>
#include "expDialogs.h"
#include "myLeds.h"
#include "buttons.h"
#include "myTFT.h"

#define MAX_PRINTF_STRING  1024

#define BUTTON_NO    16
#define BUTTON_YES   18




yesNoDialog::yesNoDialog(uint32_t id, const char *name, const char *format, ...) :
    expWindow(WIN_FLAG_DELETE_ON_END)
{
    m_id = id;
    m_name = name;
    m_format = format;
    m_draw_needed = 1;
    va_start(m_params,format);
}

       


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
        mylcd.printfv_justified(
            60,
            100,
            360,
            260,
            LCD_JUST_CENTER,
            TFT_WHITE,
            TFT_BLACK,
            false,
            m_format,
            m_params);
    }
}



