#ifndef __songMachine_h__
#define __songMAchine_h__

#define MAX_SONG_TOKEN  80
#define MAX_SONG_CODE   4096


#define TOKEN_DISPLAY                   1    // "DISPLAY"
#define TOKEN_LOOP_VOLUME               2    // "LOOP_VOLUME"
#define TOKEN_SYNTH_VOLUME              3    // "SYNTH_VOLUME"
#define TOKEN_GUITAR_VOLUME             4    // "SYNTH_VOLUME"
#define TOKEN_GUITAR_EFFECT_NONE        5    // "GUITAR_EFFECT_NONE"
#define TOKEN_GUITAR_EFFECT_DISTORT     6    // "GUITAR_EFFECT_DISTORT"
#define TOKEN_GUITAR_EFFECT_WAH         7    // "GUITAR_EFFECT_WAH"
#define TOKEN_GUITAR_EFFECT_CHORUS      8    // "GUITAR_EFFECT_CHORUS"
#define TOKEN_GUITAR_EFFECT_ECHO        9    // "GUITAR_EFFECT_ECHO"
#define TOKEN_ON                        10   // "ON"
#define TOKEN_OFF                       11   // "OFF"
#define TOKEN_CLEAR_LOOPER              12   // "CLEAR_LOOPER"
#define TOKEN_BUTTON1                   13   // "BUTTON1:"
#define TOKEN_LOOP                      14   // "LOOP:"
#define TOKEN_LOOPER_TRACK              15   // "LOOPER_TRACK"
#define TOKEN_LOOPER_STOP               16   // "LOOPER_STOP"
#define TOKEN_DUB_MODE                  17   // "DUB_MODE"
#define TOKEN_SYNTH_PATCH               18   // "SYNTH_PATCH"
#define TOKEN_LOOPER_CLIP               19   // "LOOPER_CLIP"
#define TOKEN_MUTE                      20   // "MUTE"
#define TOKEN_UNMUTE                    21   // "UNMUTE"
#define TOKEN_LOOPER_SET_START_MARK     22   // "LOOPER_SET_START_MARK"
#define TOKEN_STRING                    24   // "string"
#define TOKEN_NUMBER                    25   // number
#define TOKEN_EOF                       26   // end of file


class songMachine
{
    public:

        static void clear();
            // clear the currently running program

        static bool load();
            // load the test song, parse it, and prepare machine to run
            // will eventually have a UI for filenames and load any given song file

        static void notifyPress();
            // for now we are overusing the songMachine button
            // long_click = load and start, or clear

        static void notifyLoop();
            // notify the songMachine that a loop has taken place

        static void task();
            // called approx 30 times per second from rigNew::updateUI()

};




#endif  // !__songMAchine_h__