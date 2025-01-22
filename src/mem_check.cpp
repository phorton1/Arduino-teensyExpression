// mem_check.cpp - quick and dirty memory checkinng

#include <Arduino.h>
#include <myDebug.h>


#define WITH_DETAILS 1

// __bss_end was not found in new teensyDuino compiling for teensy 4.1

#define RAM_START   0x1fff0000

extern char _sbss;          // start of bss segment
#ifndef __IMXRT1062__       // if !teensy 4
    extern char __bss_end;      // end of bss
#endif
extern char* __brkval;      // end (top) of heape
extern char _estack;        // start (top) of stack



void print_mem_info(const char *what)
{
    char *tos;
    __asm__ volatile(
        "mrs %0, msp	\r\n" :
        "=r" (tos) :: );

    Serial.printf("MEM(%s) bss(%ld) heap(%ld) free(%ld) stack(%ld)\n",
        what ? what : "",
        #ifdef __IMXRT1062__       // if teensy 4
            0,0,
        #else
            ((uint32_t) &__bss_end) - RAM_START,
            ((uint32_t) __brkval) - ((uint32_t) &__bss_end),
        #endif

        ((uint32_t) tos) - ((uint32_t) __brkval),
        ((uint32_t) &_estack) - ((uint32_t) tos) );
}




void print_one(bool hex, const char *name, uint32_t loc)
{
    int blanks = 15 - strlen(name);
    Serial.print("// ");
    Serial.print(name);
    while (blanks > 0)
    {
        Serial.print(" ");
        blanks--;
    }
    if (hex)
    {
        Serial.print("0x");
        Serial.println(loc,HEX);
    }
    else
    {
        Serial.println(loc);
    }
}



void print_long_mem_info(const char *where)
{
    char *tos;
    __asm__ volatile(
        "mrs %0, msp	\r\n" :
        "=r" (tos) :: );

    Serial.print("MEM ");
    if (where)
        Serial.println(where);

    #if WITH_DETAILS
        print_one(1,"ram_start",RAM_START);
        print_one(1,"bss_start",(uint32_t) &_sbss);
        #ifndef __IMXRT1062__       // if !teensy 4
            print_one(1,"heap_start",(uint32_t) &__bss_end);
        #endif
        print_one(1,"heap_end",(uint32_t) __brkval);
        print_one(1,"stack_start",(uint32_t) tos);
        print_one(1,"stack_end",(uint32_t) &_estack);
    #endif

    #ifndef __IMXRT1062__       // if !teensy 4
        print_one(0,"mem used",((uint32_t) &__bss_end) - RAM_START);
        print_one(0,"heap_used",((uint32_t) __brkval) - ((uint32_t) &__bss_end) );
    #endif

    print_one(0,"free",((uint32_t) tos) - ((uint32_t) __brkval) );
    print_one(0,"stack_used",((uint32_t) &_estack) - ((uint32_t) tos) );
}


// bare bones program
// mem used       6752
// heap_used      0
// free           255351
// stack_used     41
