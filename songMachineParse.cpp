#include "songMachine.h"
#include "rigNew.h"
#include "myDebug.h"
#include <SdFat.h>

#define dbg_token 1
#define dbg_parse 1


#define MAX_SONG_TEXT   16384

//-----------------------------------
// global variables
//-----------------------------------


extern SdFatSdio SD;
    // in fileSystem.cpp

extern char song_name[80];
    // in songMachine.cpp

int song_text_len = 0;
char song_text[MAX_SONG_TEXT];

// parser state

int song_text_ptr = 0;
int song_code_len = 0;
uint8_t song_code[MAX_SONG_CODE];

char token[MAX_SONG_TOKEN+1] = {0};
int int_token = 0;
int parse_line_num = 0;
int parse_char_num = 0;



const char *tokenToString(int token_num)
{
    if (token_num == TOKEN_DISPLAY                ) return "DISPLAY";
    if (token_num == TOKEN_LOOP_VOLUME            ) return "LOOP_VOLUME";
    if (token_num == TOKEN_SYNTH_VOLUME           ) return "SYNTH_VOLUME";
    if (token_num == TOKEN_GUITAR_VOLUME          ) return "GUITAR_VOLUME";
    if (token_num == TOKEN_GUITAR_EFFECT_NONE     ) return "GUITAR_EFFECT_NONE";
    if (token_num == TOKEN_GUITAR_EFFECT_DISTORT  ) return "GUITAR_EFFECT_DISTORT";
    if (token_num == TOKEN_GUITAR_EFFECT_WAH      ) return "GUITAR_EFFECT_WAH";
    if (token_num == TOKEN_GUITAR_EFFECT_CHORUS   ) return "GUITAR_EFFECT_CHORUS";
    if (token_num == TOKEN_GUITAR_EFFECT_ECHO     ) return "GUITAR_EFFECT_ECHO";
    if (token_num == TOKEN_ON                     ) return "ON";
    if (token_num == TOKEN_OFF                    ) return "OFF";
    if (token_num == TOKEN_CLEAR_LOOPER           ) return "CLEAR_LOOPER";
    if (token_num == TOKEN_BUTTON1                ) return "BUTTON1:";
    if (token_num == TOKEN_LOOP                   ) return "LOOP:";
    if (token_num == TOKEN_LOOPER_TRACK           ) return "LOOPER_TRACK";
    if (token_num == TOKEN_LOOPER_STOP            ) return "LOOPER_STOP";
    if (token_num == TOKEN_DUB_MODE               ) return "DUB_MODE";
    if (token_num == TOKEN_SYNTH_PATCH            ) return "SYNTH_PATCH";
    if (token_num == TOKEN_LOOPER_CLIP            ) return "LOOPER_CLIP";
    if (token_num == TOKEN_MUTE                   ) return "MUTE";
    if (token_num == TOKEN_UNMUTE                 ) return "UNMUTE";
    if (token_num == TOKEN_LOOPER_SET_START_MARK  ) return "LOOPER_SET_START_MARK";
    if (token_num == TOKEN_STRING                 ) return "STRING";
    if (token_num == TOKEN_NUMBER                 ) return "NUMBER";
    if (token_num == TOKEN_EOF                    ) return "EOF";
    return "UNKNOWN_TOKEN_NUMBER";
}


