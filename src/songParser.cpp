#include "songParser.h"
#include "songMachine.h"
#include "rigBase.h"
#include "myDebug.h"
#include "fileSystem.h"

#define dbg_token 2
#define dbg_parse 2

#define SONG_DIR    "/songs"



//-----------------------------------
// global variables
//-----------------------------------



char songParser::song_name[80];

int songParser::song_text_len = 0;
int songParser::song_text_ptr = 0;
char songParser::song_text[MAX_SONG_TEXT];

// parser state

int songParser::song_code_len = 0;
uint8_t songParser::song_code[MAX_SONG_CODE];

int songParser::token_len = 0;;
int songParser::int_token = 0;
int songParser::parse_line_num = 0;
int songParser::parse_char_num = 0;
int songParser::token_line_num = 0;
int songParser::token_char_num = 0;
char songParser::token[MAX_SONG_TOKEN+1] = {0};
bool songParser::in_method = 0;;

int songParser::num_labels = 0;
label_t songParser::labels[MAX_LABELS];

int songParser::num_song_names = 0;
char *songParser::song_names[MAX_SONG_NAMES];

int songParser::num_relocations = 0;
relocation_t songParser::relocs[MAX_RELOCATIONS];


// static methods

const char *songParser::tokenToString(int token_num)
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
    if (token_num == TOKEN_BUTTON1                ) return "BUTTON1";
    if (token_num == TOKEN_BUTTON2                ) return "BUTTON2";
    if (token_num == TOKEN_BUTTON3                ) return "BUTTON3";
    if (token_num == TOKEN_BUTTON4                ) return "BUTTON4";
    if (token_num == TOKEN_LOOP                   ) return "LOOP";
    if (token_num == TOKEN_LOOPER_TRACK           ) return "LOOPER_TRACK";
    if (token_num == TOKEN_LOOPER_STOP            ) return "LOOPER_STOP";
    if (token_num == TOKEN_LOOPER_STOP_IMMEDIATE  ) return "LOOPER_STOP_IMMEDIATE";
    if (token_num == TOKEN_LOOP_IMMEDIATE         ) return "LOOP_IMMEDIATE";
    if (token_num == TOKEN_DUB_MODE               ) return "DUB_MODE";
    if (token_num == TOKEN_SYNTH_PATCH            ) return "SYNTH_PATCH";
    if (token_num == TOKEN_LOOPER_CLIP            ) return "LOOPER_CLIP";
    if (token_num == TOKEN_MUTE                   ) return "MUTE";
    if (token_num == TOKEN_UNMUTE                 ) return "UNMUTE";
    if (token_num == TOKEN_LOOPER_SET_START_MARK  ) return "LOOPER_SET_START_MARK";
    if (token_num == TOKEN_IDENTIFIER             ) return "IDENT";
    if (token_num == TOKEN_STRING                 ) return "STRING";
    if (token_num == TOKEN_NUMBER                 ) return "NUMBER";
    if (token_num == TOKEN_COMMA                  ) return "COMMA";
    if (token_num == TOKEN_COLON                  ) return "COLON";
    if (token_num == TOKEN_DELAY                  ) return "DELAY";
    if (token_num == TOKEN_GOTO                   ) return "GOTO";
    if (token_num == TOKEN_CALL                   ) return "CALL";
    if (token_num == TOKEN_METHOD                 ) return "METHOD";
    if (token_num == TOKEN_END_METHOD             ) return "END_METHOD";
    if (token_num == TOKEN_BUTTON_COLOR           ) return "BUTTON_COLOR";
    if (token_num == TOKEN_RED                    ) return "RED";
    if (token_num == TOKEN_GREEN                  ) return "GREEN";
    if (token_num == TOKEN_BLUE                   ) return "BLUE";
    if (token_num == TOKEN_YELLOW                 ) return "YELLOW";
    if (token_num == TOKEN_PURPLE                 ) return "PURPLE";
    if (token_num == TOKEN_ORANGE                 ) return "ORANGE";
    if (token_num == TOKEN_WHITE                  ) return "WHITE";
    if (token_num == TOKEN_CYAN                   ) return "CYAN";
    if (token_num == TOKEN_BLACK                  ) return "BLACK";
    if (token_num == TOKEN_FLASH                  ) return "FLASH";
    if (token_num == TOKEN_EOF                    ) return "EOF";
    return "UNKNOWN_TOKEN_NUMBER";
}


