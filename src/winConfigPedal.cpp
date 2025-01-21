//--------------------------------------------------------------
// winConfigPedal.cpp
//--------------------------------------------------------------
// Navigation:
//
//	 The meaning of the upper right CANCEL/DONE buttons
//	 does not change while configuring pedals.
//
//   The upper left BLUE button cycles through the pedals.
//
//   The keypad is modal, depending on if we are editing
//   the graph, or in the outer menu mode.
//
//   MENU MODE:
//			The RIGHT arrow is black, as it does nothing
//			The LEFT arrow returns to the Pedal List
//			UP and DOWN move through the options.
//          CENTER green toggles a value or enters EDIT_MODE
//
//	 EDIT MODE
//			 LEFT and RIGHT choose the points
//			 UP and DOWN modify the values


#include <myDebug.h>
#include "winConfigPedal.h"
#include "configSystem.h"
#include "myTFT.h"
#include "myLeds.h"
#include "pedals.h"
#include "buttons.h"


#define dbg_cpedal	0
//----------------------------------
// Pedals
//----------------------------------

#define NEXT_PEDAL_BUTTON  0

#define KEYPAD_UP      12
#define KEYPAD_DOWN    22
#define KEYPAD_LEFT    16
#define KEYPAD_RIGHT   18
#define KEYPAD_SELECT  17

#define ITEM_MODE         0
#define ITEM_CALIBRATE    1
#define ITEM_CURVE_TYPE   2
#define NUM_FIXED_ITEMS   3

#define CALIB_SAFETY_MARGIN   10

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
    display(dbg_cpedal,"winConfigPedal(%d) pedal=%d",warm,m_pedal_num);
	expWindow::begin(warm);

    m_redraw_curve = 1;
    m_display_curve = -1;
    m_display_item = -1;
    m_display_raw_value = -1;
    m_display_pedal_x = -1;
    m_display_pedal_value = -1;
    m_in_calibrate = 0;

    m_cur_mode = getPref8(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_MODE_OFFSET);
    m_cur_curve = getPref8(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_CURVE_TYPE_OFFSET);
    m_curve_names = getPrefStrings(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_CURVE_TYPE_OFFSET);
    m_mode_names = getPrefStrings(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_MODE_OFFSET);
    getPrefPedalPoints();
    setEditPoints();

    m_cur_item   = NUM_FIXED_ITEMS + m_num_points-1;        // start on MAX point
    m_cur_point  = -1;                  					// -1 when not selected for editing

	// the CANCEL/DONE buttons are inherited from,
	// and pass button presses through to the config system

    theButtons.setButtonType(BUTTON_EXIT_DONE,       BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, config_system.m_dirty?LED_PURPLE:LED_CYAN);
    theButtons.setButtonType(BUTTON_EXIT_CANCEL,     BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, config_system.m_dirty?LED_ORANGE:LED_YELLOW);

	theButtons.setButtonType(KEYPAD_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_BLACK);	// starts out black
	theButtons.setButtonType(KEYPAD_SELECT,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	theButtons.setButtonType(NEXT_PEDAL_BUTTON,	BUTTON_TYPE_CLICK, LED_BLUE);

	showLEDs();
}