int stringToToken(const char *buf)
{
    if (!strcmp(buf,"DISPLAY"))                    return TOKEN_DISPLAY;
    if (!strcmp(buf,"LOOP_VOLUME"))                return TOKEN_LOOP_VOLUME;
    if (!strcmp(buf,"SYNTH_VOLUME"))               return TOKEN_SYNTH_VOLUME;
    if (!strcmp(buf,"GUITAR_VOLUME"))              return TOKEN_GUITAR_VOLUME;
    if (!strcmp(buf,"GUITAR_EFFECT_NONE"))         return TOKEN_GUITAR_EFFECT_NONE;
    if (!strcmp(buf,"GUITAR_EFFECT_DISTORT"))      return TOKEN_GUITAR_EFFECT_DISTORT;
    if (!strcmp(buf,"GUITAR_EFFECT_WAH"))          return TOKEN_GUITAR_EFFECT_WAH;
    if (!strcmp(buf,"GUITAR_EFFECT_CHORUS"))       return TOKEN_GUITAR_EFFECT_CHORUS;
    if (!strcmp(buf,"GUITAR_EFFECT_ECHO"))         return TOKEN_GUITAR_EFFECT_ECHO;
    if (!strcmp(buf,"ON"))                         return TOKEN_ON;
    if (!strcmp(buf,"OFF"))                        return TOKEN_OFF;
    if (!strcmp(buf,"CLEAR_LOOPER"))               return TOKEN_CLEAR_LOOPER;
    if (!strcmp(buf,"BUTTON1:"))                   return TOKEN_BUTTON1;
    if (!strcmp(buf,"LOOP:"))                      return TOKEN_LOOP;
    if (!strcmp(buf,"LOOPER_TRACK"))               return TOKEN_LOOPER_TRACK;
    if (!strcmp(buf,"LOOPER_STOP"))                return TOKEN_LOOPER_STOP;
    if (!strcmp(buf,"DUB_MODE"))                   return TOKEN_DUB_MODE;
    if (!strcmp(buf,"SYNTH_PATCH"))                return TOKEN_SYNTH_PATCH;
    if (!strcmp(buf,"LOOPER_CLIP"))                return TOKEN_LOOPER_CLIP;
    if (!strcmp(buf,"MUTE"))                       return TOKEN_MUTE;
    if (!strcmp(buf,"UNMUTE"))                     return TOKEN_UNMUTE;
    if (!strcmp(buf,"LOOPER_SET_START_MARK"))      return TOKEN_LOOPER_SET_START_MARK;
    return -1;  // unknown token
}


//--------------------------------------
// PARSE
//--------------------------------------


void token_error(const char *errmsg)
{
    my_error("%s at line %d char %d",errmsg,parse_line_num,parse_char_num);
}

void parse_error(const char *errmsg,const char *param)
{
    my_error("%s(%s) at line %d char %d ",errmsg,param,parse_line_num,parse_char_num);
}