int songParser::stringToToken(const char *buf)
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
    if (!strcmp(buf,"BUTTON1"))                    return TOKEN_BUTTON1;
    if (!strcmp(buf,"BUTTON2"))                    return TOKEN_BUTTON2;
    if (!strcmp(buf,"BUTTON3"))                    return TOKEN_BUTTON3;
    if (!strcmp(buf,"BUTTON4"))                    return TOKEN_BUTTON4;
    if (!strcmp(buf,"LOOP"))                       return TOKEN_LOOP;
    if (!strcmp(buf,"LOOPER_TRACK"))               return TOKEN_LOOPER_TRACK;
    if (!strcmp(buf,"LOOPER_STOP"))                return TOKEN_LOOPER_STOP;
    if (!strcmp(buf,"LOOPER_STOP_IMMEDIATE"))      return TOKEN_LOOPER_STOP_IMMEDIATE;
    if (!strcmp(buf,"LOOP_IMMEDIATE"))             return TOKEN_LOOP_IMMEDIATE;
    if (!strcmp(buf,"DUB_MODE"))                   return TOKEN_DUB_MODE;
    if (!strcmp(buf,"SYNTH_PATCH"))                return TOKEN_SYNTH_PATCH;
    if (!strcmp(buf,"LOOPER_CLIP"))                return TOKEN_LOOPER_CLIP;
    if (!strcmp(buf,"MUTE"))                       return TOKEN_MUTE;
    if (!strcmp(buf,"UNMUTE"))                     return TOKEN_UNMUTE;
    if (!strcmp(buf,"LOOPER_SET_START_MARK"))      return TOKEN_LOOPER_SET_START_MARK;
    if (!strcmp(buf,"DELAY"))                      return TOKEN_DELAY;
    if (!strcmp(buf,"GOTO"))                       return TOKEN_GOTO;
    if (!strcmp(buf,"CALL"))                       return TOKEN_CALL;
    if (!strcmp(buf,"METHOD"))                     return TOKEN_METHOD;
    if (!strcmp(buf,"END_METHOD"))                 return TOKEN_END_METHOD;
    if (!strcmp(buf,"BUTTON_COLOR"))               return TOKEN_BUTTON_COLOR;
    if (!strcmp(buf,"RED"))                        return TOKEN_RED;
    if (!strcmp(buf,"GREEN"))                      return TOKEN_GREEN;
    if (!strcmp(buf,"BLUE"))                       return TOKEN_BLUE;
    if (!strcmp(buf,"YELLOW"))                     return TOKEN_YELLOW;
    if (!strcmp(buf,"PURPLE"))                     return TOKEN_PURPLE;
    if (!strcmp(buf,"ORANGE"))                     return TOKEN_ORANGE;
    if (!strcmp(buf,"WHITE"))                      return TOKEN_WHITE;
    if (!strcmp(buf,"CYAN"))                       return TOKEN_CYAN;
    if (!strcmp(buf,"BLACK"))                      return TOKEN_BLACK;
    if (!strcmp(buf,"FLASH"))                      return TOKEN_FLASH;
    return -1;  // unknown token
}



void songParser::releaseSongNames()
{
    for (int i=0; i<num_song_names; i++)
    {
        free(song_names[i]);
    }
    num_song_names = 0;
    memset(song_names,0,MAX_SONG_NAMES * sizeof(char *));
}


int mystrcmpi(const char *str1, const char *str2)
{
    while (*str1 && *str2)
    {
        int c1 = *str1++;
        int c2 = *str2++;
        if (c1 >= 'a' && c1 <= 'z') { c1 = c1 - 'a' + 'A'; }
        if (c2 >= 'a' && c2 <= 'z') { c2 = c2 - 'a' + 'A'; }
        if (c1 > c2) return 1;
        if (c1 < c2) return -1;
    }
    if (*str1) return 1;
    if (*str2) return -1;
    return 0;
}