void winConfigPedal::setEditPoints()
    // called when the curve changes
{
    m_num_points = m_cur_curve + 2;     // number of points for drawing
    m_num_items  = m_num_points + NUM_FIXED_ITEMS;    // number of items to scroll thru
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

    display(dbg_cpedal+1,"PREF_PEDAL(%d)=%d",m_pedal_num,PREF_PEDAL(m_pedal_num));
    display(dbg_cpedal+1,"sizeof(m_points)=%d,  CURVE_BYTES_PER_PEDAL=%d ",sizeof(m_points),CURVE_BYTES_PER_PEDAL);

    uint8_t *p = (uint8_t *) m_points;
    for (int i=0; i<CURVE_BYTES_PER_PEDAL; i++)
    {
        *p++ = getPref8(PREF_PEDAL(m_pedal_num)+PREF_PEDAL_POINTS_OFFSET+i);
    }
    display_bytes(dbg_cpedal+2,"buf",(uint8_t *) m_points, 48);

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

        if (m_in_calibrate)
        {
            // empty case if calibrating
        }
        else if (m_cur_point >= 0)
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
                thePedals.getPedal(m_pedal_num)->invalidate();
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
        if (m_cur_point >= 0)
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
                thePedals.getPedal(m_pedal_num)->invalidate();
            }
        }
        else if (num == KEYPAD_LEFT)
        {
			config_system.checkDirty();
			m_in_calibrate = 0;
			endModal(237);	// return to config system
        }
	}
    else if (num == KEYPAD_SELECT)
    {
        if (m_cur_point >= 0)
        {
            m_cur_point = -1;
            m_display_item = -1;
			theButtons.setButtonColor(KEYPAD_RIGHT,	LED_BLACK);		// black while not editing
		}
        else if (m_cur_item >= NUM_FIXED_ITEMS)
        {
            clearPrevPoints();
            m_cur_point = m_cur_item - NUM_FIXED_ITEMS;
            m_display_item = -1;
			theButtons.setButtonColor(KEYPAD_RIGHT,	LED_BLUE);			// blue while editing
        }
        else if (m_cur_item == ITEM_MODE)
        {
            m_cur_mode = (m_cur_mode + 1) % 2;
            setPrefPedalMode(m_pedal_num,m_cur_mode);
            thePedals.getPedal(m_pedal_num)->invalidate();
            setPrefPedalCurve(m_pedal_num, m_cur_curve);
            setEditPoints();
            m_display_curve = -1;
        }
        else if (m_cur_item == ITEM_CURVE_TYPE)
        {
            m_cur_curve = (m_cur_curve + 1) % MAX_PEDAL_CURVES;
            thePedals.getPedal(m_pedal_num)->invalidate();
            setPrefPedalCurve(m_pedal_num, m_cur_curve);
            setEditPoints();
        }
        else if (m_cur_item == ITEM_CALIBRATE)
        {
			if (m_in_calibrate)
			{
				m_in_calibrate = 0;
			}
			else
			{
				m_in_calibrate = 1;
				setPrefPedalCalibMin(m_pedal_num,1023);
				setPrefPedalCalibMax(m_pedal_num,0);
			}
            m_display_curve = -1;
            clearPrevPoints();
        }
		config_system.checkDirty();
    }

	// next pedal
	
	else if (num == NEXT_PEDAL_BUTTON)
	{
		config_system.checkDirty();
		m_in_calibrate = 0;
		endModal(237);
			// ending the modal causes swapModal short return
			// which seems to fix bugs
		int next_num = (m_pedal_num+1) % NUM_PEDALS;
		theSystem.swapModal(new winConfigPedal(next_num),0);
	}

    // endModal && pass through to config_system

	else if (num == BUTTON_EXIT_DONE ||
			 num == BUTTON_EXIT_CANCEL)
	{
		config_system.checkDirty();
		endModal(237);
		config_system.onButtonEvent(row, col, event);
    }

}




//---------------------------------------------------
// DRAW
//---------------------------------------------------


#define TEXT_MARGIN  10
#define TEXT_SPACING 40

#define X_OFFSET    140
#define Y_OFFSET    50
#define CHART_MAX   255

