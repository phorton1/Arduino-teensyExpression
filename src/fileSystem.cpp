//----------------------------------------------------------
// fileSystem.cpp
//----------------------------------------------------------

#include "fileSystem.h"
#include "prefs.h"
#include <myDebug.h>

#define dbg_ts    0
	// 0 = show timestamp operations
	// -1 = show callback setting


#define SHOW_CARD_INFO		1
#define LIST_CARD_CONTENTS	1
#define TEST_PATH_TS		0
#define TEST_SET_FILE_TS	0
#define TEST_MKDIR_TS		0



char write_timestamp[32];
char static_timestamp[32];
	// buffers for use by getTimeStamp and dtCallback

bool has_file_system;
bool has_sd_card;

bool hasFileSystem() { return has_file_system; }
bool hasSDCard()     { return has_sd_card; }


#define BYTES_PER_BLOCK  512
#define BLOCKS_PER_MB   2048




#if LIST_CARD_CONTENTS

	void showDirectory(const char *dir_name, File dir)
	{
		const char *dir_ts = getTimeStamp(&dir);
		display(0,"/%-40s  ts=%s",dir_name,dir_ts);
		proc_entry();

		while(true)
		{
			File entry = dir.openNextFile();
			if (!entry)
				break;
			const char *name = entry.name();

			if (entry.isDirectory())
			{
				showDirectory(name,entry);
			}
			else
			{
				uint32_t size = entry.size();
				const char *ts = getTimeStamp(&entry);
				display(0,"%-30s %-10d  ts=%s",name,size,ts);
			}
			entry.close();
		}
		proc_leave();
	}

#endif


#if SHOW_CARD_INFO

	const char *cardType(Sd2Card &card)
	{
		switch(card.type()) {
			case SD_CARD_TYPE_SD1: return "SD1";
			case SD_CARD_TYPE_SD2: return "SD2";
			case SD_CARD_TYPE_SDHC: return "SDHC";
		}
		return "Unknown";
	}

#endif







bool initFileSystem()
{
	display(0,"initFileSystem() started",0);

	has_file_system = SD.begin(BUILTIN_SDCARD);

	if (!has_file_system)
		my_error("Could not initialize SD card",0);
	else
	{
		#if SHOW_CARD_INFO
			Sd2Card card;
			// The unitialized "card" variable somehow magically works here
			display(0,"CARD TYPE: %s",cardType(card));
			proc_entry();

			SdVolume volume;
			bool has_volume;
			has_volume = volume.init(card);
			if (has_volume)
			{
				display(0,"FAT%d",volume.fatType());

				// SD card blocks are "always 512 bytes"
				// so to convert to megs we divide by 2 * 1024 = 2048
				// bug - in all cases for some reason total and free are coming out the same

				uint32_t blocks_per_cluster = volume.blocksPerCluster();
				uint32_t total_clusters = volume.clusterCount();
				uint32_t free_clusters = SD.sdfs.freeClusterCount();

				// the only "freeClusterCount()" I couild find was on the Sd.sdfs sub-object

				uint32_t total_megs = (total_clusters * blocks_per_cluster) / BLOCKS_PER_MB;
				uint32_t free_megs = (free_clusters * blocks_per_cluster) / BLOCKS_PER_MB;

				display(0,"Total: %d MB",total_megs);
				display(0,"Free:  %d MB",free_megs);

				// I have more important things to do with my time that to try to
				// sort out API's to 50 year old file systems.  I will use the
				// simplest "SD" and "File" API until it hurts.
			}
			else
				my_error("Could not initialize/get SDCard volume",0);

			proc_leave();

		#endif

		#if LIST_CARD_CONTENTS
			display(0,"LISTING SD CARD",0);
			proc_entry();
			File root = SD.open("/");
			showDirectory("ROOT",root);
			proc_leave();
		#endif




		#if TEST_PATH_TS
			const char *test_filename = "/data/songs/BigRiver.song";
			const char *test_ts = getTimeStamp(test_filename);
			display(0,"TEST_PATH_TS(%s)=%s",test_filename,test_ts);
		#endif

		#if TEST_SET_FILE_TS
			const char *test_filename2 = "/data/songs/BigRiver.song";
			char save_ts[30];
			// strcpy(save_ts,"2020-10-13 20:18:20");
			strcpy(save_ts,getTimeStamp(test_filename2));
			const char *test_ts2 = "2024-03-08 12:23:56";
			File test_file = SD.open(test_filename2);
			if (!test_file)
				my_error("Could not open TEST_SET_FILE_TS(%s)",test_filename2);
			else
			{
				display(0,"TEST_SET_FILE_TS(%s)=%s",test_filename2,test_ts2);
				setTimeStamp(&test_file, test_ts2);
				test_file.close();
				display(0,"TEST_SET_FILE_READBACK(%s)=%s",test_filename2,getTimeStamp(test_filename));
				File test_file2 = SD.open(test_filename2);
				if (!test_file2)
					my_error("Could not re-open TEST_SET_FILE_TS(%s)",test_filename2);
				else
				{
					display(0,"RESETTING TIMESTAMP to %s",save_ts);
					setTimeStamp(&test_file2, save_ts);
					test_file2.close();
				}
			}
		#endif 	// TEST_SET_FILE_TS

		#if TEST_MKDIR_TS
			const char *path_ts = "2025-03-07 01:02:03";
			const char *test_path = "/junk";
			if (SD.exists(test_path))
			{
				display(0,"removing existing mkdir path(%s)",test_path);
				SD.rmdir(test_path);
			}
			mkDirTS(test_path,path_ts);
		#endif 	// TEST_MKDIR_TS

	}

	display(0,"initFileSystem() returning %d",has_file_system);
	return has_file_system;
}




const char *getTimeStamp(File *file)
{
	static_timestamp[0] = 0;
	DateTimeFields tm;
	if (file->getModifyTime(tm))
	{
		sprintf(static_timestamp,"%04d-%02d-%02d %02d:%02d:%02d",
			tm.year + 1900,
			tm.mon + 1,
			tm.mday,
			tm.hour,
			tm.min,
			tm.sec);
	}
	return static_timestamp;
}


const char *getTimeStamp(const char *path)
{
	File file = SD.open(path);
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


void setTimeStamp(File *file, const char *ts)
{
    int year = bufToNum(ts,0,4);	// year
    int month = bufToNum(ts,5,2);	// month
	int day = bufToNum(ts,8,2);		// day
    int hour = bufToNum(ts,11,2);	// hour
    int minute = bufToNum(ts,14,2);	// minute
    int second = bufToNum(ts,17,2);	// second)

    display_level(dbg_ts,4,"setTimeStamp(%s) = (%04d,%02d,%02d,%02d,%02d,%02d)",
		ts,
		year,
		month,
		day,
		hour,
		minute,
		second);

	DateTimeFields tm;
	tm.year = year - 1900;
	tm.mon = month - 1;
	tm.mday = day;
	tm.hour = hour;
	tm.min = minute;
	tm.sec = second;

	file->setModifyTime(tm);
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

	display_level(dbg_ts,4,"dtCallback(%s,%s,%s,%s,%s,%s)",year,month,day,hour,minute,second);

	*date = FAT_DATE(atoi(year),atoi(month),atoi(day));
	*time = FAT_TIME(atoi(hour),atoi(minute),atoi(second));
}

bool mkDirTS(const char *path, const char *ts)
{
	strcpy(write_timestamp,ts);
	SdFile::dateTimeCallback(dtCallback);
	int rslt = SD.mkdir(path);
	SdFile::dateTimeCallbackCancel();
	return rslt;
}


// end of fileSystem.cpp