int songParser::getSongNames()
{
    num_song_names = 0;
    memset(song_names,0,MAX_SONG_NAMES * sizeof(char *));
    File the_dir = SD.open(SONG_DIR);
    if (!the_dir)
    {
        song_error("Could not opendir ",SONG_DIR);
        return 0;
    }

    File entry = the_dir.openNextFile();
    while (entry)
    {
        char filename[255];
        entry.getName(filename, sizeof(filename));

        if (!entry.isDirectory())
        {
            int len = strlen(filename)-5;
            if (len>-0 && !strcmp(".song",&filename[len]))
            {
                if (num_song_names < MAX_SONG_NAMES)
                {
                    char *fn = new char[len+1];
                    strncpy(fn,filename,len);
                    fn[len] = 0;
                    song_names[num_song_names++] = fn;
                }
                else
                {
                    song_error("warning: too many song names",0);
                }
            }
        }

        entry.close();
        entry = the_dir.openNextFile();

    }   // while (entry)

    the_dir.close();

    // sort em

    int i = 0;
    while (i<num_song_names-1)
    {
        int rslt = mystrcmpi(song_names[i],song_names[i+1]);
        if (rslt > 0)
        {
            char *t = song_names[i];
            song_names[i] = song_names[i+1];
            song_names[i+1] = t;
            if (i>0) i--;
        }
        else
        {
            i++;
        }
    }

    return num_song_names;
}




//--------------------------------------
// utils
//--------------------------------------

void songParser::token_error(const char *errmsg)
{
    song_error("%d:%d %s",parse_line_num,parse_char_num,errmsg);
}

void songParser::parse_error(const char *errmsg,const char *param)
{
    song_error("%d:%d %s(%s)",token_line_num,token_char_num,errmsg,param);
}


bool songParser::addTokenChar(int c)
{
    if (token_len >= MAX_SONG_TOKEN)
    {
        token_error("token too long");
        return false;
    }
    token[token_len++] = c;
    return true;
}


bool songParser::addSongCode(uint8_t byte)

{
    if (song_code_len >= MAX_SONG_CODE)
    {
        song_error("CODE TOO LONG !!  line(%d) char(%d)",parse_line_num, parse_char_num);
        return false;
    }
    song_code[song_code_len++] = byte;
    return true;
}


int songParser::getTokenIf(int matches_token)
{
    int save_ptr = song_text_ptr;
    int save_line = parse_line_num;
    int save_char = parse_char_num;

    int i = getToken();
    if (i == -1)
        return -1;

    if (i == matches_token)
        return i;
    song_text_ptr = save_ptr;
    parse_line_num = save_line;
    parse_char_num = save_char;
    return 0;
}


bool songParser::getComma(const char* ttype)
{
    int t2 = getToken();
    if (t2<0)
        return false;
    if (t2 != TOKEN_COMMA)
    {
        parse_error("expected comma in ",ttype);
        return false;
    }
    return true;
}



label_t *songParser::findLabel(const char *id)
{
	char buf1[MAX_ID_LEN+1];
	strcpy(buf1,id);
	songMachine::uc(buf1);

	for (int i=0; i<num_labels; i++)
	{
		char buf2[MAX_ID_LEN+1];
        label_t *pLabel = &labels[i];
		strcpy(buf2,pLabel->name);
		songMachine::uc(buf2);

		if (!strcmp(buf1,buf2))
			return pLabel;
	}
	return 0;
}




label_t *songParser::addLabel(const char *ident, int offset)
{
    display(dbg_parse+1,"addLabel(%d) %d:%s",num_labels,offset,ident);
    if (strlen(ident) > MAX_ID_LEN)
    {
        parse_error("label too long",ident);
        return 0;
    }

    // we are called with -1 for references AFTER seeing if
    // it already exists ... or with >= 0 for definitions
    // we are never called with -1 when there is a -1 label
    // already existing ...

    label_t *label = findLabel(ident);
    if (label)
    {
        if (offset != -1 && label->code_offset != -1)
        {
            parse_error("multiply defined label",ident);
            return 0;
        }
        if (label->code_offset == -1 && offset != -1)
        {
            display(dbg_parse+1,"    resolving label to %d",offset);
            label->code_offset = offset;
        }
        return label;
    }

    if (num_labels >= MAX_LABELS)
    {
        parse_error("too manu labels",ident);
        return 0;
    }

    label = &labels[num_labels++];
    strcpy(label->name,ident);
    label->code_offset = offset;
    return label;
}




bool songParser::addRelocation(label_t *label, int offset)
{
    display(dbg_parse+1,"addRelocation(%s,%d)",label->name,offset);

    if (num_relocations >= MAX_RELOCATIONS)
    {
        parse_error("TOO MANY RELOCATIONS",label->name);
        return false;
    }
    relocation_t *rel = &relocs[num_relocations++];
    rel->label = label;
    rel->offset = offset;
    rel->line_num = token_line_num;
    rel->char_num = token_char_num;
    return true;
}

