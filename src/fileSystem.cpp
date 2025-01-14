//----------------------------------------------------------
// fileSystem.cpp
//----------------------------------------------------------
// Abstracted file system for use with serial IO protocol
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.
//
// GET USES 20K of stack!!
// OLD NOTE:
//
// prh - I was getting an error when starting the system from a cold power up.
// it was working ok from a compile (ctrl-I in Komodo) right after the TeensyLoader
// put the program on the teensy, but not on a subsequent cold-powerup.
// I "fixed" it by increasing BUSY_TIMEOUT_MICROS from 500000 to 1000000
// in /Arduino/libraries/SdFat/src/SdCard/SdioTeensy.cpp.
//
// VERSION2 NOTES
//
// In TE1 was using a slighly modified version of the sdFat library v1.1.2 that I
// downloaded from https://github.com/greiman/SdFat 3 years ago in my libraries
// folder.  It was working well with my old version 1.53 of teensyDuino ...
// The only mod I had made was mentioned above.
//
// I was worried about my teensy3.6's, they're being discontinued, and I have two brand new 4.1's.
// But the 4.1 is not supported by that old version of SdFat.  To get SdFat for 4.1's, it was
// "recommended" that one upgrades the teensyDuino to a newer version which includes
// SdFat (and SD) libraries ...
//
// So I went through the hassle of upgrading teensyDuino to v1.58.
// A nice change with the new teensyDuino was that I also was able to do away
// with my usb_desc.prh scheme and use a somewhat approved method of overriding
// the USB definitions.
//
// With teensyDuino 1.58 I got SdFat v2.1.2 in the Program Files (x86)
// folder as part of the deal. So, I saved off and removed my old SdFat library,
// and endeavored to build with the new SdFat library. I immediatly found that
// SdFat v2.1.2 is a complete rewrite from the old version and I had to figure
// out what objects to use just to get it basically working:
//
//		SdFat32 instead of the old SdFatSdio for the SD type
//		File32 instead of the old File for files
//		DirFat_t instead the old dir_t for directory entries
//      ... I still haven't figured out directory entries and datetime stamps
//
// I got it basically working with the teensy3.6 (then made new PCB's and
// fixed the bad connector problem), and then noticed that new SdFat library
// has the opposite problem of the one I 'fixed' in the old library. It
// fails to initialze the SD card on a soft reboot (after a compile/upload),
// and now requiires me to unplug and replug in the teensy each time I want
// to test the program.
//
// So I made the compile conditional based on a USE_OLD_FAT define.
// I moved the SdFat and SD libraries from Program Files (x86) and put
// them in my libraries folder and saved them off elsewhere.
// I can now place either the 'new' or 'old' SdFat library in my
// libraries folder, adjust the #define, and build against either one.
// And I verified that the old one still works on either kind of reboot.
// Howevber,
//
//--------------------------------------------------------------------------------
// I CANNOT HAVE BOTH SDFAT LIBRARIES IN MY LIBRARIES FOLDER AT THE SAME TIME.
//--------------------------------------------------------------------------------
// SO I NEED A GOOD PLACE TO STORE THEM, esp since one is under source control
// and I am tempted to modify them, and /junk is not a good place for them.
// So they are in:     /zip/_teensy/_SdFat_libraries


#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>

#define dbg_ts    1
	// 0 = show timestamp operations
	// -1 = show callback setting

#if USE_OLD_FAT
	SdFatSdio SD;
#else
	SdFat32 SD;
#endif


char write_timestamp[32];
char static_timestamp[32];
	// buffers for use by getTimeStamp and dtCallback

bool has_file_system;
bool has_sd_card;

bool hasFileSystem() { return has_file_system; }
bool hasSDCard()     { return has_sd_card; }


//---------------------------------------------
// debugging
//---------------------------------------------

#define LIST_DIRECTORY_AT_STARTUP  0	// USE_OLD_FAT

