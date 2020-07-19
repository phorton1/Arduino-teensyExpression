#ifndef __winConfigPedal_h__
#define __winConfigPedal_h__

#include "prefs.h"
#include "expSystem.h"

// There are 3 complete curves of four points each in the prefs
// That get loaded into memory and stored as a whole ?!?

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t unused;
}   prefCurvePoint_t;




class winConfigPedal : public expWindow
{
    public:

        winConfigPedal(int i);

    private:

        const char **m_curve_names;

        int m_pedal_num;
        bool m_redraw_curve;

        bool m_cur_auto;      // getPref8(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_AUTO_OFFSET)  0..1
        int m_cur_curve;      // getPref8(PREF_PEDAL(m_pedal_num) + PREF_PEDAL_CURVE_TYPE_OFFSET)  0..2
        int m_num_points;     // m_cur_curve + 2
        int m_num_items;      // m_num_points + 1
        int m_cur_item;       // 0 .. m_num_items-1
        int m_cur_point;      // 0 .. m_num_points-1, -1 when not editing

        int m_display_item;
        int m_display_curve;
        int m_display_raw_value;
        int m_display_pedal_x;
        int m_display_pedal_value;
        bool m_in_calibrate;

        prefCurvePoint_t m_points[MAX_PEDAL_CURVE_POINTS];
        prefCurvePoint_t m_prev_points[MAX_CURVE_POINTS];

        virtual const char *name();
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();

        void getPrefPedalPoints();
        void clearPrevPoints();
        void setEditPoints();
        void showSelectedItem(int item, int selected);

        prefCurvePoint_t *getCurvePoints() { return &m_points[m_cur_curve * MAX_CURVE_POINTS]; }

};


#endif  // !__winConfigPedal_h__
