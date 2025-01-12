//-----------------------------------------
// myTFT.h - CHEAP TFT and TOUCH SCREEN
//-----------------------------------------
// Cheap Ardino 3.5" 320x480 TFT's
// Uses my modified version of LCDWIKI, which
// I was not using the myLCDWici TouchScreen.
// I had denormalized that into the TeensyExpression source.

#include "myTFT.h"


myLcdDevice mylcd(
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
    mylcd.begin();
    mylcd.setRotation(1);
    // mylcd.Set_Text_Mode(0);
    // starts with default my "m_use_bc=1"
    mylcd.fillScreen(TFT_BLACK);
}
