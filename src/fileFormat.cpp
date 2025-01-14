//---------------------------------------------
// fileFormat.cpp
//---------------------------------------------
// http://www.sdcard.org/consumers/formatter/
// The source code files fileXXXX.cpp and h are the same in TE1 and TE2.
// They have NOT been made into a submodule yet, so must be manually normalized.

#include "fileSystem.h"
#include <myDebug.h>
// #include <sdios.h>

#define dbg_format 0
	// 0 = normal level

static uint32_t cardSizeBlocks;
static uint32_t cardCapacityMB;

static cache_t cache;
	// cache for SD block

// MBR information
static uint8_t partType;
static uint32_t relSector;
static uint32_t partSize;

// Fake disk geometry
static uint8_t numberOfHeads;
static uint8_t sectorsPerTrack;

// FAT parameters
static uint16_t reservedSectors;
static uint8_t sectorsPerCluster;
static uint32_t fatStart;
static uint32_t fatSize;
static uint32_t dataStart;

// constants for file system structure
static uint16_t const BU16 = 128;
static uint16_t const BU32 = 8192;

//  strings needed in file system structures
static char noName[] = "NO NAME    ";
static char fat16str[] = "FAT16   ";
static char fat32str[] = "FAT32   ";





extern void sdFormatDot();
extern void sdFormatMsg(const char *msg, bool err = false);
	// in winDialogs.cpp

static void _sdMessage(bool err, const char *format, va_list args)
{
	char buffer[255];
	vsprintf(buffer,format,args);
	if (err)
		my_error(buffer,0);
	else
		display(dbg_format,buffer,0);
	sdFormatMsg(buffer,err);
}
static void sdError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	_sdMessage(true,format,args);
}
static void sdMessage(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	_sdMessage(false,format,args);
}





static uint8_t writeCache(uint32_t lbn)
	// write cached block to the card
{
	return SD.card()->writeBlock(lbn, cache.data);
}


bool initSizes()
	// initialize appropriate sizes for SD capacity
{
	cardSizeBlocks = SD.card()->cardSize();

	// grr - the card I am testing with is 32GB but shows
	// 128GB as the card size, both here, and in Windows.
	// The only other working 32GB card I have shows 29.72GB
	// in Windows with 30435 MB when I make a new simple
	// volume on it.   So, weird, for safety, for the time
	// being, on the teensy with my version of SdFat, I am
	// assuming only SDHC cards (no SDXC) limited to 30435
	// blocks, regardless of what the csd shows.

	if (cardSizeBlocks > 30435 * 2048)
		cardSizeBlocks = 30435 * 2048;

	if (cardSizeBlocks == 0)
	{
		sdError("Bad SD Card Size(0)");
		return false;
	}
	cardCapacityMB = (cardSizeBlocks + 2047)/2048;
	display(dbg_format,"cardCapacityMB(%u)",cardCapacityMB);

	if (cardCapacityMB <= 6)
	{
		sectorsPerCluster = 0;
		sdError("SD Card of %lu MB is too small",cardCapacityMB);
		return false;

	} else if (cardCapacityMB <= 16) {
		sectorsPerCluster = 2;
	} else if (cardCapacityMB <= 32) {
		sectorsPerCluster = 4;
	} else if (cardCapacityMB <= 64) {
		sectorsPerCluster = 8;
	} else if (cardCapacityMB <= 128) {
		sectorsPerCluster = 16;
	} else if (cardCapacityMB <= 1024) {
		sectorsPerCluster = 32;
	} else if (cardCapacityMB <= 32768) {
		sectorsPerCluster = 64;
	} else {
		// SDXC cards
		sectorsPerCluster = 128;
	}

	sdMessage("Blocks/Cluster: %d",int(sectorsPerCluster));

  // set fake disk geometry

	sectorsPerTrack = cardCapacityMB <= 256 ? 32 : 63;

	if (cardCapacityMB <= 16) {
		numberOfHeads = 2;
	} else if (cardCapacityMB <= 32) {
		numberOfHeads = 4;
	} else if (cardCapacityMB <= 128) {
		numberOfHeads = 8;
	} else if (cardCapacityMB <= 504) {
		numberOfHeads = 16;
	} else if (cardCapacityMB <= 1008) {
		numberOfHeads = 32;
	} else if (cardCapacityMB <= 2016) {
		numberOfHeads = 64;
	} else if (cardCapacityMB <= 4032) {
		numberOfHeads = 128;
	} else {
		numberOfHeads = 255;
	}
	return true;
}