#if LIST_DIRECTORY_AT_STARTUP

	#define dbg_print_dir  1

    void printDirectory(myFile_t dir, int numTabs = 0)
    {
        while(true)
        {
            myFile_t entry = dir.openNextFile();
            if (!entry)
                break;

            char filename[255];
            entry.getName(filename, sizeof(filename));

            dir_t dir_entry;
            if (!entry.dirEntry(&dir_entry))
            {
                my_error("Could not get dir_entry for %s",filename);
                return;
            }
            display_level(dbg_print_dir,numTabs + 1,"    cdate(%d) ctime(%d) c10ths(%d)",
                dir_entry.creationDate,dir_entry.creationTime,dir_entry.creationTimeTenths);
            display_level(dbg_print_dir,numTabs + 1,"    adate(%d) wdate(%d) wtime(%d)",
                dir_entry.lastAccessDate,dir_entry.lastWriteDate,dir_entry.lastWriteTime);
            display_level(dbg_print_dir,,numTabs + 1"    attr(0x%02x), size(%d)",
                dir_entry.attributes,dir_entry.fileSize);

            uint16_t year = FAT_YEAR(dir_entry.creationDate);
            uint16_t month = FAT_MONTH(dir_entry.creationDate);
            uint16_t day = FAT_DAY(dir_entry.creationDate);
            uint16_t hour = FAT_HOUR(dir_entry.creationTime);
            uint16_t minute = FAT_MINUTE(dir_entry.creationTime);
            uint16_t second = FAT_SECOND(dir_entry.creationTime);
            uint16_t hundredths = dir_entry.creationTimeTenths;
                // according to comment this actually hundredths ...

            if (hundredths > 100)
            {
                hundredths -= 100;
                second += 1;
            }

			char time_buf[36];
			sprintf(time_buf,"%04d-%02d-%02d %02d:%02d:%02d.%02d",
                year,
                month,
                day,
                hour,
                minute,
                second,
                hundredths);

			char tab_buf[24];
			memset(tab_buf,32,24);
			if (numTabs > 4)
				tab_buf[16] = 0;
			tab_buf[ (4-numTabs) * 4 ] = 0;

            if (entry.isDirectory())
            {
                display(0,":%-32s%s         %s",filename,tab_buf,time_buf);
                printDirectory(entry, numTabs+1);
            }
            else
            {
                display_level(0,numTabs + 1,"%-32s%s%-08d  %s",
                    filename,
					tab_buf,
                    entry.size(),
					time_buf);
            }

            entry.close();
        }
     }
#endif  // LIST_DIRECTORY_AT_STARTUP



//---------------------------------------------
// INIT
//---------------------------------------------

bool initFileSystem()
{


#if USE_OLD_FAT
	SdioCard *card = SD.card();
	has_file_system = SD.begin();
	has_sd_card = has_file_system || card->begin();
#else
	SDFat32Card *card = SD.card();
	has_file_system = SD.begin(BUILTIN_SDCARD);
	has_sd_card = has_file_system || card)->begin();
#endif

	if (!has_sd_card)
	{
        my_error("NO SD CARD!!",0);
        return false;
    }
	if (!has_file_system)
	{
        warning(0,"NO fileSystem found!",0);
    }

   #if 0   // SHOW CID

		delay(1000);

		uint8_t type = card->type();
		uint32_t blocks = card->cardCapacity();
			// 'sectors' == 'blocks' == 512 bytes each
		display(0,"fileSystem: type(%d)=%s  blocks/sectors=%lu",
			type,
			type == SD_CARD_TYPE_SDHC ? "SDHC" :
			type == SD_CARD_TYPE_SD2 ? "SDV2" :
			type == SD_CARD_TYPE_SD1 ? "SDV1" :
			"NO SDCARD",
			blocks);

        cid_t cid;
		memset(&cid,0,sizeof(cid_t));

		// readCID returns true, with no information
		// if card->begin() not called.  I think on the teensy
		// it just returns the memory that is gotten in card.begin()

        if (!card->readCID(&cid))
			warning(0,"Could not read SDCard cid",0);

        // unsigned char mid;                  // manufacturer id
        // char oid[2];                        // OEM/Application ID
        // char pnm[5];                        // product name
        // unsigned char prv_m : 4;            // Product revision least significant digit
        // unsigned char prv_n : 4;            // Product revision most significant digit
        // uint32_t psn;                       // Product serial number
        // unsigned char mdt_year_high : 4;    //  Manufacturing date year high digit
        // unsigned char mdt_month : 4;        // Manufacturing date month
        // unsigned char mdt_year_low : 4;     // Manufacturing date year low digit

		if (!cid.pnm[0]) cid.pnm[0] = ' ';
		if (!cid.pnm[1]) cid.pnm[1] = ' ';
		if (!cid.pnm[2]) cid.pnm[2] = ' ';
		if (!cid.pnm[3]) cid.pnm[3] = ' ';
		if (!cid.pnm[4]) cid.pnm[4] = ' ';

        display(0,"fileSystem: Mfr(0x%02x) OEM_ID(0x%02x,0x%02x) Product(%c%c%c%c%c) Version(%d.%d) SN(0x%08x)",
            cid.mid,
            cid.oid[0],cid.oid[1],
            cid.pnm[0],cid.pnm[1],cid.pnm[2],cid.pnm[3],cid.pnm[4],
            cid.prv_n,cid.prv_m,
            cid.psn);
        display(0,"fileSystem: Manufacture Date: %d-%d",
            (2000 + cid.mdt_year_low + 10 * cid.mdt_year_high),
            cid.mdt_month);
    #endif  // SHOW_CID

    #if 0   // SHOW_SIZE_DETAILS
        uint8_t fat_type = SD.fatType();
        uint32_t cluster_count = SD.clusterCount();
        int32_t free_cluster_count = SD.freeClusterCount();
        uint8_t blocks_per_cluster = SD.blocksPerCluster();
        uint32_t block_count = SD.volumeBlockCount();
        display(0,"fileSystem: fat_type(%d),clusters(%d) free(%d) blocks_per(%d) block_count(%d)",
            fat_type,
            cluster_count,
            free_cluster_count,
            blocks_per_cluster,
            block_count);
    #endif  // SHOW_SIZE_DETAILS

    #if  1  // SHOW_HUMAN_READABLE_SPACE_USED ('real' GB same as Windows)
        uint32_t gb_free = 10 * getFreeMB() / 1024;
        uint32_t gb_total = 10 * getTotalMB() / 1024;
        uint32_t gb_used = gb_total - gb_free;
        display(0,"FileSystem: %d.%d/%d.%dGB Used",
            gb_used/10,gb_used%10, gb_total/10, gb_total%10);
    #endif

    #if LIST_DIRECTORY_AT_STARTUP
        myFile_t root = SD.open("/");
        printDirectory(root);
        root.close();
    #endif

    return has_file_system;
}