//--------------------------------------
// getToken
//--------------------------------------

int songParser::getToken()
{
    if (song_text_ptr >= song_text_len)
    {
        display(dbg_token,"EOF",0);
        return TOKEN_EOF;
    }

    int_token = 0;
    token_len = 0;
    int token_type = -1;
    bool done = false;
    bool in_comment = false;

    display(dbg_token,"getToken() -----------------",0);

    while (!done && song_text_ptr < song_text_len)
    {
        char c = song_text[song_text_ptr++];
        parse_char_num++;
        if (!token_len)
        {
            token_line_num = parse_line_num;
            token_char_num  = parse_char_num;
        }

        display(dbg_token+1,"%d:%d c=0x%02x '%c' token_len=%d token_type=%d in_comment=%d int_token=%d",
            parse_line_num,
            parse_char_num,
            c,
            c>32?c:' ',
            token_len,
            token_type,
            in_comment,
            int_token);

        if (c == 10)
        {
            // skip LF, though it also resets character number
            parse_char_num = 0;
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

            if (in_comment)
            {
                display(dbg_token,"end_comment",0);
                in_comment = false;
            }

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
            if (c=='\\' && song_text[song_text_ptr]=='n')
            {
                song_text_ptr += 1;
                parse_char_num++;
                c = 13;
            }
            if (!addTokenChar(c))
                return -1;
        }

        // delims

        else if (c == ',' || c == ':')
        {
            done = true;
            display(dbg_token,"comma",0);
            if (!token_len)
            {
                token_type = c == ',' ? TOKEN_COMMA : TOKEN_COLON;
                if (!addTokenChar(c))
                    return -1;
            }
            else
            {
                song_text_ptr--;        // backup for next time
                parse_char_num--;
            }
        }

        // white space

        else if (c == 8 || c == 32)
        {
            display(dbg_token,"white_space",0);
            if (token_len)
            {
                done = true;
            }
        }

        // numbers

        else if (!token_len && c >= '0' && c <= '9')
        {
            token_type = TOKEN_NUMBER;
            int_token = c - '0';
            if (!addTokenChar(c))
                return -1;
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
                int_token *= 10;
                int_token += c - '0';
                if (!addTokenChar(c))
                    return -1;
            }
            else
            {
                token_error("bad number");
                return -1;
            }
        }

        // building possible identifier we only allow letter, underscore, and numbers

        else if (c == '_' || c == '-' || c == '.' ||
                 (c>='A' && c<='Z') ||
                 (c>='a' && c<='z') ||
                 (c>='0' && c<='9'))
        {
            if (!addTokenChar(c))
                return -1;
        }
        else
        {
            token_error("illegal character");
            return -1;
        }

    }   // while !(done)

    if (!done && !token_len)
        token_type = TOKEN_EOF;

    // see if the buffer matches a known token

    token[token_len++] = 0;
    display(dbg_token,"DONE(%s)==%d",token,token_type);


    if (token_type == -1)
    {
        // uppercase
        char uc_token[MAX_SONG_TOKEN+1];
        strcpy(uc_token,token);
        for (int i=0; i<token_len; i++)
        {
            int c = uc_token[i];
            if (c >= 'a' && c <= 'z')
            {
                uc_token[i] = c - 'a' + 'A';
            }
        }
        token_type = stringToToken(uc_token);
        if (token_type == -1)
            token_type = TOKEN_IDENTIFIER;
    }

    display(dbg_token-1,"getToken() ---> %d:%s",
            token_type,
            tokenToString(token_type),
            token_type == TOKEN_IDENTIFIER || token_type == TOKEN_STRING ? token : "");

    return token_type;
}




