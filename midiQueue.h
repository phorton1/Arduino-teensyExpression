#ifndef _midiQueue_h_
#define _midiQueue_h_



class msgUnion
{
    public:
        
        msgUnion(uint32_t msg)  { i = msg; }
        
        bool isHost()           { return (i & 0x80); }
        int  hostIndex()        { return isHost() ? 1 : 0; }
        uint8_t getMsgType()    { return i & 0x0f; }
        uint8_t getCable()      { return (i>>4) & 0x7; }
        uint8_t getChannel()    { return (b[1] & 0xf) + 1; }
        
        uint8_t type()       { return b[1]; }
        uint8_t param1()     { return b[2]; }
        uint8_t param2()     { return b[3]; }
        
        bool isActiveSense()    { return (i & 0xff0f) == 0xfe0f; }
        
    union {
        uint32_t i;
        uint8_t b[4];
    };
};


// debugging visibility fileter

extern int showSysex;   // 0,1, or 2 for detail
extern bool showActiveSense;
extern bool showTuningMessages;
extern bool showNoteInfoMessages;



extern void showRawMessage(uint32_t i);
extern void processMsg(uint32_t i);
extern void showDisplayQueue();
extern void enqueueDisplay(uint32_t msg);
extern void enqueueProcess(uint32_t msg);
extern uint32_t dequeueProcess();


#endif // !_midiQueue_h_