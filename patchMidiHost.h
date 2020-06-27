#ifndef _patchMidiHost_h_
#define _patchMidiHost_h_

#include "expSystem.h"


class patchMidiHost : public expWindow
{
    public:

        patchMidiHost();

    private:

        virtual const char *name()          { return "FTP Tester"; }
        virtual const char *short_name()    { return "FTP Test"; }

        virtual void end();
        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);

        // implementation

        bool draw_needed;
        int last_vel[6];
        int last_velocity[6];
        int last_sens[6];

        void init();
        void vel2ToInts(int *vel2, int *velocity);
        void drawBox(int string, int box32, int vel16);

        uint8_t dbg_bank_num;
        uint8_t dbg_patch_num;
        uint8_t dbg_command;
        uint8_t dbg_param;

        void myIncDec(int inc, uint8_t *val);
};


#endif      // !_patchMidiHost_h_