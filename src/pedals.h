//--------------------------------
// pedals.h
//--------------------------------

#pragma once

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

        int getCCChannel()              { return m_cc_channel; }
        int getCCNum()                  { return m_cc_num; }

        int getValue()                  { return m_value; }
        int getRawValue()               { return m_raw_value; }
            // the actual values

        int getDisplayValue()           { return m_display_value; }
        void setDisplayValue(int i)     { m_display_value = i; }
        bool displayValueChanged()      { return m_last_display_value != m_display_value; }
        void clearDisplayValueChanged() { m_last_display_value = m_display_value; }
            // completely separate display values

        void invalidate()               { m_valid = false; }

        float getRawValuePct();
        int getRawValueScaled();


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
        int      m_raw_value;               // 0..1023
        int      m_direction;               // -1,0,1
        unsigned m_settle_time;
        int      m_value;                    // 0..127

        int      m_display_value;           // display helper
        int      m_last_display_value;      // display helper
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

        void pedalEvent(int num, int value);


    private:

        friend class expressionPedal;

        expressionPedal m_pedals[NUM_PEDALS];

};


extern pedalManager thePedals;

