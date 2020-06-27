#ifndef _pedals_h_
#define _pedals_h_

#include "prefs.h"



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

        int getRawValue()               { return m_raw_value; }
        inline float getRawValuePct()
        {
            float min = getPrefPedalCalibMin(m_num);
            float max = getPrefPedalCalibMax(m_num);
            float val = m_raw_value - min;
            if (val < 0.00) val = 0.00;
            float ret_val = val / (max - min);
            if (ret_val > 1.0) ret_val = 1.0;
            return ret_val;
        }
        int getRawValueScaled()
        {
            float ret_val = getRawValuePct() * 127.00 + 0.5;
            return ret_val;
        }
        void invalidate()
        {
            m_valid = false;
        }


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
            int cc_num);

        void poll();

    private:

        // construction paramaters

        int     m_num;
        int     m_pin;          // defined in pedals.cpp
        int     m_pedal_num;    // they know this too ...
        int     m_cc_channel;
        int     m_cc_num;

        const char *m_name;

        // runtime working variables

        bool     m_valid;
        int      m_raw_value;       // 0..1023
        int      m_direction;       // -1,0,1
        unsigned m_settle_time;
        int      m_value;           // 0..127
        int      m_last_value;      // display helper
};



class pedalManager
{
    public:

        pedalManager() {}

        void init();
            // called at runtime to setup pedals from prefs

        void task();
            // polls pedals, may call expSystem::onPedalEvent()

        expressionPedal *getPedal(int i)  { return &m_pedals[i]; }


    private:

        expressionPedal m_pedals[NUM_PEDALS];

};


extern pedalManager thePedals;



#endif      // !_pedals_h_