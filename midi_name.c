// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

#include "usb_names.h"
#include "defines.h"


// Edit these lines to create your own name.  The length must
// match the number of characters in your custom name.

#if 1   //  NAME_MIDI_DEVICE_AS_FISHMAN
        #define MIDI_NAME   {'F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y'}
        #define MIDI_NAME_LEN  18
#else
        #define MIDI_NAME   {'t','e','e','n','s','y','E','x','p','r','e','s','s','i','o','n'}
        #define MIDI_NAME_LEN  16
#endif

// Do not change this part.  This exact format is required by USB.

struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};
