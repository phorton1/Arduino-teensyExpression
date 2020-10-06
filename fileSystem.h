// abstracted file system for use with serial IO protocol
//
// initially implemented over the main USB serial port
// which DOES NOT expect any input, though it does throw
// out a ton of debugging output.


#ifndef __fileSystem_h__
#define __fileSystem_h__

#include "Arduino.h"

class fileSystem
{
public:

   static bool init();
   static void task();

   static uint32_t getFreeMB();
   static uint32_t getTotalMB();

};


#endif  // !__fileSystem_h__