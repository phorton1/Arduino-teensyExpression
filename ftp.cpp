#include <myDebug.h>
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "midiQueue.h"


note_t *first_note = 0;
note_t *last_note = 0;
note_t *most_recent_note = 0;
note_t *tuning_note = 0;

int ftp_battery_level = -1;
int ftp_sensitivity[NUM_STRINGS] = {-1,-1,-1,-1,-1,-1};
int ftp_poly_mode = 1;
int ftp_bend_mode = 0;


uint8_t ftp_get_sensitivy_command_string_number = 0;

const int  string_base_notes[6] = {0x40, 0x3b, 0x37, 0x32, 0x2d, 0x28};


const char *getFTPCommandName(uint8_t p2)
{
    if (p2 == FTP_CMD_EDITOR_MODE           ) return "EDITOR_MODE";                 // 0x04
    if (p2 == FTP_CMD_SLIDER_POSITION       ) return "SLIDER_POSITION";             // 0x05
    if (p2 == FTP_CMD_BATTERY_LEVEL         ) return "BATTERY_LEVEL";               // 0x07
    if (p2 == FTP_CMD_VOLUME_LEVEL          ) return "VOLUME_LEVEL";                // 0x08
    if (p2 == FTP_CMD_DYNAMICS_SENSITIVITY  ) return "DYNAMICS_SENSITIVITY";        // 0x2F
    if (p2 == FTP_CMD_SET_PGM_NUMBER        ) return "SET_PGM_NUMBER";              // 0x28
    if (p2 == FTP_CMD_SET_BANK_LSB          ) return "SET_BANK_LSB";                // 0x29
    if (p2 == FTP_CMD_SET_BANK_MSB          ) return "SET_BANK_MSB";                // 0x2A
    if (p2 == FTP_CMD_PITCHBEND_MODE        ) return "PITCHBEND_MODE";              // 0x2B
    if (p2 == FTP_CMD_TRANSPOSE             ) return "TRANSPOSE";                   // 0x2C
    if (p2 == FTP_CMD_MIDI_VOLUME           ) return "MIDI_VOLUME";                 // 0x2D
    if (p2 == FTP_CMD_DYNAMICS_OFFSET       ) return "DYNAMICS_OFFSET";             // 0x30
    if (p2 == FTP_CMD_MIDI_REVERB           ) return "MIDI_REVERB";                 // 0x31
    if (p2 == FTP_CMD_GET_SENSITIVITY       ) return "GET_SENSITIVITY";             // 0x3C
    if (p2 == FTP_CMD_POLY_MODE             ) return "POLY_MODE";                   // 0x3f
    if (p2 == FTP_CMD_SET_SENSITIVITY       ) return "SET_SENSITIVITY";             // 0x42
    if (p2 == FTP_CMD_BLOCK_MIDI_NOTES      ) return "BLOCK_MIDI_NOTES";            // 0x46
    if (p2 == FTP_CMD_TOUCH_SENSITIVITY     ) return "TOUCH_SENSITIVITY";           // 0x4f
    if (p2 == FTP_CMD_SPLIT_NUMBER          ) return "SPLIT_NUMBER";                // 0x52
    return 0;
}



const char *note_names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};


const char *noteName(uint8_t note)
{
    return note_names[note % 12];
}


note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2)
    // -1 is an invalid fret
    // zero is an invalid note
{
    note_t *note = new note_t;
    note->val = val;
    note->vel = vel;
    note->vel2 = vel2;
    note->fret = val ? ((int) val) - string_base_notes[string] : -1;
    note->string = string;
    note->tuning = 0;
    note->next = 0;

    __disable_irq();
    if (!first_note)
        first_note = note;
    if (last_note)
    {
        last_note->next = note;
        note->prev = last_note;
    }
    else
        note->prev = 0;
    last_note = note;
    most_recent_note = note;
    __enable_irq();

    return note;
}




void deleteNote(uint8_t string)
{
    __disable_irq();
    note_t *note = first_note;
    while (note && note->string != string)
    {
        note = note->next;
    }
    if (note)
    {
        if (note->prev)
            note->prev->next = note->next;
        if (note->next)
            note->next->prev = note->prev;
        if (note == first_note)
            first_note = note->next;
        if (note == last_note)
            last_note = note->prev;

        if (note == most_recent_note)
            most_recent_note = 0;

        if (note == tuning_note)
            tuning_note = 0;
    }
    else
    {
        warning(0,"could not find note on string(%d) to delete",string);
    }
    __enable_irq();
    delete note;
}







//-------------------------------------------------------------
// patch display
//-------------------------------------------------------------

const char *pedalModeName(int i)
{
    if (i==0) return "BlockNew";
    if (i==1) return "DontBlock";
    if (i==2) return "HoldUp";
    if (i==3) return "HoldDown";
    if (i==4) return "Alternate";
    if (i==5) return "huh?";
    return "unknownPedalMode";
}


const char *pitchBendModeName(int i)
{
	if (i == 1) return "Smooth";
	if (i == 2) return "Stepped";
	if (i == 3) return "Trigger";
	if (i == 0) return "Auto";
    return "unknownPitchBendMode";
}



uint8_t patch_sig[6] = {0xF0, 0x00, 0x01, 0x6E, 0x01, 0x21};