static void clearCache(uint8_t addSig)
	// zero cache and optionally set the sector signature
{
	memset(&cache, 0, sizeof(cache));
	if (addSig)
	{
		cache.mbr.mbrSig0 = BOOTSIG0;
		cache.mbr.mbrSig1 = BOOTSIG1;
	}
}



static bool clearFatDir(uint32_t bgn, uint32_t count)
	// zero FAT and root dir area on SD
{
	sdMessage("clearFatDir(%lu)",count);
	proc_entry();

	clearCache(false);
	for (uint32_t i = 0; i < count; i++)
	{
		if (!SD.card()->writeBlock(bgn + i, cache.data))
		{
			sdError("DIR writeBlock(%lu) failed",bgn+i);
			proc_leave();
			return false;
		}
		if ((i & 0XFF) == 0)
		{
			sdFormatDot();
			// display(dbg_format,"count(%lu)",i);;
		}
	}
	proc_leave();
	return true;
}


static uint16_t lbnToCylinder(uint32_t lbn)
	// return cylinder number for a logical block number
{
	return lbn / (numberOfHeads * sectorsPerTrack);
}


static uint8_t lbnToHead(uint32_t lbn)
	// return head number for a logical block number
{
	return (lbn % (numberOfHeads * sectorsPerTrack)) / sectorsPerTrack;
}


static uint8_t lbnToSector(uint32_t lbn)
	// return sector number for a logical block number
{
	return (lbn % sectorsPerTrack) + 1;
}


static bool writeMbr()
	// format and write the Master Boot Record
{
	sdMessage("writeMbr()");
	proc_entry();

	clearCache(true);
	part_t* p = cache.mbr.part;
	p->boot = 0;

	uint16_t c = lbnToCylinder(relSector);
	if (c > 1023)
	{
		sdError("MBR bad cylinder number(%d)",c);
		proc_leave();
		return false;
	}
	p->beginCylinderHigh = c >> 8;
	p->beginCylinderLow = c & 0XFF;
	p->beginHead = lbnToHead(relSector);
	p->beginSector = lbnToSector(relSector);
	p->type = partType;

	uint32_t endLbn = relSector + partSize - 1;
	c = lbnToCylinder(endLbn);
	if (c <= 1023)
	{
		p->endCylinderHigh = c >> 8;
		p->endCylinderLow = c & 0XFF;
		p->endHead = lbnToHead(endLbn);
		p->endSector = lbnToSector(endLbn);
	}
	else
	{
		// Too big flag, c = 1023, h = 254, s = 63
		p->endCylinderHigh = 3;
		p->endCylinderLow = 255;
		p->endHead = 254;
		p->endSector = 63;
	}
	p->firstSector = relSector;
	p->totalSectors = partSize;
	if (!writeCache(0))
	{
		sdError("MBR write failed");
		proc_leave();
		return false;
	}
	proc_leave();
	display(dbg_format,"writeMbr() done",0);
	return true;
}



uint32_t volSerialNumber()
	// generate serial number from card size and micros since boot
{
	return (cardSizeBlocks << 8) + micros();
}



