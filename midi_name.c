// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

#include <Arduino.h>
#include "usb_names.h"
#include "defines.h"
#include <myDebug.h>



// my original version of midi_name.c
//
// This was weird.  There's something about the compilation of these static structures
// that did not allow me to fluidly set them at run time.   I had to create the actual
// structures and copy them in toto.

#define MIDI_NAME       {'t','e','e','n','s','y','E','x','p','r','e','s','s','i','o','n'}
#define MIDI_NAME_LEN   16                                              // padding words ^^^^

// added ability to change and init usb at runtime

#define MIDI_NAME_FISHMAN  {'F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y'}
#define MIDI_NAME_FISHMAN_LEN   18


struct usb_string_descriptor_struct usb_string_product_name_normal = {
    2 + MIDI_NAME_LEN * 2,  // uint8_t bLength;
    3,                      // uint8_t bDescriptorType;
    MIDI_NAME               // uint16_t wString[];
};


struct usb_string_descriptor_struct usb_string_product_name_fishman = {
    2 + MIDI_NAME_FISHMAN_LEN * 2,  // uint8_t bLength;
    3,                      // uint8_t bDescriptorType;
    MIDI_NAME_FISHMAN       // uint16_t wString[];
};


struct usb_string_descriptor_struct usb_string_product_name = {
    0,
    3,
    {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0}     // 24
};


void setMidiName(int i)
{
    if (i == 0)
    {
        memcpy(&usb_string_product_name,&usb_string_product_name_normal,2 + MIDI_NAME_LEN * 2);
    }
    else
    {
        memcpy(&usb_string_product_name,&usb_string_product_name_fishman,2 + MIDI_NAME_FISHMAN_LEN * 2);
    }
}