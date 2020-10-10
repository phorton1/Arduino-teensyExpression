#ifndef __songMachine_h__
#define __songMachine_h__

#define song_error(f,...)        { error_fxn(f,__VA_ARGS__); songMachine::error_msg(f,__VA_ARGS__); }


#define SONG_STATE_EMPTY   0
#define SONG_STATE_RUNNING 1
#define SONG_STATE_PAUSED  2


class songMachine
{
    public:

        static void clear();
            // clear the currently running program

        static bool load(const char *name);
            // load the test song, parse it, and prepare machine to run
            // will eventually have a UI for filenames and load any given song file

        static int getMachineState();
        static void setMachineState(int state);

        static void notifyPress(int button_num);
            // one based buttons
        static void notifyLoop();
            // notify the songMachine that a loop has taken place

        static void task();
            // called approx 30 times per second from rigNew::updateUI()

        static void uc(char *buf);
        static void error_msg(const char *format, ...);


    private:

        static void dumpCode();

};




#endif  // !__songMachine_h__