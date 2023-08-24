#include "myTFT.h"
#include "defines.h"


LCDWIKI_KBV mylcd(
    ILI9486,
    TFT_CS,
    TFT_CD_RS,
    TFT_WR,
    TFT_RD,
    TFT_RESET);


void initMyTFT()
{
    setTFTDataPins(
        TFT_DATA0,
        TFT_DATA1,
        TFT_DATA2,
        TFT_DATA3,
        TFT_DATA4,
        TFT_DATA5,
        TFT_DATA6,
        TFT_DATA7);
    mylcd.Init_LCD();
    mylcd.Set_Rotation(1);
    // mylcd.Set_Text_Mode(0);
    // starts with default my "m_use_bc=1"
    mylcd.Fill_Screen(0);
}
