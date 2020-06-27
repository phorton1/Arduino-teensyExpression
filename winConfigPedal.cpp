
#include <myDebug.h>
#include "winConfigPedal.h"
#include "myTFT.h"
#include "myLeds.h"
#include "pedals.h"
#include "buttons.h"

//----------------------------------
// Pedals
//----------------------------------
// for the time being there is one set of values for the pedals
// across all patches.  An idea is then to have "pedal sets"
// that can be shared between different patches while still
// allowing for multiple definitions.

#define KEYPAD_UP      12
#define KEYPAD_DOWN    22
#define KEYPAD_LEFT    16
#define KEYPAD_RIGHT   18
#define KEYPAD_SELECT  17

#define CANCEL_BUTTON  3
#define NEXT_PEDAL_BUTTON  0



// virtual
const char *winConfigPedal::name()
{
    static char buf[120];
    sprintf(buf,"Configure Pedal %d (%s)",
        m_pedal_num + 1,
        thePedals.getPedal(m_pedal_num)->getName());
    return buf;
}



winConfigPedal::winConfigPedal(int i) :
    expWindow(WIN_FLAG_DELETE_ON_END)
{
    m_pedal_num = i;
}




// virtual
void winConfigPedal::begin(bool warm)
{
    display(0,"winConfigPedal(%d) pedal=%d",warm,m_pedal_num);
	expWindow::begin(warm);

    m_redraw_curve = 1;
    m_display_curve = -1;
    m_display_item = -1;
    m_cur_curve = getPref8(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_CURVE_TYPE_OFFSET);
    m_curve_names = getPrefStrings(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_CURVE_TYPE_OFFSET);
    getPrefPedalPoints();
    setEditPoints();

    m_cur_item   = m_num_points;        // start on MAX point
    m_cur_point  = -1;                  // -1 when not selected for editing

	theButtons.setButtonType(KEYPAD_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_SELECT,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	theButtons.setButtonType(NEXT_PEDAL_BUTTON,	BUTTON_TYPE_CLICK);
    theButtons.setButtonType(CANCEL_BUTTON,     BUTTON_TYPE_CLICK,  LED_ORANGE);
	theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	showLEDs();
}


void winConfigPedal::setEditPoints()
    // called when the curve changes
{
    m_num_points = m_cur_curve + 2;     // number of points for drawing
    m_num_items  = m_num_points + 1;    // number of items to scroll thru
    clearPrevPoints();
}



void winConfigPedal::clearPrevPoints()
{
    for (int i=0; i<MAX_CURVE_POINTS; i++)
    {
        m_prev_points[i].x = 255;
        m_prev_points[i].y = 255;
    }
}


void winConfigPedal::getPrefPedalPoints()
{

    display(0,"PREF_PEDAL(%d)=%d",m_pedal_num,PREF_PEDAL(m_pedal_num));
    display(0,"sizeof(m_points)=%d,  CURVE_BYTES_PER_PEDAL=%d ",sizeof(m_points),CURVE_BYTES_PER_PEDAL);

    uint8_t *p = (uint8_t *) m_points;
    for (int i=0; i<CURVE_BYTES_PER_PEDAL; i++)
    {
        *p++ = getPref8(PREF_PEDAL(m_pedal_num)+PREF_PEDAL_POINTS_OFFSET+i);
    }
    display_bytes(0,"buf",(uint8_t *) m_points, 48);

}




//---------------------------------------------------
// BUTTONS
//---------------------------------------------------


// virtual
void winConfigPedal::onButtonEvent(int row, int col, int event)
{
    int num = row * NUM_BUTTON_COLS + col;

	if (num == KEYPAD_UP || num == KEYPAD_DOWN)
	{
        int inc = num == KEYPAD_UP ? -1 : 1;
        if (m_cur_point >= 0)
        {
            prefCurvePoint_t *points = getCurvePoints();
            int y = points[m_cur_point].y;
            y += -inc;

            // restrict to above button to left and below button to right

            int right_y = m_cur_point < m_num_points-1 ? points[m_cur_point+1].y-1 : 127;
            int left_y = m_cur_point ? points[m_cur_point-1].y+1 : 0;

            if (y > right_y) y = right_y;
            if (y < left_y) y = left_y;

            if (y != points[m_cur_point].y)
            {
                points[m_cur_point].y = y;
                setPref8(PREF_PEDAL_CURVE_POINT(m_pedal_num,m_cur_curve,m_cur_point) + PEDAL_POINTS_OFFSET_Y, y);
                m_redraw_curve = 1;
            }
        }
        else
        {
            m_cur_item += inc;
            if (m_cur_item >= m_num_items) m_cur_item = 0;
            if (m_cur_item < 0) m_cur_item = m_num_items - 1;
       }
	}
	else if (num == KEYPAD_LEFT || num == KEYPAD_RIGHT)
	{
        if (m_cur_point > 0 &&                  // not on min or max
            m_cur_point < m_num_points -1)
        {
            prefCurvePoint_t *points = getCurvePoints();
            int x = points[m_cur_point].x;
            int inc = num == KEYPAD_LEFT ? -1 : 1;
            x += inc;

            // restrict to above button to left and below button to right

            int right_x = m_cur_point < m_num_points-1 ? points[m_cur_point+1].x-1 : 127;
            int left_x = m_cur_point ? points[m_cur_point-1].x+1 : 0;

            if (x > right_x) x = right_x;
            if (x < left_x) x = left_x;

            if (x != points[m_cur_point].x)
            {
                points[m_cur_point].x = x;
                setPref8(PREF_PEDAL_CURVE_POINT(m_pedal_num,m_cur_curve,m_cur_point) + PEDAL_POINTS_OFFSET_X, x);
                m_redraw_curve = 1;
            }
        }
        else if (m_cur_point == -1)
        {

        }
	}
    else if (num == KEYPAD_SELECT)
    {
        if (m_cur_point >= 0)
        {
            m_cur_point = -1;
            m_display_item = -1;
        }
        else if (m_cur_item)
        {
            clearPrevPoints();
            m_cur_point = m_cur_item - 1;
            m_display_item = -1;
        }
        else
        {
            m_cur_curve = (m_cur_curve + 1) % MAX_PEDAL_CURVES;
            setPrefPedalCurve(m_pedal_num, m_cur_curve);
            setEditPoints();
        }
    }

    // finishes

	else if (num == THE_SYSTEM_BUTTON)
	{
		endModal(237);
    }
    else if (num == CANCEL_BUTTON)
	{
        // restore just the pedals prefs, and then
        // call the prefs.cpp function setDefaultPrefs()
        // to reset the actual default pref values.

        int pref = PREF_PEDAL(m_pedal_num);
        for (int i=0; i<PREF_BYTES_PER_PEDAL; i++)
            restore_pref8(pref+i);
        setDefaultPrefs();

		endModal(237);
    }
	else if (num == NEXT_PEDAL_BUTTON)
	{
        int next_num = (m_pedal_num+1) % NUM_PEDALS;
		theSystem.swapModal(new winConfigPedal(next_num),0);
	}
}




//---------------------------------------------------
// DRAW
//---------------------------------------------------


#define TEXT_MARGIN  20
#define TEXT_SPACING 40

#define X_OFFSET    160
#define Y_OFFSET    50
#define CHART_MAX   255


const char *getPointName(int curve, int point)
{
    if (!point) return "min";
    if (point == curve + 1) return "max";
    if (curve==2 && point==1) return "left";
    return curve==2 ? "right" : "mid";
}

void winConfigPedal::showSelectedItem(int item, int selected)
{
    const char *name = "huh?";
    if (!item)
    {
        name = m_curve_names[m_cur_curve];
    }
    else
    {
        name = getPointName(m_cur_curve,item-1);
    }

    int x = TEXT_MARGIN;
    int y = Y_OFFSET + item * TEXT_SPACING;
    int w = X_OFFSET - 2 * TEXT_MARGIN;
    int h = 30;

    int fc = TFT_WHITE;
    int bc = selected ? TFT_BLUE : TFT_BLACK;

    if (selected && item && item-1 == m_cur_point)
    {
        fc = TFT_BLUE;
        bc = TFT_YELLOW;
    }

    mylcd.setFont(Arial_16_Bold);
    mylcd.Fill_Rect(x,y,w,h,bc);
    mylcd.printf_justified(
        x+5,
        y+7,
        w-10,
        h-6,
        LCD_JUST_CENTER,
        fc,
        bc,
        false,
        name);
}




// virtual
void winConfigPedal::updateUI()
{
    // take copies of all variables for protection

    __disable_irq();

    int  prev_item = m_display_item;
    bool full_redraw = m_display_curve != m_cur_curve;
    bool select_changed = m_display_item != m_cur_item;
    bool redraw_curve = m_redraw_curve;
    prefCurvePoint_t points[MAX_CURVE_POINTS];
    prefCurvePoint_t prev_points[MAX_CURVE_POINTS];
    memcpy(points,getCurvePoints(),PREF_BYTES_PER_CURVE);
    memcpy(prev_points,m_prev_points,PREF_BYTES_PER_CURVE);

    // and set "previous" variables as soon as possible

    m_redraw_curve = 0;
    m_display_item = m_cur_item;
    m_display_curve = m_cur_curve;
    memcpy(m_prev_points,points,PREF_BYTES_PER_CURVE);
    __enable_irq();

    // clear

    if (full_redraw)
    {
        mylcd.Fill_Rect(0,Y_OFFSET,480,320-Y_OFFSET,0);
    }

    // selected item

    for (int i=0; i<m_num_items; i++)
    {
        if (full_redraw || (
            select_changed && (
                i == m_cur_item ||
                i == prev_item)))
        {
            showSelectedItem(i,i == m_cur_item);
        }
    }

    // X Y Weight Text

    if (full_redraw || select_changed)
    {
        mylcd.Fill_Rect(60,255,60,60,0);
        if (m_cur_item)
        {
            mylcd.setFont(Arial_12);
            mylcd.Set_Text_colour(TFT_WHITE);
            mylcd.Set_Text_Cursor(60,255);
            mylcd.print("x: ");
            mylcd.print(points[m_cur_item-1].x,DEC);
            mylcd.Set_Text_Cursor(60,275);
            mylcd.print("y: ");
            mylcd.print(points[m_cur_item-1].y,DEC);
            mylcd.Set_Text_Cursor(60,295);
            mylcd.print("w: ");
            mylcd.print(points[m_cur_point-1].w,DEC);
        }
    }

    // left grid only if full or the min is being edited
    // middle grid and diagonal only if full redraw
    // right grid only if full or the min is being edited

    if (full_redraw ||
        select_changed ||
        (redraw_curve && !m_cur_point))
    {
        mylcd.Set_Draw_color(TFT_WHITE);
        mylcd.Draw_Line(X_OFFSET, Y_OFFSET,             X_OFFSET,           Y_OFFSET+CHART_MAX);
        // ticks
        for (int i=0; i<16; i++)
        {
            int y = Y_OFFSET + i * 16;
            mylcd.Draw_Line(X_OFFSET, y, X_OFFSET+9, y);
        }
    }
    if (full_redraw ||
        select_changed ||
        redraw_curve)
    {
        mylcd.Set_Draw_color(TFT_WHITE);
        mylcd.Draw_Line(X_OFFSET, Y_OFFSET+CHART_MAX,   X_OFFSET+CHART_MAX, Y_OFFSET+CHART_MAX);  // bottom
        mylcd.Draw_Line(X_OFFSET, Y_OFFSET+CHART_MAX,   X_OFFSET+CHART_MAX, Y_OFFSET);            // diagonal
    }

    if (full_redraw ||
        select_changed ||
        (redraw_curve && m_cur_point == m_num_points-1))
    {
        mylcd.Set_Draw_color(TFT_WHITE);
        mylcd.Draw_Line(X_OFFSET+CHART_MAX, Y_OFFSET,   X_OFFSET+CHART_MAX,   Y_OFFSET+CHART_MAX);
        // ticks
        for (int i=0; i<16; i++)
        {
            int y = Y_OFFSET + i * 16;
            mylcd.Draw_Line(X_OFFSET+CHART_MAX-9, y, X_OFFSET+CHART_MAX, y);
        }
    }

    if (full_redraw ||
        select_changed ||
        redraw_curve)
    {
        // erase old points if they moved

        for (int i=0; i<m_num_points; i++)
        {
            prefCurvePoint_t *cur0 = &points[i];
            prefCurvePoint_t *prev0 = &prev_points[i];
            if (prev0->x != 255 &&
                prev0->y != 255 && (
                prev0->x != cur0->x ||
                prev0->y != cur0->y))
            {
                mylcd.Set_Draw_color(TFT_BLACK);
                mylcd.Fill_Circle(
                    X_OFFSET + 2*prev0->x,
                    Y_OFFSET + (CHART_MAX-2*prev0->y),
                    6);
            }
        }

        // lines between points

        for (int i=0; i<m_num_points-1; i++)
        {
            prefCurvePoint_t *cur0 = &points[i];
            prefCurvePoint_t *cur1 = &points[i+1];
            prefCurvePoint_t *prev0 = &prev_points[i];
            prefCurvePoint_t *prev1 = &prev_points[i+1];

            // don't draw lines that haven't moved

            if (prev0->x != cur0->x  ||
                prev0->y != cur0->y  ||
                prev1->x != cur1->x  ||
                prev1->y != cur1->y)
            {
                // if previous line was valid, redraw it in black

                if (prev0->x != 255 &&
                    prev0->y != 255 &&
                    prev1->x != 255 &&
                    prev1->y != 255)
                {
                    mylcd.Set_Draw_color(TFT_BLACK);
                    mylcd.Draw_Line(
                        X_OFFSET + 2*prev0->x ,
                        Y_OFFSET + CHART_MAX - 2*prev0->y,
                        X_OFFSET + 2*prev1->x ,
                        Y_OFFSET + CHART_MAX - 2*prev1->y);
                }

                mylcd.Set_Draw_color(TFT_YELLOW);
                mylcd.Draw_Line(
                    X_OFFSET + 2*cur0->x ,
                    Y_OFFSET + CHART_MAX - 2*cur0->y,
                    X_OFFSET + 2*cur1->x ,
                    Y_OFFSET + CHART_MAX - 2*cur1->y);

            }   // line moved
        }   // for each line


        // new points

        for (int i=0; i<m_num_points; i++)
        {
            prefCurvePoint_t *cur0 = &points[i];
            // prefCurvePoint_t *prev0 = &prev_points[i];
            // if (prev0->x == 255 ||
            //     prev0->y == 255 || (
            //     prev0->x != cur0->x ||
            //     prev0->y != cur0->y))
            {
                int fill_color =
                    i == m_cur_point ? TFT_RED :
                    i == m_cur_item -1 ? TFT_GREEN : TFT_YELLOW;

                mylcd.Set_Draw_color(fill_color);
                mylcd.Fill_Circle(
                    X_OFFSET + 2*cur0->x,
                    Y_OFFSET + (CHART_MAX-2*cur0->y),
                    6);
            }
        }
    }

    // finished

}   // updateUI()
