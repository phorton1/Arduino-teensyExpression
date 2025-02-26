//--------------------------------
// songMachine.h
//--------------------------------

#pragma once

#include "Arduino.h"
#include "pedals.h"


#define song_error(f,...)        { error_fxn(f,__VA_ARGS__); songMachine::error_msg(f,__VA_ARGS__); }


#define SONG_STATE_EMPTY            0x0000
#define SONG_STATE_RUNNING          0x0001
#define SONG_STATE_PAUSED           0x0002
#define SONG_STATE_WAITING_BUTTON   0x0010
#define SONG_STATE_WAITING_LOOP     0x0020
#define SONG_STATE_FINISHED         0x1000
#define SONG_STATE_ERROR            0x8000

#define NUM_SONG_BUTTONS            4

#define MAX_CALL_STACK              10


typedef struct
{
    int from_val;               // where did the volume change start at
    int to_val;                 // what is the target value
    int last_val;               // the last value sent
    int delay_tenths;           // tenths of a second for fade
    uint32_t event_time;        // the start time
    uint32_t last_cmd_time;     // when did we send the last command
} pedal_volume_t;


class songMachine
{
    public:

        songMachine();
        ~songMachine()  {}

        bool load(const char *name);
            // sets the machine state to EMPTY to begin with,
            // you do not need to call setMachineState(EMPTY)
            // if it fails.

        int getMachineState()   { return m_state; }
        void setMachineState(int state);

        void resetDisplay()  { m_redraw = 1; }
        void updateUI();
            // only called with focus

        void notifyPress(int button_num);
            // one based buttons
        void notifyLoop();
            // notify the songMachine that a loop has taken place

        // utilities

        static void uc(char *buf);
        static void error_msg(const char *format, ...);


    private:

        void init()
        {
            m_redraw = 1;
            m_state = 0;
            m_last_state = -1;
            m_song_name = 0;
            m_delay = 0;
            m_delay_time = 0;
            m_num_calls = 0;

            button_flash_time = 0;
            button_flash_state = 0;

            for (int i=0; i<NUM_SONG_BUTTONS; i++)
            {
                m_button_color[i] = 0;
                m_last_button_color[i] = -1;
                m_button_flash[i] = 0;
            }

            m_code_ptr = 0;
            for (int i=0; i<2; i++)
            {
                m_show_msg[i] = 0;
                m_last_show_msg[i] = 0;
                m_show_color[i] = 0;
                m_last_show_color[i] = 0;
            }
            for (int i=0; i<NUM_PEDALS; i++)
            {
                pedal_volumes[i].from_val = 0;
                pedal_volumes[i].to_val = 0;
                pedal_volumes[i].last_val = 0;
                pedal_volumes[i].delay_tenths = 0;
                pedal_volumes[i].event_time = 0;
                pedal_volumes[i].last_cmd_time = 0;
            }
        }

        // ui variables

        bool m_redraw;
        elapsedMillis button_flash_time;
        bool button_flash_state;

        int m_state;
        int m_last_state;
        int m_button_color[NUM_SONG_BUTTONS];
        bool m_button_flash[NUM_SONG_BUTTONS];
        int m_last_button_color[NUM_SONG_BUTTONS];

        const char *m_song_name;

        const char *m_show_msg[2];
        const char *m_last_show_msg[2];
        int m_show_color[2];
        int m_last_show_color[2];

        // machine variables and methods

        int m_code_ptr;
        int m_delay;
        elapsedMillis m_delay_time;
        int m_num_calls;
        int m_call_stack[MAX_CALL_STACK];

        pedal_volume_t pedal_volumes[NUM_PEDALS];

        // private methods

        void runMachine();
        void doSongOp(int op);
        int tokenToLEDColor(int ttype);
        int tokenToTFTColor(int ttype);
        int advance(int ptr);

        // debugging

        void dumpCode();

};



extern songMachine song_machine;

