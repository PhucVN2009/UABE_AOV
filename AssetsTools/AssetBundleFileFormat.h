#pragma once
#ifndef __AssetsTools__AssetBundleFormat_Header
#define __AssetsTools__AssetBundleFormat_Header
#include "defines.h"
#include "BundleReplacer.h"
#include "ClassDatabaseFile.h"

// ============================================================
// UABE_AOV: Arena of Valor (Lien Quan Mobile) AssetBundle patch
// Unity 2022.3.5f1 / UnityFS 5.x format with:
//   - AES-128-CBC encrypted 16-byte header  (flag 0x200)
//   - BlockInfo order:  flags(2)+unknown(2)+compressedSize(4)+decompressedSize(4)  [12 bytes]
//   - DirectoryInfo order: size(8)+offset(8)+flags(4)+name  (size/offset swapped vs standard)
// ============================================================
#define AOV_BUNDLE_ENCRYPT_FLAG  0x200u

// AES-128-CBC key/IV used by AOV to encrypt the 16-byte header block
static const uint8_t kAovHeaderAESKey[16] = {
    0xE3,0x05,0x62,0x14, 0xD6,0x0A,0x20,0x25,
    0x36,0x96,0x1B,0x07, 0x74,0xDC,0x24,0x02
};
static const uint8_t kAovHeaderAESIV[16] = {
    0x1D,0x6E,0xEB,0x4C, 0x86,0xA9,0x45,0x44,
    0x45,0x72,0x12,0x21, 0x2B,0x43,0x25,0x2F
};

class AssetBundleFile;
struct AssetBundleHeader06;
struct AssetBundleHeader03;
struct AssetBundleEntry;
struct AssetBundleList;

struct AssetBundleDirectoryInfo06
{
	QWORD offset;
	QWORD decompressedSize;
	uint32_t flags; //(flags & 4) : has serialized data
	const char *name;
	ASSETSTOOLS_API QWORD GetAbsolutePos(AssetBundleHeader06 *pHeader);
	ASSETSTOOLS_API QWORD GetAbsolutePos(class AssetBundleFile *pFile);
};
struct AssetBundleBlockInfo06
{
	uint32_t decompressedSize;
	uint32_t compressedSize;
	uint16_t flags; //(flags & 0x3F) : compression; (flags & 0x40) : streamed
	uint16_t _aov_unknown; // AOV: extra 2 bytes after flags (normally 0x0000)
	inline uint8_t GetCompressionType() { return (uint8_t)(flags & 0x3F); }
};
struct AssetBundleBlockAndDirectoryList06
{
	QWORD checksumLow;
	QWORD checksumHigh;
	uint32_t blockCount;
	AssetBundleBlockInfo06 *blockInf;
	uint32_t directoryCount;
	AssetBundleDirectoryInfo06 *dirInf;
	
	ASSETSTOOLS_API void Free();
	ASSETSTOOLS_API bool Read(QWORD filePos, IAssetsReader *pReader, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Write(IAssetsWriter *pWriter, QWORD &curFilePos, AssetsFileVerifyLogger errorLogger = NULL);
};

#define LargestBundleHeader AssetBundleHeader03
struct AssetBundleHeader06
{
	char signature[13];
	uint32_t fileVersion;
	char minPlayerVersion[24];
	char fileEngineVersion[64];
	QWORD totalFileSize;
	uint32_t compressedSize;
	uint32_t decompressedSize;
	// (flags & 0x3F)  compression mode
	// (flags & 0x40)  directory info present
	// (flags & 0x80)  block+dir list at end of file
	// (flags & 0x200) AOV: header 16-bytes are AES-128-CBC encrypted
	uint32_t flags;
	