static bool makeFat16()
	// format the SD as FAT16
{
	display(dbg_format,"makeFat16()",0);
	proc_entry();

	uint32_t nc;
	for (dataStart = 2 * BU16;; dataStart += BU16)
	{
		nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
		fatSize = (nc + 2 + 255)/256;
		uint32_t r = BU16 + 1 + 2 * fatSize + 32;
		if (dataStart < r)
		{
			continue;
		}
		relSector = dataStart - r + BU16;
		break;
	}

	// check valid cluster count for FAT16 volume
	if (nc < 4085 || nc >= 65525)
	{
		sdError("FAT16 bad cluster count(%lu)",nc);
		proc_leave();
		return false;
	}
	reservedSectors = 1;
	fatStart = relSector + reservedSectors;
	partSize = nc * sectorsPerCluster + 2 * fatSize + reservedSectors + 32;

	if (partSize < 32680) {
		partType = 0X01;
	} else if (partSize < 65536) {
		partType = 0X04;
	} else {
		partType = 0X06;
	}
	// write MBR

	if (!writeMbr())
	{
		proc_leave();
		return false;
	}

	clearCache(true);
	fat_boot_t* pb = &cache.fbs;
	pb->jump[0] = 0XEB;
	pb->jump[1] = 0X00;
	pb->jump[2] = 0X90;
	for (uint8_t i = 0; i < sizeof(pb->oemId); i++)
	{
		pb->oemId[i] = ' ';
	}

	pb->bytesPerSector = 512;
	pb->sectorsPerCluster = sectorsPerCluster;
	pb->reservedSectorCount = reservedSectors;
	pb->fatCount = 2;
	pb->rootDirEntryCount = 512;
	pb->mediaType = 0XF8;
	pb->sectorsPerFat16 = fatSize;
	pb->sectorsPerTrack = sectorsPerTrack;
	pb->headCount = numberOfHeads;
	pb->hidddenSectors = relSector;
	pb->totalSectors32 = partSize;
	pb->driveNumber = 0X80;
	pb->bootSignature = EXTENDED_BOOT_SIG;
	pb->volumeSerialNumber = volSerialNumber();
	memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
	memcpy(pb->fileSystemType, fat16str, sizeof(pb->fileSystemType));

	// write partition boot sector

	sdMessage("write partition boot sector");
	if (!writeCache(relSector))
	{
		sdError("FAT16 write Boot Sector failed");
		proc_leave();
		return false;
	}

	// clear FAT and root directory

	if (!clearFatDir(fatStart, dataStart - fatStart))
	{
		proc_leave();
		return false;
	}

	clearCache(false);
	cache.fat16[0] = 0XFFF8;
	cache.fat16[1] = 0XFFFF;

	// write first block of FAT and backup for reserved clusters

	sdMessage("write first block of FAT ");
	if (!writeCache(fatStart) ||
		!writeCache(fatStart + fatSize))
	{
		sdError("FAT16 write failed");
		proc_leave();
		return false;
	}

	proc_leave();
	display(dbg_format,"makeFat16() done",0);
	return true;
}


