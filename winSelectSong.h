#ifndef _winSelectSong_h_
#define _winSelectSong_h_

#include "expSystem.h"


class winSelectSong : public expWindow
{
    public:

        winSelectSong(const char *sel_name);
        virtual ~winSelectSong() {}

        static char selected_name[80];

    private:

        void init();

        virtual const char *name()          { return "Select Song"; }
        virtual const char *short_name()    { return "Select Song"; }

        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);

        // implementation

        bool draw_needed;

        int num_songs;
        int top_song;
        int selected_song;
        int last_selected_song;



};


#endif      // !_winSelectSong_h_