//------------------------------------------------------------
// API
//------------------------------------------------------------

#if USE_OLD_FAT
	#define BYTES_PER_BLOCK  512
	#define BLOCKS_PER_MB   2048
#endif


uint64_t getFreeBytes()
{
	#if USE_OLD_FAT
		uint64_t cluster_count = SD.freeClusterCount();
		uint64_t blocks_per_cluster = SD.blocksPerCluster();
		return cluster_count * blocks_per_cluster * BYTES_PER_BLOCK;
	#else
		uint32_t cluster_count32 = SD.freeClusterCount();
		uint32_t bytes_per_cluster32 = SD.bytesPerCluster();
		delay(100);
		display(0,"free  count=%d  per=%d  shift=%d",cluster_count32,bytes_per_cluster32,SD.bytesPerClusterShift());
		uint64_t cluster_count = cluster_count32;
		uint64_t bytes_per_cluster = bytes_per_cluster32;
		uint64_t total_bytes = cluster_count * bytes_per_cluster;
		return total_bytes;
	#endif
}


uint32_t getFreeMB()
{
	#if USE_OLD_FAT
		uint32_t cluster_count = SD.freeClusterCount();
		uint32_t blocks_per_cluster = SD.blocksPerCluster();
		return (cluster_count * blocks_per_cluster) / BLOCKS_PER_MB;
	#else
		uint32_t cluster_count32 = SD.freeClusterCount();
		uint32_t bytes_per_cluster32 = SD.bytesPerCluster();
		delay(100);
		display(0,"free  count=%d  per=%d  shift=%d",cluster_count32,bytes_per_cluster32,SD.bytesPerClusterShift());
		uint64_t cluster_count = cluster_count32;
		uint64_t bytes_per_cluster = bytes_per_cluster32;
		uint64_t total_bytes = cluster_count * bytes_per_cluster;
		return total_bytes / BYTES_PER_MB;
	#endif
}



