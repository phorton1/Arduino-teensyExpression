#ifndef __songMachine_h__
#define __songMAchine_h__


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
            // called approx 30 times per second from patchNewRig::updateUI()

};




#endif  // !__songMAchine_h__