bool songParser::parseSongText(rigBase *baseRig)
{
    display(dbg_parse,"",0);
    init_parse();

    int t = getToken();
    while (t != TOKEN_EOF)
    {
        if (t < 0)
            return false;

        display(dbg_parse+1,"line(%d) char(%d)",token_line_num,token_char_num);

        const char *ttype = tokenToString(t);

        // opcodes that take string identifiers

        if (t == TOKEN_SYNTH_PATCH)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_IDENTIFIER)
            {
                parse_error("expected identifier following",ttype);
                return false;
            }

            int patch_num = baseRig ? baseRig->findPatchByName(token) : -1;
            if (patch_num == -1)
            {
                parse_error("Could not find patch",token);
                return false;
            }
            display(dbg_parse,"%-5d:    %s '%s' == %d",song_code_len,ttype,token,patch_num);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(patch_num))
                return false;
        }

        // opcodes that take string parameters

        else if (t == TOKEN_DISPLAY)
        {
            // number

            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_NUMBER || (
                int_token != 1 && int_token != 2))
            {
                parse_error("expected 1 or 2 for",ttype);
                return false;
            }
            int display_num = int_token;

            if (!addSongCode(t))
                return false;
            if (!addSongCode(display_num))
                return false;

            if (!getComma(ttype))
                return false;

            // string

            t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_STRING)
            {
                parse_error("expected string following",ttype);
                return false;
            }

            const char *str_ptr = (const char *) &song_code[song_code_len];
            int len = strlen(token);
            for (int i=0; i<len; i++)
            {
                if (!addSongCode(token[i]))
                    return false;
            }
            if (!addSongCode(0))
                return false;

            // optional color

            int color = 0;
            const char *opt_display_comma = "";
            const char *opt_display_color = "";
            int got_opt = getTokenIf(TOKEN_COMMA);
            if (got_opt == -1)
                return false;
            if (got_opt)
            {
                t2 = getToken();
                if (t2<0)
                    return false;
                if (t2 != TOKEN_RED    &&
                    t2 != TOKEN_GREEN  &&
                    t2 != TOKEN_BLUE   &&
                    t2 != TOKEN_YELLOW &&
                    t2 != TOKEN_PURPLE &&
                    t2 != TOKEN_ORANGE &&
                    t2 != TOKEN_WHITE  &&
                    t2 != TOKEN_CYAN   &&
                    t2 != TOKEN_BLACK)
                {
                    parse_error("expected a color follwing comma for",ttype);
                    return false;
                }
                color = t2;
                opt_display_comma = ",";
                opt_display_color = tokenToString(t2);
            }
            if (!addSongCode(color))
                return false;

            display(dbg_parse,"%-5d:    %s %d,'%s'%s%s",
                song_code_len,
                ttype,
                display_num,
                str_ptr,
                opt_display_comma,
                opt_display_color);

        }

        // opcodes that take single integer parameters

        else if (t == TOKEN_DELAY ||
                 t == TOKEN_LOOPER_TRACK)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_NUMBER)
            {
                parse_error("expected integer following",ttype);
                return false;
            }
            if (t == TOKEN_LOOPER_TRACK && (int_token==0 || int_token>4))
            {
                parse_error("track number out of range",ttype);
                return false;
            }
            if (t == TOKEN_LOOPER_TRACK && int_token > 255)
            {
                parse_error("delay integer out of range",ttype);
                return false;
            }

            display(dbg_parse,"%-5d:    %s %d",song_code_len,ttype,int_token);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(int_token))
                return false;
        }

        // opcodes that take opt comma numbers

        else if (t == TOKEN_SYNTH_VOLUME ||
                 t == TOKEN_LOOP_VOLUME ||
                 t == TOKEN_GUITAR_VOLUME)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_NUMBER)
            {
                parse_error("expected integer following",ttype);
                return false;
            }
            if (int_token > 127)
            {
                parse_error("volume integer out of range",ttype);
                return false;
            }

            #define DEFAULT_VOLUME_DELAY_10THS 2
                // by default we will go 200 milliseconds for testing
                // which means that upto 20 commands can be sent
                // by default ...

            int value = int_token;
            int delay = DEFAULT_VOLUME_DELAY_10THS;

            int got_opt = getTokenIf(TOKEN_COMMA);
            if (got_opt == -1)
                return false;
            if (got_opt)
            {
                int t2 = getToken();
                if (t2<0)
                    return false;
                if (t2 != TOKEN_NUMBER)
                {
                    parse_error("expected delay number after value for ",ttype);
                    return false;
                }
                if (int_token > 255)
                {
                    parse_error("delay integer out of range",ttype);
                    return false;
                }
                delay = int_token;
            }

            display(dbg_parse,"%-5d:    %s %d,%d",song_code_len,ttype,value,delay);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(value))
                return false;
            if (!addSongCode(delay))
                return false;
        }

        // opcodes that take ON or OFF

        else if (t == TOKEN_GUITAR_EFFECT_DISTORT ||
                 t == TOKEN_GUITAR_EFFECT_WAH  ||
                 t == TOKEN_GUITAR_EFFECT_CHORUS ||
                 t == TOKEN_GUITAR_EFFECT_ECHO)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_ON && t2 != TOKEN_OFF)
            {
                parse_error("expected ON or OFF following",ttype);
                return false;
            }

            int val = t2 == TOKEN_ON ? 1 : 0;
            display(dbg_parse,"%-5d:    %s %d",song_code_len,ttype,val);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(val))
                return false;

        }

        // LOOPER_CLIP takes clip_num (1..4) MUTE or UNMUTE

        else if (t == TOKEN_LOOPER_CLIP)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
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

            if (!getComma(ttype))
                return false;

            t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_MUTE && t2 != TOKEN_UNMUTE)
            {
                parse_error("expected MUTE or UNMUTE follwing comma for",ttype);
                return false;
            }

            int val = t2 == TOKEN_MUTE ? 1 : 0;
            display(dbg_parse,"%-5d:    %s %d,%d",song_code_len,ttype,clip_num,val);
            if (!addSongCode(t))
                return false;
            if (!addSongCode(clip_num))
                return false;
            if (!addSongCode(val))
                return false;
        }

        // BUTTON_COLOR takes button number (1..4), comma, color constant, opt_comma(FLASH)

        else if (t == TOKEN_BUTTON_COLOR)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_NUMBER)
            {
                parse_error("expected integer following",ttype);
                return false;
            }
            if (int_token == 0 || int_token > 4)
            {
                parse_error("button number out of range",ttype);
            }
            int button_num = int_token;

            if (!getComma(ttype))
                return false;

            t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_RED    &&
                t2 != TOKEN_GREEN  &&
                t2 != TOKEN_BLUE   &&
                t2 != TOKEN_YELLOW &&
                t2 != TOKEN_PURPLE &&
                t2 != TOKEN_ORANGE &&
                t2 != TOKEN_WHITE  &&
                t2 != TOKEN_CYAN   &&
                t2 != TOKEN_BLACK)
            {
                parse_error("expected a color follwing comma for",ttype);
                return false;
            }

            int flash = 0;
            int got_opt = getTokenIf(TOKEN_COMMA);
            if (got_opt == -1)
                return false;
            if (got_opt)
            {
                int t3 = getToken();
                if (t3 != TOKEN_FLASH)
                {
                    parse_error("only option is FLASH",ttype);
                    return false;
                }
                flash = 1;
            }


            display(dbg_parse,"%-5d:    %s %d,%s%s",
                song_code_len,
                ttype,
                button_num,
                tokenToString(t2),
                flash ? ",FLASH" : "");

            if (!addSongCode(t))
                return false;
            if (!addSongCode(button_num))
                return false;
            if (!addSongCode(t2))
                return false;
            if (!addSongCode(flash))
                return false;
        }

        // outdented (for debugging) tokens
        // must be followed by colons

        else if (t == TOKEN_BUTTON1 ||
                 t == TOKEN_BUTTON2 ||
                 t == TOKEN_BUTTON3 ||
                 t == TOKEN_BUTTON4 ||
                 t == TOKEN_LOOP)
        {
            display(dbg_parse,"%-5d: %s",song_code_len,ttype);

            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_COLON)
            {
                parse_error("expected integer following",ttype);
                return false;
            }

            if (!addSongCode(t))
                return false;
        }

        // allowed monadic opcodes

        else if (t == TOKEN_END_METHOD ||
                 t == TOKEN_GUITAR_EFFECT_NONE ||
                 t == TOKEN_CLEAR_LOOPER ||
                 t == TOKEN_LOOPER_STOP ||
                 t == TOKEN_LOOPER_STOP_IMMEDIATE ||
                 t == TOKEN_LOOP_IMMEDIATE ||
                 t == TOKEN_DUB_MODE ||
                 t == TOKEN_LOOPER_SET_START_MARK)
        {
            display(dbg_parse,"%-5d:    %s",song_code_len,ttype);

            if (t == TOKEN_END_METHOD)
            {
                if (!in_method)
                {
                    parse_error("outside of METHOD",ttype);
                    return false;
                }
                in_method = false;
            }
            if (!addSongCode(t))
                return false;
        }

        // lone identifiers (labels) must be followed by colon

        else if (t == TOKEN_IDENTIFIER)
        {
            display(dbg_parse,"%-5d:    %s %s",song_code_len,ttype,token);

            if (token_len > MAX_ID_LEN)
            {
                parse_error("label too long",ttype);
                return false;
            }

            if (!addLabel(token,song_code_len))
                return false;

            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_COLON)
            {
                parse_error("expected colon following label",ttype);
                return false;
            }
        }

        else if (t == TOKEN_GOTO ||
                 t == TOKEN_CALL)
        {
            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_IDENTIFIER)
            {
                parse_error("expected identifier following ",ttype);
                return false;
            }

            uint16_t offset = 0;
            display(dbg_parse,"%-5d:    %s %s",song_code_len,ttype,token);
            label_t *label = findLabel(token);

            bool add_relocation = true;

            if (label)
            {
                display(dbg_parse+1,"foundLabel %d:%s  0x%02x 0x%02x",
                    label->code_offset,
                    label->name,
                    label->code_offset & 0xFF,
                    (label->code_offset >> 8) & 0xFF);

                if (label->code_offset != -1)        // it's real
                {
                    add_relocation = false;
                    offset = label->code_offset;
                }
            }

            // add the relocation with an offset 1 after the GOTO

            if (add_relocation)
            {
                label = addLabel(token,-1);
                if (!label)
                    return false;
                if (!addRelocation(label, song_code_len + 1))
                    return false;
            }

            // store the offset as two bytes, LSB first

            if (!addSongCode(t))
                return false;
            if (!addSongCode(offset & 0xFF))
                return false;
            if (!addSongCode((offset >> 8) & 0xFF))
                return false;
        }

        else if (t == TOKEN_METHOD)
        {
            display(dbg_parse,"%-5d:    %s %s",song_code_len,ttype,token);
            if (in_method)
            {
                parse_error("illegal nested method",ttype);
                return false;
            }
            in_method = 1;

            int t2 = getToken();
            if (t2<0)
                return false;
            if (t2 != TOKEN_IDENTIFIER)
            {
                parse_error("expected identifier following ",ttype);
                return false;
            }

            if (token_len > MAX_ID_LEN)
            {
                parse_error("label too long",ttype);
                return false;
            }

            if (!addLabel(token,song_code_len))
                return false;

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

    if (in_method)
    {
        parse_error("","missing END_METHOD at end of file");
        return false;
    }

    // DO RELOCATIONS

    if (num_relocations)
    {
        display(dbg_parse,"---------- RELOCATIONS ------------",0);
        for (int i=0; i<num_relocations; i++)
        {
            relocation_t *rel = &relocs[i];
            label_t *label = rel->label;

            display(dbg_parse,"   rel(%s) code_offset=%d  rel_offset=%d from line(%d) char(%d)",
                    label->name,label->code_offset,rel->offset,rel->line_num,rel->char_num);

            if (label->code_offset == -1)
            {
                song_error("LABEL %s UNDEFINED at line(%d) char(%d)",
                    label->name,
                    rel->line_num,
                    rel->char_num);
                return false;
            }

            song_code[rel->offset] = label->code_offset & 0xFF;
            song_code[rel->offset+1] = (label->code_offset >> 8) & 0xFF;
        }
    }


    display(dbg_parse,"parseSongText() returning true",0);
    return true;
}



//--------------------------------------
// open and read into memory
//--------------------------------------


char *songParser::openSongFile(const char *name)
{
    song_text_len = 0;
    song_code_len = 0;

    char name_buffer[128];
    strcpy(name_buffer,SONG_DIR);
    strcat(name_buffer,"/");
    strcat(name_buffer,name);
    strcat(name_buffer,".song");

    display(0,"openSongFile(%s)",name);

    File the_file = SD.open(name_buffer);
    if (!the_file)
    {
        song_error("Could not open song file: %s",name_buffer);
        return 0;
    }
    uint32_t size = the_file.size();
    if (size > MAX_SONG_TEXT)
    {
        song_error("Song(%s) size(%d) exceeds MAX_SONG_TEXT=%d",name,size,MAX_SONG_TEXT);
        the_file.close();
        return 0;
    }

    uint32_t got = the_file.read(song_text,size);
    if (got != size)
    {
        song_error("Reading song(%s) got(%d) size(%d)",name,got,size);
        the_file.close();
        return 0;
    }

    the_file.close();
    song_text_len = size;
    strcpy(song_name,name);
    display(0,"song file(%s) opened",name);

    return song_text;
}
