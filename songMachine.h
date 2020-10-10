#ifndef __songMachine_h__
#define __songMachine_h__

#define song_error(f,...)        { error_fxn(f,__VA_ARGS__); songMachine::error_msg(f,__VA_ARGS__); }


#define SONG_STATE_EMPTY   0
#define SONG_STATE_RUNNING 1
#define SONG_STATE_PAUSED  2
#define NUM_SONG_BUTTONS   4

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
            for (int i=0; i<NUM_SONG_BUTTONS; i++)
            {
                m_button_color[i] = 0;
                m_last_button_color[i] = -1;
            }
        }

        int m_state;
        int m_last_state;
        int m_button_color[NUM_SONG_BUTTONS];
        int m_last_button_color[NUM_SONG_BUTTONS];
        bool m_redraw;
        const char *m_song_name;

        void dumpCode();

};



extern songMachine *theSongMachine;



#endif  // !__songMachine_h__