#ifndef _pedals_h_
#define _pedals_h_

#include "defines.h"



typedef struct
{
    int x;          // 0..127
    int y;
    int weight;

} pedalPoint_t;


class expressionPedal
{
    public:

        int getNum()                    { return m_num; }
        const char *getName()           { return m_name; }

        int getValue()                  { return m_value; }
        bool displayValueChanged()      { return m_last_value != m_value; }
        void clearDisplayValueChanged() { m_last_value = m_value; }

        int getCalibMin()               { return m_calib_min; }
        int getCalibMax()               { return m_calib_max; }
        int getCurveType()              { return m_curve_type; }
        int getValueMin()               { return m_value_min; }
        int getValueMax()               { return m_value_max; }

        void setCalibMin(int i)         { if (i>=m_calib_max) i=m_calib_max-1;  if (i<0) i=0; m_calib_min = i; }
        void setCalibMax(int i)         { if (i<=m_calib_min) i=m_calib_min+1;  if (i>1023) i=1023; m_calib_max = i; }
        void setCurveType(int i)        { if (i<0) i=0; if (i>2) i=2;  m_curve_type = i; }
        void setValueMin(int i)         { if (i>=m_value_max) i=m_value_max-1;  if (i<0) i=0; m_value_min = i; }
        void setValueMax(int i)         { if (i<=m_value_min) i=m_value_min+1;  if (i>127) i=127; m_value_max = i; }

        pedalPoint_t *getPoint(int i)   { return &m_points[i]; }

        // midi

        int getCCChannel()              { return m_cc_channel; }
        int getCCNum()                  { return m_cc_num; }

    protected:

        friend class pedalManager;

        expressionPedal() {}

        void init(
            int num,
            int pin,
            const char *name,
            int cc_channel,
            int cc_num,
            int value_max=127);

        void poll();

    private:

        // construction paramaters

        int     m_num;
        int     m_pin;          // defined in pedals.cpp
        int     m_pedal_num;    // they know this too ...
        int     m_cc_channel;
        int     m_cc_num;

        const char *m_name;

        // implmented configuration variables

        int     m_calib_min;        // defaults in pedals.cpp
        int     m_calib_max;        // 0..1023
        int     m_value_min;        // 0..127
        int     m_value_max;

        // runtime working variables

        int      m_raw_value;       // 0..1023
        int      m_direction;       // -1,0,1
        unsigned m_settle_time;
        int      m_value;           // 0..127
        int      m_last_value;      // display helper

        // for editing curves (future development)

        int     m_curve_type;     // 0..2 (also defines number of points)
        int     m_cur_point;
            // min=0, max=m_curve_type+1
            // in between are m_curve_type points that are called
            // "mid", or "left" and "right"

        pedalPoint_t m_points[MAX_PEDAL_CURVE_POINTS];

};



class pedalManager
{
    public:

        pedalManager() {}

        void init();
            // called at runtime to setup pedals from EEPROM

        void task();
            // polls pedals, may call expSystem::onPedalEvent()

        expressionPedal *getPedal(int i)  { return &m_pedals[i]; }


    private:

        expressionPedal m_pedals[NUM_PEDALS];

};


extern pedalManager thePedals;



#endif      // !_pedals_h_