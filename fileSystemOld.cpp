//-------------------------------------
// SD Card Test
//-------------------------------------
// This code does NOT work with Arduino/libraries/SdFat library
// which is used in my fileSystem.
//
// I am keeping it because it shows alternative uses of Arduino/Teensy
// libraries

#define WITH_SDCARD      0
    // test defines at this time


#if WITH_SDCARD
    #include <SD.h>
    #include <SPI.h>

    Sd2Card card;
    SdVolume volume;
    SdFile root;

    const int chipSelect = BUILTIN_SDCARD;
        // change this to match your SD shield or module;
        // Arduino Ethernet shield: pin 4
        // Adafruit SD shields and modules: pin 10
        // Sparkfun SD shield: pin 8
        // Teensy audio board: pin 10
        // Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD
        // Wiz820+SD board: pin 4
        // Teensy 2.0: pin 0
        // Teensy++ 2.0: pin 20


    void sdCardTest()
    {
        // UNCOMMENT THESE TWO LINES FOR TEENSY AUDIO BOARD:
        // SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
        // SPI.setSCK(14);  // Audio shield has SCK on pin 14

        Serial.print("\nInitializing SD card...");

        // we'll use the initialization code from the utility libraries
        // since we're just testing if the card is working!

        if (!card.init(SPI_HALF_SPEED, chipSelect))
        {
            Serial.println("initialization failed. Things to check:");
            Serial.println("* is a card inserted?");
            Serial.println("* is your wiring correct?");
            Serial.println("* did you change the chipSelect pin to match your shield or module?");
            return;
        } else
        {
            Serial.println("Wiring is correct and a card is present.");
        }

        // print the type of card
        Serial.print("\nCard type: ");
        switch(card.type())
        {
            case SD_CARD_TYPE_SD1:
                Serial.println("SD1");
                break;
            case SD_CARD_TYPE_SD2:
                Serial.println("SD2");
                break;
            case SD_CARD_TYPE_SDHC:
                Serial.println("SDHC");
                break;
            default:
                Serial.println("Unknown");
        }

        // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
        if (!volume.init(card))
        {
            Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
            return;
        }

        // print the type and size of the first FAT-type volume
        uint32_t volumesize;
        Serial.print("\nVolume type is FAT");
        Serial.println(volume.fatType(), DEC);
        Serial.println();

        volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
        volumesize *= volume.clusterCount();       // we'll have a lot of clusters
        if (volumesize < 8388608ul)
        {
            Serial.print("Volume size (bytes): ");
            Serial.println(volumesize * 512);        // SD card blocks are always 512 bytes
        }
        Serial.print("Volume size (Kbytes): ");
        volumesize /= 2;
        Serial.println(volumesize);
        Serial.print("Volume size (Mbytes): ");
        volumesize /= 1024;
        Serial.println(volumesize);

        Serial.println("\nFiles found on the card (name, date and size in bytes): ");
        root.openRoot(volume);

        // list all files in the card with date and size
        root.ls(LS_R | LS_DATE | LS_SIZE);
    }
#endif

// this goes in the ino file's setup() method

#if WITH_SDCARD
    sdCardTest();
#endif
