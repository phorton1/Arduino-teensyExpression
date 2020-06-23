#include "myTFT.h"
#include "defines.h"


LCDWIKI_KBV mylcd(
    ILI9486,
    CHEAP_TFT_CS,
    CHEAP_TFT_CD_RS,
    CHEAP_TFT_WR,
    CHEAP_TFT_RD,
    CHEAP_TFT_RESET);
    
    
void initMyTFT()
{
    setTFTDataPins(
        CHEAP_TFT_DATA0,
        CHEAP_TFT_DATA1,
        CHEAP_TFT_DATA2,
        CHEAP_TFT_DATA3,
        CHEAP_TFT_DATA4,
        CHEAP_TFT_DATA5,
        CHEAP_TFT_DATA6,
        CHEAP_TFT_DATA7);
    mylcd.Init_LCD();
    mylcd.Set_Rotation(1);
    // mylcd.Set_Text_Mode(0);
    // starts with default my "m_use_bc=1"
    mylcd.Fill_Screen(0);
}