#define show                out_stream->printf
#define colorHeader(i)      { show("\033[%d;%dm    ",color,bg_color);  for (int j=0; j<i; j++) show("    "); }
#define warningHeader(i)    { show("\033[%d;%dm    ",ansi_color_yellow,ansi_color_bg_black);  for (int j=0; j<i; j++) show("    "); }


bool showFtpPatch(
    Stream *out_stream,
    int color,
    int bg_color,
    bool is_ftp_controller,
    uint8_t *patch_buf,
    uint32_t buflen)
    // returns true if there was an error
{
    uint8_t *p = patch_buf;
    patch_sig[5] = is_ftp_controller ? 0x21 : 0x41;
    if (buflen != 142) return false;      // patches are 42

    for (int i=0; i<6; i++)
    {
        if (*p != patch_sig[i])
        {
            warningHeader(0);
            show("sysex of 142 that does not match patch_sig at byte(%d) sig(%02x) != patch(%02x)\n\r",
                i,patch_sig[i],*p);
            return true;
        }
        p++;
    }

    uint8_t bank_num = *p++;
    uint8_t patch_num = *p++;
    colorHeader(0);
    show("PATCH BANK(%d) PATCH(%d)\n\r",bank_num,patch_num);

    // acting on presumption that banks 3-4 ONLY contain the name of the patch

    if (bank_num > 1)
    {
        char name_buf[128];
        memset(name_buf,0,128);
        int name_len = *p++;
        memcpy(name_buf,p,name_len);
        colorHeader(1);
        show("NAME: %s\n\r",name_buf);
        return false;
    }


    patch_buffer0_t *patch = (patch_buffer0_t *) patch_buf;

    colorHeader(1);
    show("Pedal Mode(%d):%s\n\r",patch->pedal_mode,pedalModeName(patch->pedal_mode));

    colorHeader(1);
    show("Fret Range low(%d) high(%d)  String Range low(%d) high(%d)\n\r",
         patch->fret_range_low_up_12,
         patch->fret_range_high_down_34,
         patch->string_range_12,
         patch->string_range_34);

    if (patch->azero0)
    {
        warningHeader(1);
        show("unexpected azero0=0x%02x\n\r",patch->azero0);
    }

    colorHeader(1);
    show("Unused arp_speed=%d  arp_length=%d\n\r",patch->arp_speed,patch->arp_length);

    colorHeader(1);
    show("TOUCH_SENSE=%d   POLY_MODE=%d\n\r",patch->touch_sens,patch->poly_mode);

    if (patch->azero1)
    {
        warningHeader(1);
        show("unexpected azero1=0x%02x\n\r",patch->azero1);
    }

    for (int i=0; i<64; i++)
    {
        if (patch->seqbytes[i] != i)
        {
            warningHeader(1);
            show("unexpected segbytes[%d]=0x%02x\n\r",i,patch->seqbytes[i]);
        }
    }

    if (patch->program[0] != 'P' ||
        patch->program[1] != 'r' ||
        patch->program[2] != 'o' ||
        patch->program[3] != 'g' ||
        patch->program[4] != 'r' ||
        patch->program[5] != 'a' ||
        patch->program[6] != 'm' ||
        patch->program[7] != ' ')
    {
        warningHeader(1);
        show("expected the word 'Program' and a space\n\r");
        display_bytes_long(0,0,patch->program,8,out_stream);
    }

    if (patch->azero2)
    {
        warningHeader(1);
        show("unexpected azero2=0x%02x\n\r",patch->azero2);
    }

    colorHeader(1);
    show("max_volume=0x%02x  max_reverb=0x%02x\n\r",patch->max_volume,patch->max_reverb);

    uint16_t checksum = 0;
    int from = 1;
    int to = 139;
    for (int i=from; i<to; i++)
    {
        checksum += patch_buf[i];
    }
    uint8_t my_checksum[2];
    my_checksum[1] = checksum & 0x7f;
    my_checksum[0] = (checksum >> 7) & 0x7f;

    // display(0,"sizeof=(sizeof(patch_buffer0_t)=%d  sysex check=%02x",sizeof(patch_buffer0_t),patch_buf[141]);
    if (my_checksum[0] == patch->checksum[0] &&
        my_checksum[1] == patch->checksum[1])
    {
        colorHeader(1);
    }
    else
    {
        warningHeader(1);
    }
    show("checksum(from %d to %d)=0x%02x%02x calculated=0x%04x my(%02x%02x) endmidi=0x%02x\n\r",
        from,
        to,
        patch->checksum[0],
        patch->checksum[1],
        checksum,
        my_checksum[0],
        my_checksum[1],
        patch->endmidi);

    for (int i=0; i<5; i++)
    {
        colorHeader(1);
        show("SPLIT(%d)\n\r",i);
        split_section_t *split = &patch->split[i];

        colorHeader(2);
        show("pgm_change=%d bank_lsb=%d bank_msb=%d\n\r",split->pgm_change,split->bank_lsb,split->bank_msb);

        colorHeader(2);
        show("PITCH_BEND(%d)=%s   transpose=%d\n\r",split->pitch_bend,pitchBendModeName(split->pitch_bend),split->transpose);

        colorHeader(2);
        show("DYN_RANGE=%d DYN_OFFSET=%d\n\r",split->dyn_sens,split->dyn_offs);

        colorHeader(2);
        show("midi_volume=%d midi_reverb=%d\n\r",split->midi_volume,split->midi_reverb);
    }

    return false;
}
