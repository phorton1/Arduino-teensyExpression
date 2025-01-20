//-------------------------------
// winSelectSong.h
//-------------------------------

#pragma once

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
        char *show_song_text;
        int num_text_lines;
        int top_text_line;
        int last_top_line;

        int num_songs;
        int top_song;
        int selected_song;
        int last_selected_song;
};