#define RIGHT_COL          410
#define RIGHT_WIDTH        60
#define RIGHT_LINE_HEIGHT  20


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
    if (item == ITEM_MODE)
    {
        name = m_mode_names[m_cur_mode];
    }
    else if (item == ITEM_CALIBRATE)
    {
        name = "calib";
    }
    else if (item == ITEM_CURVE_TYPE)
    {
        name = m_curve_names[m_cur_curve];
    }
    else
    {
        name = getPointName(m_cur_curve,item-NUM_FIXED_ITEMS);
    }

    int x = TEXT_MARGIN;
    int y = Y_OFFSET + item * TEXT_SPACING;
    int w = X_OFFSET - 2 * TEXT_MARGIN;
    int h = 30;

    int fc = TFT_WHITE;
    int bc = selected ? TFT_BLUE : TFT_BLACK;

    if (selected && (
        (m_in_calibrate && item==ITEM_CALIBRATE) ||
        (item>=NUM_FIXED_ITEMS && item-NUM_FIXED_ITEMS == m_cur_point)))
    {
        fc = TFT_BLUE;
        bc = TFT_YELLOW;
    }

    mylcd.setFont(Arial_16_Bold);
    mylcd.fillRect(x,y,w,h,bc);
    mylcd.printfJustified(
        x,	// x+5,
        y+5,	// y+7,
        w,	// w-10,
        h-3,	// h-6,
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
    expressionPedal *pedal = thePedals.getPedal(m_pedal_num);
    int raw_value = pedal->getRawValue();
    int pedal_value = pedal->getValue();
    int pedal_x = pedal->getRawValueScaled();
    int last_pedal_x = m_display_pedal_x;
    int last_raw_value = m_display_raw_value;
    int last_pedal_value = m_display_pedal_value;

    prefCurvePoint_t points[MAX_CURVE_POINTS];
    prefCurvePoint_t prev_points[MAX_CURVE_POINTS];
    memcpy(points,getCurvePoints(),PREF_BYTES_PER_CURVE);
    memcpy(prev_points,m_prev_points,PREF_BYTES_PER_CURVE);

    // and set "previous" variables as soon as possible

    m_redraw_curve = 0;
    m_display_item = m_cur_item;
    m_display_curve = m_cur_curve;
    m_display_raw_value = raw_value;
    m_display_pedal_value = pedal_value;
    m_display_pedal_x = pedal_x;

    memcpy(m_prev_points,points,PREF_BYTES_PER_CURVE);

    __enable_irq();

    // clear

    if (full_redraw)
    {
        mylcd.fillRect(0,Y_OFFSET,480,320-Y_OFFSET,0);

        mylcd.setFont(Arial_12_Bold);
        int text_y = Y_OFFSET + 3 * RIGHT_LINE_HEIGHT;
        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            false,
            "CALIB");
        text_y += 4*RIGHT_LINE_HEIGHT;
        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            false,
            "VALUE");
    }


    bool calib_changed = m_in_calibrate && raw_value != last_raw_value;
    if (full_redraw || calib_changed)
    {
       int text_y = Y_OFFSET + 4*RIGHT_LINE_HEIGHT;
        if (!full_redraw)
            mylcd.fillRect(RIGHT_COL,text_y,RIGHT_WIDTH,2*RIGHT_LINE_HEIGHT,0);
        int color = m_in_calibrate ? TFT_GREEN : TFT_YELLOW;

        if (calib_changed)
        {
            if (raw_value < getPrefPedalCalibMin(m_pedal_num))
                setPrefPedalCalibMin(m_pedal_num,raw_value + CALIB_SAFETY_MARGIN);
            if (raw_value > getPrefPedalCalibMax(m_pedal_num))
                setPrefPedalCalibMax(m_pedal_num,raw_value - CALIB_SAFETY_MARGIN);
        }

        mylcd.setFont(m_in_calibrate ? Arial_12_Bold : Arial_12);
        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            color,
            0,
            false,
            "%d",
            getPrefPedalCalibMin(m_pedal_num));
        text_y += RIGHT_LINE_HEIGHT;

        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            color,
            0,
            false,
            "%d",
            getPrefPedalCalibMax(m_pedal_num));
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

    if (full_redraw || select_changed || redraw_curve)
    {
        int text_y = Y_OFFSET;
        mylcd.fillRect(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT*2,0);
        if (m_cur_item >= NUM_FIXED_ITEMS)
        {
            int cur_point = m_cur_item-NUM_FIXED_ITEMS;

            mylcd.setFont(Arial_12);
            mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
                LCD_JUST_CENTER,
                TFT_YELLOW,
                0,
                false,
                "x=%d",
                points[cur_point].x);
            text_y += RIGHT_LINE_HEIGHT;

            mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
                LCD_JUST_CENTER,
                TFT_YELLOW,
                0,
                false,
                "y=%d",
                points[cur_point].y);

            // mylcd.print(points[cur_point].w,DEC);
        }
    }

    // REDRAW CHART

    if (full_redraw ||
        select_changed ||
        redraw_curve ||
        raw_value != last_raw_value ||
        last_pedal_value != pedal_value)
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
                mylcd.setDrawColor(TFT_BLACK);
                mylcd.fillCircle(
                    X_OFFSET + 2*prev0->x,
                    Y_OFFSET + (CHART_MAX-2*prev0->y),
                    6);
            }
        }

        if (last_pedal_x != -1)
        {
            mylcd.setDrawColor(TFT_BLACK);
            mylcd.fillCircle(
                X_OFFSET + 2*last_pedal_x,
                Y_OFFSET + (CHART_MAX-2*last_pedal_value),
                6);
        }


        // grid left w/ticks

        mylcd.setDrawColor(TFT_WHITE);
        mylcd.drawLine(X_OFFSET, Y_OFFSET,             X_OFFSET,           Y_OFFSET+CHART_MAX);
        for (int i=0; i<16; i++)
        {
            int y = Y_OFFSET + i * 16;
            mylcd.drawLine(X_OFFSET, y, X_OFFSET+9, y);
        }

        // grid center

        mylcd.drawLine(X_OFFSET, Y_OFFSET+CHART_MAX,   X_OFFSET+CHART_MAX, Y_OFFSET+CHART_MAX);  // bottom
        mylcd.drawLine(X_OFFSET, Y_OFFSET+CHART_MAX,   X_OFFSET+CHART_MAX, Y_OFFSET);            // diagonal

        // grid right w/ticks

        mylcd.drawLine(X_OFFSET+CHART_MAX, Y_OFFSET,   X_OFFSET+CHART_MAX,   Y_OFFSET+CHART_MAX);
        for (int i=0; i<16; i++)
        {
            int y = Y_OFFSET + i * 16;
            mylcd.drawLine(X_OFFSET+CHART_MAX-9, y, X_OFFSET+CHART_MAX, y);
        }

        //----------------------------------
        // lines
        //----------------------------------
        // line from 0 to MIN

        // if (prev_points[0].x != points[0].x ||
        //     prev_points[0].y != points[0].y)
        // {
            // erase old one if it moved

            if (prev_points[0].x &&
                prev_points[0].x != 255)
            {
                mylcd.setDrawColor(TFT_BLACK);
                mylcd.drawLine(
                    X_OFFSET,
                    Y_OFFSET + CHART_MAX - 2*prev_points[0].y,
                    X_OFFSET + 2*prev_points[0].x ,
                    Y_OFFSET + CHART_MAX - 2*prev_points[0].y);
            }

            // draw new one if off zero

            if (points[0].x)
            {
                mylcd.setDrawColor(TFT_YELLOW);
                mylcd.drawLine(
                    X_OFFSET,
                    Y_OFFSET + CHART_MAX - 2*points[0].y,
                    X_OFFSET + 2*points[0].x ,
                    Y_OFFSET + CHART_MAX - 2*points[0].y);
            }
        // }

        // interior lines

        for (int i=0; i<m_num_points-1; i++)
        {
            prefCurvePoint_t *cur0 = &points[i];
            prefCurvePoint_t *cur1 = &points[i+1];
            prefCurvePoint_t *prev0 = &prev_points[i];
            prefCurvePoint_t *prev1 = &prev_points[i+1];

            // don't draw lines that haven't moved

            // if (prev0->x != cur0->x  ||
            //     prev0->y != cur0->y  ||
            //     prev1->x != cur1->x  ||
            //     prev1->y != cur1->y)
            {
                // if previous line was valid, redraw it in black

                if (prev0->x != 255 &&
                    prev0->y != 255 &&
                    prev1->x != 255 &&
                    prev1->y != 255)
                {
                    mylcd.setDrawColor(TFT_BLACK);
                    mylcd.drawLine(
                        X_OFFSET + 2*prev0->x ,
                        Y_OFFSET + CHART_MAX - 2*prev0->y,
                        X_OFFSET + 2*prev1->x ,
                        Y_OFFSET + CHART_MAX - 2*prev1->y);
                }

                mylcd.setDrawColor(TFT_YELLOW);
                mylcd.drawLine(
                    X_OFFSET + 2*cur0->x ,
                    Y_OFFSET + CHART_MAX - 2*cur0->y,
                    X_OFFSET + 2*cur1->x ,
                    Y_OFFSET + CHART_MAX - 2*cur1->y);

            }   // line moved
        }   // for each line


        // line from MAX to 127

        int max_point = m_num_points-1;

        // if (prev_points[max_point].x != points[max_point].x ||
        //     prev_points[max_point].y != points[max_point].y)
        // {
            // erase old one if it moved

            if (prev_points[max_point].x < 127)
            {
                mylcd.setDrawColor(TFT_BLACK);
                mylcd.drawLine(
                    X_OFFSET + 2*prev_points[max_point].x,
                    Y_OFFSET + CHART_MAX - 2*prev_points[max_point].y,
                    X_OFFSET + CHART_MAX,
                    Y_OFFSET + CHART_MAX - 2*prev_points[max_point].y);
            }

            // draw new one if off zero

            if (points[max_point].x < 127)
            {
                mylcd.setDrawColor(TFT_YELLOW);
                mylcd.drawLine(
                    X_OFFSET + 2*points[max_point].x,
                    Y_OFFSET + CHART_MAX - 2*points[max_point].y,
                    X_OFFSET + CHART_MAX,
                    Y_OFFSET + CHART_MAX - 2*points[max_point].y);
            }
        // }


        //---------------------
        // new points
        //---------------------

        for (int i=0; i<m_num_points; i++)
        {
            prefCurvePoint_t *cur0 = &points[i];
            int fill_color =
                i == m_cur_point ? TFT_RED :
                i == m_cur_item-NUM_FIXED_ITEMS ? TFT_GREEN : TFT_YELLOW;

            mylcd.setDrawColor(fill_color);
            mylcd.fillCircle(
                X_OFFSET + 2*cur0->x,
                Y_OFFSET + (CHART_MAX-2*cur0->y),
                6);
        }

        mylcd.setDrawColor(TFT_CYAN);
        mylcd.fillCircle(
            X_OFFSET + 2*pedal_x,
            Y_OFFSET + (CHART_MAX-2*pedal_value),
            6);
    }

    // current value

    if (full_redraw ||
        select_changed ||
        redraw_curve ||
        raw_value != last_raw_value ||
        last_pedal_value != pedal_value)
    {
        int text_y = Y_OFFSET + 8 * RIGHT_LINE_HEIGHT;
        mylcd.fillRect(RIGHT_COL,text_y,RIGHT_WIDTH,3*RIGHT_LINE_HEIGHT,0);

        mylcd.setFont(Arial_12);
        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            false,
            "%d",
            raw_value);
        text_y += RIGHT_LINE_HEIGHT;

        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            false,
            "%0.2f%%",
            pedal->getRawValuePct());
        text_y += RIGHT_LINE_HEIGHT;

        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            false,
            "%d",
            pedal_x);
        text_y += 2*RIGHT_LINE_HEIGHT;

        mylcd.setFont(Arial_16_Bold);
        mylcd.printfJustified(RIGHT_COL,text_y,RIGHT_WIDTH,RIGHT_LINE_HEIGHT,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            0,
            true,
            "%d",
            pedal_value);
    }

    // show raw value changes as a rectangle just below the grid
    // and a line to the value

    if (full_redraw ||
        redraw_curve ||
        pedal_x != last_pedal_x)
    {
        int bar_y = Y_OFFSET + CHART_MAX + 1;
        int bar_h = 320 - bar_y + 1;

        mylcd.fillRect(X_OFFSET-1,bar_y,pedal_x*2+3,bar_h,TFT_CYAN);
        mylcd.fillRect(X_OFFSET+pedal_x*2+3,bar_y,CHART_MAX-pedal_x*2-3,bar_h,0);

    }

    // finished

}   // updateUI()
