#ifndef __expDialogs_h__
#define __expDialogs_h__

#include "expSystem.h"


class yesNoDialog : public expWindow
{
    public:
        
        yesNoDialog(uint32_t id, const char *name, const char *format, ...);
        virtual uint32_t getId()    { return m_id; }

    private:
        
        uint32_t m_id;
        const char *m_name;
        const char *m_format;
        bool m_draw_needed;
    	va_list m_params;
        
        virtual const char *name()          { return m_name; }
        virtual const char *short_name()    { return m_name; }
        
        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);
        

};




#endif