	ASSETSTOOLS_API bool ReadInitial(IAssetsReader *pReader, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Read(IAssetsReader *pReader, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Write(IAssetsWriter *pWriter, QWORD &curFilePos, AssetsFileVerifyLogger errorLogger = NULL);
	inline QWORD GetBundleInfoOffset()
	{
		if (this->flags & 0x80)
		{
			if (this->totalFileSize == 0)
				return -1;
			return this->totalFileSize - this->compressedSize;
		}
		else
		{
			QWORD ret = strlen(minPlayerVersion) + strlen(fileEngineVersion) + 0x1A;
			if (this->flags & 0x100)
				ret = (ret + 0x0A);
			else
				ret = (ret + strlen(signature) + 1);
			if (this->fileVersion >= 7)
				ret = (ret + 15) & ~15;
			return ret;
		}
	}
	inline QWORD GetFileDataOffset()
	{
		QWORD ret = 0;
		if (!strcmp(this->signature, "UnityArchive"))
			return this->compressedSize;
		if (!strcmp(this->signature, "UnityFS") || !strcmp(this->signature, "UnityWeb"))
		{
			ret = (QWORD)strlen(minPlayerVersion) + (QWORD)strlen(fileEngineVersion) + 0x1A;
			if (this->flags & 0x100)
				ret += 0x0A;
			else
				ret += (QWORD)strlen(signature) + 1;
			if (this->fileVersion >= 7)
				ret = (ret + 15) & ~15;
		}
		if (!(this->flags & 0x80))
		{
			ret += this->compressedSize;
			if (this->flags & 0x200)
				ret = (ret + 15) & ~15;
		}
		return ret;
	}
};

struct AssetBundleHeader03
{
	char signature[13];
	uint32_t fileVersion;
	char minPlayerVersion[24];
	char fileEngineVersion[64];
	uint32_t minimumStreamedBytes;
	uint32_t bundleDataOffs;
	uint32_t numberOfAssetsToDownload;
	uint32_t blockCount;
	struct AssetBundleOffsetPair *pBlockList;
	uint32_t fileSize2;
	uint32_t unknown2;
	uint8_t unknown3;

	ASSETSTOOLS_API bool Read(IAssetsReader *pReader, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Write(IAssetsWriter *pWriter, QWORD &curFilePos, AssetsFileVerifyLogger errorLogger = NULL);
};

struct AssetBundleEntry
{
	uint32_t offset;
	uint32_t length;
	char name[1];
	ASSETSTOOLS_API unsigned int GetAbsolutePos(AssetBundleHeader03 *pHeader);
	ASSETSTOOLS_API unsigned int GetAbsolutePos(class AssetBundleFile *pFile);
};
struct AssetsList
{
	uint32_t pos;
	uint32_t count;
	AssetBundleEntry **ppEntries;
	uint32_t allocatedCount;
	ASSETSTOOLS_API void Free();
	ASSETSTOOLS_API bool Read(IAssetsReader *pReader, QWORD &curFilePos, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Write(IAssetsWriter *pWriter, QWORD &curFilePos, AssetsFileVerifyLogger errorLogger = NULL);
	ASSETSTOOLS_API bool Write(IAssetsReader *pReader, 
		IAssetsWriter *pWriter, bool doWriteAssets, QWORD &curReadPos, QWORD *curWritePos = NULL,
		AssetsFileVerifyLogger errorLogger = NULL);
};
struct AssetBundleOffsetPair
{
	uint32_t compressed;
	uint32_t uncompressed;
};
enum ECompressionTypes
{
	COMPRESS_NONE,
	COMPRESS_LZMA,
	COMPRESS_LZ4,
	COMPRESS_MAX
};
class AssetBundleFile
{
	public:
		union {
			AssetBundleHeader03 bundleHeader3;
			AssetBundleHeader06 bundleHeader6;
		};
		union {
			AssetsList *assetsLists3;
			AssetBundleBlockAndDirectoryList06 *bundleInf6;
		};

		ASSETSTOOLS_API AssetBundleFile();
		ASSETSTOOLS_API ~AssetBundleFile();
		ASSETSTOOLS_API void Close();
		ASSETSTOOLS_API bool Read(IAssetsReader *pReader, AssetsFileVerifyLogger errorLogger = NULL, bool allowCompressed = false, uint32_t maxDirectoryLen = 16*1024*1024);
		ASSETSTOOLS_API bool IsCompressed();
		ASSETSTOOLS_API bool Write(IAssetsReader *pReader,
			IAssetsWriter *pWriter,
			class BundleReplacer **pReplacers, size_t replacerCount, 
			AssetsFileVerifyLogger errorLogger = NULL, ClassDatabaseFile *typeMeta = NULL);
		ASSETSTOOLS_API bool Unpack(IAssetsReader *pReader, IAssetsWriter *pWriter);
		ASSETSTOOLS_API bool Pack(IAssetsReader *pReader, IAssetsWriter *pWriter, ECompressionTypes *settings = NULL, ECompressionTypes fileTableCompression = COMPRESS_LZ4);
		ASSETSTOOLS_API bool IsAssetsFile(IAssetsReader *pReader, AssetBundleDirectoryInfo06 *pEntry);
		ASSETSTOOLS_API bool IsAssetsFile(IAssetsReader *pReader, AssetBundleEntry *pEntry);
		ASSETSTOOLS_API bool IsAssetsFile(IAssetsReader *pReader, size_t entryIdx);
		inline const char *GetEntryName(size_t entryIdx)
		{
			if (bundleHeader6.fileVersion >= 6)
 				return (bundleInf6 == nullptr || bundleInf6->directoryCount <= entryIdx) ? nullptr : bundleInf6->dirInf[entryIdx].name;
			else if (bundleHeader6.fileVersion == 3)
 				return (assetsLists3 == nullptr || assetsLists3->count <= entryIdx) ? nullptr : assetsLists3->ppEntries[entryIdx]->name;
			return nullptr;
		}
		ASSETSTOOLS_API IAssetsReader *MakeAssetsFileReader(IAssetsReader *pReader, AssetBundleDirectoryInfo06 *pEntry);
		ASSETSTOOLS_API IAssetsReader *MakeAssetsFileReader(IAssetsReader *pReader, AssetBundleEntry *pEntry);
};
ASSETSTOOLS_API void FreeAssetBundle_FileReader(IAssetsReader *pReader);

#endif