int getToken()
{
    if (song_text_ptr >= song_text_len)
    {
        display(dbg_token,"EOF",0);
        return TOKEN_EOF;
    }

    int_token = 0;
    int token_len = 0;
    int token_type = -1;
    bool eof = false;
    bool done = false;
    bool in_comment = false;

    display(dbg_token,"getToken() -----------------",0);

    while (!done)
    {
        char c = song_text[song_text_ptr++];
        parse_char_num++;

        display(dbg_token+1,"%d:%d c=0x%02x '%c' token_len=%d token_type=%d in_comment=%d int_token=%d",
            parse_line_num,
            parse_char_num,
            c,
            c>32?c:' ',
            token_len,
            token_type,
            in_comment,
            int_token);

        // uppercase

        if (token_type != TOKEN_STRING && c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }


        if (c == 10)
        {
            // skip LF
        }
        else if (c == '#')
        {
            display(dbg_token,"start comment",0);

            in_comment = true;
            if (token_type == TOKEN_STRING)
            {
                token_error("unclosed quote");
                return -1;
            }
            else if (token_len)
            {
                done = true;
            }
        }

        // line break

        else if (c == 13)
        {
            display(dbg_token,"line_break",0);

            if (token_type == TOKEN_STRING)
            {
                token_error("unclosed quote");
                return -1;
            }

            display(dbg_token,"end_comment",0);
            in_comment = false;
            parse_line_num++;
            parse_char_num = 0;

            if (token_len)
            {
                done = true;
            }
        }

        else if (in_comment)
        {
            // do nothing
        }

        // quoted strings

        else if (c == '"')
        {
            if (token_type == TOKEN_STRING)
            {
                token[token_len] = 0;
                done = true;
            }
            else if (token_len)
            {
                token_error("unexpected quote");
                return -1;
            }
            else
            {
                token_type = TOKEN_STRING;
            }
        }
        else if (token_type == TOKEN_STRING)
        {
            token[token_len++] = c;
        }

        // white space

        else if (c == 8 || c == 32)
        {
            display(dbg_token,"white_space",0);
            if (token_type == TOKEN_STRING)
            {
                if (token_len >= MAX_SONG_TOKEN)
                {
                    token_error("token too long");
                    return -1;
                }
                token[token_len++] = ' ';
            }
            else if (token_len)
            {
                done = true;
            }
            if (c == 8)
                parse_char_num += 3;
        }

        // numbers

        else if (!token_len && c >= '0' && c <= '9')
        {
            token[token_len++] = c;
            token_type = TOKEN_NUMBER;
            int_token = c - '0';
        }
        else if (token_type == TOKEN_NUMBER)
        {
            if (c >= '0' && c <= '9')
            {
                if (token_len >= 3)
                {
                    token_error("number too long");
                    return -1;
                }

                token[token_len++] = c;
                int_token *= 10;
                int_token += c - '0';
            }
            else
            {
                token_error("bad number");
                return -1;
            }
        }

        else
        {
            if (token_len >= MAX_SONG_TOKEN)
            {
                token_error("token too long");
                return -1;
            }

            token[token_len++] = c;
        }

        if (song_text_ptr >= song_text_len)
        {
            eof = true;
            done = true;
            display(dbg_token,"EOF",0);
        }
        else if (token_len >= MAX_SONG_TOKEN)
        {
            token_error("token too long");
            return -1;
        }

    }   // while !(done)


    // see if the buffer matches a known token

    token[token_len++] = 0;
    display(dbg_token,"DONE(%s)==%d",token,token_type);

    if (token_type == -1)
    {
        token_type = stringToToken(token);
    }

    if (token_type == -1)
    {
        if (eof)
            return TOKEN_EOF;
        parse_error("unknown token",token);
    }


    return token_type;
}


bool addSongCode(uint8_t byte)
{
    if (song_code_len >= MAX_SONG_CODE)
    {
        my_error("CODE TOO LONG !!  line(%d) char(%d)",parse_line_num, parse_char_num);
        return false;
    }
    song_code[song_code_len++] = byte;
    return true;
}



