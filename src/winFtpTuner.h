//-------------------------------
// winFtpTuner.h
//-------------------------------

#pragma once

#include "expSystem.h"



class winFtpTuner : public expWindow
{
    public:

        winFtpTuner(bool swap_modal = false);

    private:

        virtual const char *name()          { return "FTP Tuner"; }
        virtual const char *short_name()    { return "FTP Tuner"; }
        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);

        // implementation

        bool m_swap_modal;
        bool draw_needed;
        int last_string_pressed[6];
        int last_tuner_note;
        int last_tuner_value;

        void init();
        void drawCircle(int string, int fret, bool pressed);
        void fretsToInts(int *ints);
        void drawTunerPointer(int tuner_x, int color);
};