uint32_t getTotalMB()
	// NOWHERE is there an example of how to use these to determine the size of the file system.
	// bytesPerClusterShift() is really unclear.
	// On my 32GB card, cluserCount() returns 953948 and bytesPerCluster returns 32768.
	// Interestingly, multiplying these gives 31,258,968,064, which agrees with the
	// total bytes for the volume given by the Properties in Windows, without using
	// bytesPerClusterShift(), wtf ever that is.
{
	#if USE_OLD_FAT
		uint32_t cluster_count = SD.clusterCount();
		uint32_t blocks_per_cluster = SD.blocksPerCluster();
		return (cluster_count * blocks_per_cluster) / BLOCKS_PER_MB;
	#else
		uint32_t cluster_count32 = SD.clusterCount();
		uint32_t bytes_per_cluster32 = SD.bytesPerCluster();
		delay(100);
		display(0,"total count=%d  per=%d  shift=%d",cluster_count32,bytes_per_cluster32,SD.bytesPerClusterShift());
		uint64_t cluster_count = cluster_count32;
		uint64_t bytes_per_cluster = bytes_per_cluster32;
		uint64_t total_bytes = cluster_count * bytes_per_cluster;
		return total_bytes / BYTES_PER_MB;
	#endif
}


//---------------------------------------------
// utilities
//---------------------------------------------

const char *getTimeStamp(myFile_t *file)
{
    myDir_t dir_entry;
    static_timestamp[0] = 0;

    if (!file->dirEntry(&dir_entry))
    {
		char filename[255];
		file->getName(filename, sizeof(filename));
        my_error("Could not get dir_entry for %s",filename);
        return "";
    }

	#if USE_OLD_FAT 	// PRH PRH PRH - this needs to be reworked
		uint16_t year = FAT_YEAR(dir_entry.creationDate);
		uint16_t month = FAT_MONTH(dir_entry.creationDate);
		uint16_t day = FAT_DAY(dir_entry.creationDate);
		uint16_t hour = FAT_HOUR(dir_entry.creationTime);
		uint16_t minute = FAT_MINUTE(dir_entry.creationTime);
		uint16_t second = FAT_SECOND(dir_entry.creationTime);
		uint16_t hundredths = dir_entry.creationTimeTenths;
			// according to comment this actually hundredths ...
		if (hundredths >= 100)
		{
			hundredths -= 100;
			second += 1;
		}

		sprintf(static_timestamp,"%d-%02d-%02d %02d:%02d:%02d",
			year,month,day,hour,minute,second);
	#endif

    return static_timestamp;
}


const char *getTimeStamp(const char *path)
{
	myFile_t file = SD.open(path);
	if (!file)
	{
		my_error("Could not open %s to getTimeStamp",path);
		return "";
	}
	const char *rslt = getTimeStamp(&file);
	file.close();
	return rslt;
}


static int bufToNum(const char *ts, int offset, int length)
{
	char buf[6];
	memcpy(buf,&ts[offset],length);
	buf[length] = 0;
	return atoi(buf);
}


void setTimeStamp(myFile_t the_file, const char *ts)
{
    display_level(dbg_ts,4,"setTimeStamp(%04d,%02d,%02d,%02d,%02d,%02d)",
        bufToNum(ts,0,4),	// year
        bufToNum(ts,5,2),	// month
        bufToNum(ts,8,2),	// day
        bufToNum(ts,11,2),	// hour
        bufToNum(ts,14,2),	// minute
        bufToNum(ts,17,2));	// second)


    the_file.timestamp(
        T_ACCESS | T_CREATE | T_WRITE,
        bufToNum(ts,0,4),	// year
        bufToNum(ts,5,2),	// month
        bufToNum(ts,8,2),	// day
        bufToNum(ts,11,2),	// hour
        bufToNum(ts,14,2),	// minute
        bufToNum(ts,17,2));	// second)
}



static void dtCallback(uint16_t* date, uint16_t* time)
    // this call back function must be used on diretories
    // instead of setTimeStamp() above. It overuses the
    // global write_timestamp variable to hold the value for
    // the callback.
{
    char *ts = write_timestamp;

    char *year = &ts[0];
    ts[4] = 0;

    char *month = &ts[5];
    ts[7] = 0;

    char *day = &ts[8];
    ts[10] = 0;

    char *hour = &ts[11];
    ts[13] = 0;

    char *minute = &ts[14];
    ts[16] = 0;

    char *second = &ts[17];
    ts[19] = 0;

    display_level(dbg_ts+1,4,"dtCallback(%s,%s,%s,%s,%s,%s)",year,month,day,hour,minute,second);

    *date = FAT_DATE(atoi(year),atoi(month),atoi(day));
    *time = FAT_TIME(atoi(hour),atoi(minute),atoi(second));

}


bool mkDirTS(const char *path, const char *ts)
{
	strcpy(write_timestamp,ts);
	FatFile::dateTimeCallback(dtCallback);
	int rslt = SD.mkdir(path);
	FatFile::dateTimeCallbackCancel();
	return rslt;
}





// end of fileSystem.cpp
