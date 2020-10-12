#ifndef __songParser_h__
#define __songParser_h__

#include "Arduino.h"

#define MAX_SONG_TOKEN  80
#define MAX_SONG_CODE   4096
#define MAX_SONG_TEXT   16384

#define MAX_ID_LEN      12
#define MAX_LABELS      32
#define MAX_SONG_NAMES  255

typedef struct
{
    char name[MAX_ID_LEN+1];
    uint16_t  code_offset;
}   label_t;



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
#define TOKEN_BUTTON2                   14   // "BUTTON2:"
#define TOKEN_BUTTON3                   15   // "BUTTON3:"
#define TOKEN_BUTTON4                   16   // "BUTTON4:"
#define TOKEN_LOOP                      17   // "LOOP:"
#define TOKEN_LOOPER_TRACK              18   // "LOOPER_TRACK"
#define TOKEN_LOOPER_STOP               19   // "LOOPER_STOP"
#define TOKEN_LOOPER_STOP_IMMEDIATE     20   // "LOOPER_STOP"
#define TOKEN_DUB_MODE                  21   // "DUB_MODE"
#define TOKEN_SYNTH_PATCH               22   // "SYNTH_PATCH"
#define TOKEN_LOOPER_CLIP               23   // "LOOPER_CLIP"
#define TOKEN_MUTE                      24   // "MUTE"
#define TOKEN_UNMUTE                    25   // "UNMUTE"
#define TOKEN_LOOPER_SET_START_MARK     26   // "LOOPER_SET_START_MARK"
#define TOKEN_IDENTIFIER                27   // user defined identifier
#define TOKEN_STRING                    28   // "string"
#define TOKEN_NUMBER                    29   // number
#define TOKEN_COMMA                     30   // comma
#define TOKEN_COLON                     31   // colon
#define TOKEN_GOTO                      32   // GOTO
#define TOKEN_BUTTON_COLOR              33   // "BUTTON_COLOR"
#define TOKEN_RED                       34   // colors
#define TOKEN_GREEN                     35   // colors
#define TOKEN_BLUE                      36   // colors
#define TOKEN_YELLOW                    37   // colors
#define TOKEN_PURPLE                    38   // colors
#define TOKEN_ORANGE                    39   // colors
#define TOKEN_WHITE                     40   // colors
#define TOKEN_CYAN                      41   // colors
#define TOKEN_BLACK                     42  // colors
#define TOKEN_EOF                       43   // end of file


class songParser
{
    public:

        static bool openSongFile(const char *name);
        static const char *tokenToString(int token_num);

        static void clear()
        {
            song_text_len = 0;
            song_code_len = 0;
            song_name[0] = 0;
            num_labels = 0;
        }

        static const char *getTheSongName()   { return song_name; }
            // the currently loaded song name

        static int codeLen()               { return song_code_len; }
        static int textLen()               { return song_text_len; }
        static int getCode(int offset)     { return song_code[offset]; }
        static const char *getCodeString(int offset)    { return (const char *)&song_code[offset]; }
        static const uint16_t getCodeInteger(int offset){ return song_code[offset] | (song_code[offset+1]<<8); }

        static int getText(int offset)     { return song_text[offset]; }

        static label_t *findLabel(const char *id);

        // directory list support

        static int getSongNames();
        static const char *getSongName(int i) { return song_names[i]; }
        static void releaseSongNames();


    private:

        static char song_name[80];

        static int song_text_len;
        static int song_text_ptr;
        static char song_text[MAX_SONG_TEXT];

        static int song_code_len;
        static uint8_t song_code[MAX_SONG_CODE];

        static int token_len;
        static int int_token;
        static int parse_line_num;
        static int parse_char_num;
        static char token[MAX_SONG_TOKEN+1];

        static int num_labels;
        static label_t labels[MAX_LABELS];

        static int num_song_names;
        static char *song_names[MAX_SONG_NAMES];

        static bool parseSongText();
        static int getToken();
        static int stringToToken(const char *buf);
        static bool addTokenChar(int c);
        static bool addSongCode(uint8_t byte);
        static void token_error(const char *errmsg);
        static void parse_error(const char *errmsg,const char *param);

};




#endif  // !__songParser_h__