static bool makeFat32()
	// format the SD as FAT32
{
	display(dbg_format,"makeFat32()",0);
	proc_entry();

	uint32_t nc;
	relSector = BU32;
	for (dataStart = 2 * BU32;; dataStart += BU32)
	{
		nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
		fatSize = (nc + 2 + 127)/128;
		uint32_t r = relSector + 9 + 2 * fatSize;
		if (dataStart >= r)
		{
			break;
		}
	}

	// error if too few clusters in FAT32 volume

	if (nc < 65525)
	{
		sdError("FAT32 bad cluster count(%lu)",nc);
		proc_leave();
		return false;
	}

	reservedSectors = dataStart - relSector - 2 * fatSize;
	fatStart = relSector + reservedSectors;
	partSize = nc * sectorsPerCluster + dataStart - relSector;

	// type depends on address of end sector
	// max CHS has lbn = 16450560 = 1024*255*63

	if ((relSector + partSize) <= 16450560) {
		// FAT32
		partType = 0X0B;
	} else {
		// FAT32 with INT 13
		partType = 0X0C;
	}

	if (!writeMbr())
	{
		proc_leave();
		return false;
	}

	clearCache(true);

	fat32_boot_t* pb = &cache.fbs32;
	pb->jump[0] = 0XEB;
	pb->jump[1] = 0X00;
	pb->jump[2] = 0X90;
	for (uint8_t i = 0; i < sizeof(pb->oemId); i++)
	{
		pb->oemId[i] = ' ';
	}
	pb->bytesPerSector = 512;
	pb->sectorsPerCluster = sectorsPerCluster;
	pb->reservedSectorCount = reservedSectors;
	pb->fatCount = 2;
	pb->mediaType = 0XF8;
	pb->sectorsPerTrack = sectorsPerTrack;
	pb->headCount = numberOfHeads;
	pb->hidddenSectors = relSector;
	pb->totalSectors32 = partSize;
	pb->sectorsPerFat32 = fatSize;
	pb->fat32RootCluster = 2;
	pb->fat32FSInfo = 1;
	pb->fat32BackBootBlock = 6;
	pb->driveNumber = 0X80;
	pb->bootSignature = EXTENDED_BOOT_SIG;
	pb->volumeSerialNumber = volSerialNumber();
	memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
	memcpy(pb->fileSystemType, fat32str, sizeof(pb->fileSystemType));

	// write partition boot sector and backup

	sdMessage("write partition boot sector");
	if (!writeCache(relSector) ||
		!writeCache(relSector + 6))
	{
		sdError("FAT32 write Boot Sector failed");
		proc_leave();
		return false;
	}

	clearCache(true);

	// write extra boot area and backup

	sdMessage("write extra boot area");
	if (!writeCache(relSector + 2) || !writeCache(relSector + 8))
	{
		sdError("FAT32 write extra boot area failed");
		proc_leave();
		return false;
	}

	fat32_fsinfo_t* pf = &cache.fsinfo;
	pf->leadSignature = FSINFO_LEAD_SIG;
	pf->structSignature = FSINFO_STRUCT_SIG;
	pf->freeCount = 0XFFFFFFFF;
	pf->nextFree = 0XFFFFFFFF;

	// write FSINFO sector and backup

	sdMessage("write FSINFO sector");
	if (!writeCache(relSector + 1) ||
		!writeCache(relSector + 7))
	{
		sdError("FAT32 write FSINFO failed");
		proc_leave();
		return false;
	}

	// clear FAT and root directory

	if (!clearFatDir(fatStart, 2 * fatSize + sectorsPerCluster))
	{
		proc_leave();
		return false;
	}

	clearCache(false);
	cache.fat32[0] = 0x0FFFFFF8;
	cache.fat32[1] = 0x0FFFFFFF;
	cache.fat32[2] = 0x0FFFFFFF;

	// write first block of FAT and backup for reserved clusters

	sdMessage("write first block of FAT");
	if (!writeCache(fatStart) ||
		!writeCache(fatStart + fatSize))
	{
		sdError("FAT32 write failed");
		proc_leave();
		return false;
	}

	proc_leave();
	display(dbg_format,"makeFat32() done",0);
	return true;
}


bool formatCard()
{
	display(dbg_format,"formatCard()",0);
	proc_entry();

	if (!initSizes())
	{
		proc_leave();
		return false;
	}

	if (SD.card()->type() != SD_CARD_TYPE_SDHC)
	{
		if (!makeFat16())
		{
			proc_leave();
			return false;
		}
	}
	else
	{
		if (!makeFat32())
		{
			proc_leave();
			return false;
		}
	}

	proc_leave();
	sdMessage("SD Card Format Done!!",0);
	return true;
}





uint32_t const ERASE_SIZE = 262144L;



bool eraseCard()
	// flash erase all data
{
	sdMessage("eraseCard()");
	proc_entry();

	if (!initSizes())
	{
		proc_leave();
		return false;
	}

	uint32_t firstBlock = 0;
	uint32_t lastBlock;
	uint16_t n = 0;

	do
	{
		lastBlock = firstBlock + ERASE_SIZE - 1;
		if (lastBlock >= cardSizeBlocks)
		{
			lastBlock = cardSizeBlocks - 1;
		}
		if (!SD.card()->erase(firstBlock, lastBlock))
		{
			sdError("eraseCard failed at firstBlock(%lu)",firstBlock);
			proc_leave();
			return false;
		}
		if ((n++)%32 == 31)
		{
			sdFormatDot();
			// display(dbg_format,"eraseCard(%d)",n);
		}
		firstBlock += ERASE_SIZE;

	} while (firstBlock < cardSizeBlocks);

	if (!SD.card()->readBlock(0, cache.data))
	{
		sdError("eraseCard failed to read block(0)");
		proc_leave();
		return false;
	}

	proc_leave();
	sdMessage("eraseCard() done");
	return formatCard();
}