bool parseSongText()
{
    song_code_len = 0;
    song_text_ptr = 0;
    parse_line_num = 1;
    parse_char_num = 0;

    display(dbg_parse,"",0);

    int t = getToken();
    while (t != TOKEN_EOF)
    {
        if (t < 0)
            return false;

        const char *ttype = tokenToString(t);

        // opcodes that take string parameters

        if (t == TOKEN_SYNTH_PATCH ||
            t == TOKEN_DISPLAY)
        {
            int t2 = getToken();
            if (t2 != TOKEN_STRING)
            {
                parse_error("expected string following",ttype);
                return false;
            }
            if (t == TOKEN_DISPLAY)
            {
                int len = strlen(token);
                display(dbg_parse,"    %s %d:'%s'",ttype,len,token);
                if (!addSongCode(t))
                    return false;
                if (!addSongCode(len))
                    return false;
                for (int i=0; i<len; i++)
                {
                    if (!addSongCode(token[i]))
                        return false;
                }

            }
            else
            {
                int patch_num = theNewRig->findPatchByName(token);
                if (patch_num == -1)
                {
                    parse_error("Could not find patch",token);
                    return false;
                }
                display(dbg_parse,"    %s '%s' == %d",ttype,token,patch_num);
                if (!addSongCode(t))
                    return false;
                if (!addSongCode(patch_num))
                    return false;
            }
        }

        // opcodes that take single integer parameters

        else if (t == TOKEN_SYNTH_VOLUME ||
                 t == TOKEN_LOOP_VOLUME ||
                 t == TOKEN_GUITAR_VOLUME ||
                 t == TOKEN_LOOPER_TRACK)
        {
            int t2 = getToken();
            if (t2 != TOKEN_NUMBER)
            {
                parse_error("expected integer following",ttype);
                return false;
            }
            if (int_token > 127)
            {
                parse_error("integer out of range",ttype);
            }
            if (t == TOKEN_LOOPER_TRACK && (int_token==0 || int_token>4))
            {
                parse_error("track number out of range",ttype);
            }

            display(dbg_parse,"    %s %d",ttype,int_token);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(int_token))
                return false;

        }

        // opcodes that take ON or OFF

        else if (t == TOKEN_GUITAR_EFFECT_DISTORT ||
                 t == TOKEN_GUITAR_EFFECT_WAH  ||
                 t == TOKEN_GUITAR_EFFECT_CHORUS ||
                 t == TOKEN_GUITAR_EFFECT_ECHO)
        {
            int t2 = getToken();
            if (t2 != TOKEN_ON && t2 != TOKEN_OFF)
            {
                parse_error("expected ON or OFF following",ttype);
                return false;
            }

            int val = t2 == TOKEN_ON ? 1 : 0;
            display(dbg_parse,"    %s %d",ttype,val);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(val))
                return false;

        }

        // LOOPER_CLIP takes clip_num (1..4) MUTE or UNMUTE

        else if (t == TOKEN_LOOPER_CLIP)
        {
            int t2 = getToken();
            if (t2 != TOKEN_NUMBER)
            {
                parse_error("expected integer following",ttype);
                return false;
            }
            if (int_token == 0 || int_token > 4)
            {
                parse_error("clip number out of range",ttype);
            }
            int clip_num = int_token;

            t2 = getToken();
            if (t2 != TOKEN_MUTE && t2 != TOKEN_UNMUTE)
            {
                parse_error("expected MUTE or UNMUTE follwing integer for",ttype);
                return false;
            }

            int val = t2 == TOKEN_MUTE ? 1 : 0;
            display(dbg_parse,"    %s %d,%d",ttype,clip_num,val);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(clip_num))
                return false;
            if (!addSongCode(val))
                return false;
        }


        // outdented (for debugging) tokens

        else if (t == TOKEN_BUTTON1 ||
                 t == TOKEN_LOOP)
        {
            display(dbg_parse,"%s",ttype);
            if (!addSongCode(t))
                return false;
        }

        // allowed monadic opcodes

        else if (t == TOKEN_GUITAR_EFFECT_NONE ||
                 t == TOKEN_CLEAR_LOOPER ||
                 t == TOKEN_LOOPER_STOP ||
                 t == TOKEN_DUB_MODE ||
                 t == TOKEN_LOOPER_SET_START_MARK)
        {
            display(dbg_parse,"    %s",ttype);
            if (!addSongCode(t))
                return false;
        }

        // all other token are illegal to start an opcode

        else
        {
            parse_error("unexpected ",ttype);
            return false;
        }

        t = getToken();
    }

    return true;
}




//--------------------------------------
// open and read into memory
//--------------------------------------


bool openSongFile(const char *name)
{
    song_text_len = 0;
    song_code_len = 0;

    char name_buffer[128];
    strcpy(name_buffer,"/songs/");
    strcat(name_buffer,name);
    strcat(name_buffer,".song");

    display(0,"openSongFile(%s)",name);

    File the_file = SD.open(name_buffer);
    if (!the_file)
    {
        my_error("Could not open song file: %s",name_buffer);
        return false;
    }
    uint32_t size = the_file.size();
    if (size > MAX_SONG_TEXT)
    {
        my_error("Song(%s) size(%d) exceeds MAX_SONG_TEXT=%d",name,size,MAX_SONG_TEXT);
        the_file.close();
        return false;
    }

    uint32_t got = the_file.read(song_text,size);
    if (got != size)
    {
        my_error("Reading song(%s) got(%d) size(%d)",name,got,size);
        the_file.close();
        return false;
    }

    the_file.close();
    song_text_len = size;
    strcpy(song_name,name);
    display(0,"song file(%s) opened",name);
    return true;
}
