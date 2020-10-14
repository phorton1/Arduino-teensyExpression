#ifndef __rigBass_h__
#define __rigBass_h__

// an abstract base class representing a rig that work
// with the songMachine

#include "expSystem.h"


class rigBase : public expWindow
{
    public:

        rigBase() : expWindow(WIN_FLAG_SHOW_PEDALS) {}

        virtual int findPatchByName(const char *patch_name) = 0;
        virtual void setPatchNumber(int patch_number) = 0;

        virtual void clearGuitarEffects(bool display_only) = 0;
        virtual void setGuitarEffect(int effect_num, bool on) = 0;

        virtual void clearLooper(bool display_only) = 0;
        virtual void selectTrack(int num) = 0;
        virtual void stopLooper() = 0;
        virtual void stopLooperImmediate() = 0;
        virtual void loopImmediate() = 0;
        virtual void toggleDubMode() = 0;
        virtual void setStartMark() = 0;
        virtual void setClipMute(int layer_num, bool on) = 0;
        virtual void setClipVolume(int layer_num, int val) = 0;

        virtual bool songUIAvailable() = 0;
            // is the song rectangle available for updating?
            // not so in newRig quickMode, for example
};





#endif // !__rigBass_h__