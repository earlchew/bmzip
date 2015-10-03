// bmzip: program for compressing and decompressing the bootmgr file
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "GetBootmanagerVersion.h"

// Modified from PEDataTypes.h, PEFile.cpp, PEVersion.h, and PEVersion.cpp in pe-file project: https://github.com/coderforlife/pe-file/
// Only reads the version resource from the mini PE file in the bootmgr

#ifdef _MSC_VER
#pragma warning(disable : 4201 4480) // nonstandard extension used: [nameless struct/union | specifying underlying type for enum]
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1500
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#include <wchar.h>
#define wcscmp wcscmp_
#define wcslen wcslen_
static int wcscmp_(const wchar_t *lhs, const wchar_t *rhs) {
    while (*lhs == *rhs) {
        if (*lhs) return 0;
        ++lhs, ++rhs;
    }
    return *lhs < *rhs ? -1 : +1;
}
static size_t wcslen_(const wchar_t *wcs) {
    size_t len = 0;
    while (wcs[len])
        ++len;
    return len;
}
#endif

#include <stdlib.h>
#include <string.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) sizeof(a)/sizeof(a[0])
#endif

static_assert(sizeof(wchar_t) == 2, "16 bit wchar_t required");

template<unsigned int MULT> inline static size_t roundUpTo(size_t x) { size_t mod = x % MULT; return (mod == 0) ? x : (x + MULT - mod); }
template<> inline size_t roundUpTo<2>(size_t x) { return (x + 1) & ~0x1; }
template<> inline size_t roundUpTo<4>(size_t x) { return (x + 3) & ~0x3; }

#ifdef _MSC_VER
#pragma region Resource Finding Structures and Code
#include "pshpack2.h"
#endif
#ifdef __GNUC__
#pragma pack(push,2)
#endif
struct DOSHeader {      // IMAGE_DOS_HEADER - DOS .EXE header
	const static uint16_t SIGNATURE = 0x5A4D; // MZ

	uint16_t e_magic;    // Magic number
	uint16_t e_cblp;     // Bytes on last page of file
	uint16_t e_cp;       // Pages in file
	uint16_t e_crlc;     // Relocations
	uint16_t e_cparhdr;  // Size of header in paragraphs
	uint16_t e_minalloc; // Minimum extra paragraphs needed
	uint16_t e_maxalloc; // Maximum extra paragraphs needed
	uint16_t e_ss;       // Initial (relative) SS value
	uint16_t e_sp;       // Initial SP value
	uint16_t e_csum;     // Checksum
	uint16_t e_ip;       // Initial IP value
	uint16_t e_cs;       // Initial (relative) CS value
	uint16_t e_lfarlc;   // File address of relocation table
	uint16_t e_ovno;     // Overlay number
	uint16_t e_res[4];   // Reserved words
	uint16_t e_oemid;    // OEM identifier (for e_oeminfo)
	uint16_t e_oeminfo;  // OEM information; e_oemid specific
	uint16_t e_res2[10]; // Reserved words
	int32_t  e_lfanew;   // File address of new exe header
};
#ifdef __GNUC__
#pragma pack(pop)
#endif
#ifdef _MSC_VER
#include "poppack.h"
#endif
struct FileHeader { // IMAGE_FILE_HEADER
	enum CharacteristicFlags : uint16_t { // IMAGE_FILE_* - FLAGS
		RELOCS_STRIPPED           = 0x0001, // Relocation info stripped from file.
		EXECUTABLE_IMAGE          = 0x0002, // File is executable  (i.e. no unresolved external references).
		LINE_NUMS_STRIPPED        = 0x0004, // Line numbers stripped from file.
		LOCAL_SYMS_STRIPPED       = 0x0008, // Local symbols stripped from file.
		AGGRESIVE_WS_TRIM         = 0x0010, // Aggressively trim working set
		LARGE_ADDRESS_AWARE       = 0x0020, // App can handle >2gb addresses
		BYTES_REVERSED_LO         = 0x0080, // Bytes of machine word are reversed.
		MACHINE_32BIT             = 0x0100, // 32 bit word machine.
		DEBUG_STRIPPED            = 0x0200, // Debugging info stripped from file in .DBG file
		REMOVABLE_RUN_FROM_SWAP   = 0x0400, // If Image is on removable media, copy and run from the swap file.
		NET_RUN_FROM_SWAP         = 0x0800, // If Image is on Net, copy and run from the swap file.
		SYSTEM                    = 0x1000, // System File.
		DLL                       = 0x2000, // File is a DLL.
		UP_SYSTEM_ONLY            = 0x4000, // File should only be run on a UP machine
		BYTES_REVERSED_HI         = 0x8000, // Bytes of machine word are reversed.
	};
	enum MachineType : uint16_t { // IMAGE_FILE_MACHINE_*
		Unknown   = 0,
		I386      = 0x014c, // Intel 386.
		R3000     = 0x0162, // MIPS little-endian, 0x160 big-endian
		R4000     = 0x0166, // MIPS little-endian
		R10000    = 0x0168, // MIPS little-endian
		WCEMIPSV2 = 0x0169, // MIPS little-endian WCE v2
		ALPHA     = 0x0184, // Alpha_AXP
		SH3       = 0x01a2, // SH3 little-endian
		SH3DSP    = 0x01a3,
		SH3E      = 0x01a4, // SH3E little-endian
		SH4       = 0x01a6, // SH4 little-endian
		SH5       = 0x01a8, // SH5
		ARM       = 0x01c0, // ARM Little-Endian
		THUMB     = 0x01c2,
		AM33      = 0x01d3,
		POWERPC   = 0x01F0, // IBM PowerPC Little-Endian
		POWERPCFP = 0x01f1,
		IA64      = 0x0200, // Intel 64
		MIPS16    = 0x0266, // MIPS
		ALPHA64   = 0x0284, // ALPHA64
		MIPSFPU   = 0x0366, // MIPS
		MIPSFPU16 = 0x0466, // MIPS
		AXP64     = ALPHA64,
		TRICORE   = 0x0520, // Infineon
		CEF       = 0x0CEF,
		EBC       = 0x0EBC, // EFI Byte Code
		AMD64     = 0x8664, // AMD64 (K8)
		M32R      = 0x9041, // M32R little-endian
		CEE       = 0xC0EE,
	};

	MachineType Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	CharacteristicFlags Characteristics;
};
struct OptionalHeader { // IMAGE_OPTIONAL_HEADER_*
	static const uint16_t SIGNATURE32 = 0x10b;
	static const uint16_t SIGNATURE64 = 0x20b;

	uint16_t Magic;
	uint8_t  MajorLinkerVersion;
	uint8_t  MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	union {
		struct
		{
			uint32_t BaseOfData;
			uint32_t ImageBase32;
		};
		uint64_t ImageBase64;
	};
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
};
struct NTHeaders { // IMAGE_NT_HEADERS_*
	static const uint32_t SIGNATURE = 0x00004550; // PE00
	uint32_t Signature;
	struct FileHeader FileHeader;
	struct OptionalHeader OptionalHeader;
};
struct SectionHeader { // IMAGE_SECTION_HEADER
	enum CharacteristicFlags : uint32_t { // IMAGE_SCN_* - FLAGS
		//Reserved: TYPE_REG          = 0x00000000,
		//Reserved: TYPE_DSECT        = 0x00000001,
		//Reserved: TYPE_NOLOAD       = 0x00000002,
		//Reserved: TYPE_GROUP        = 0x00000004,
		TYPE_NO_PAD                   = 0x00000008,
		//Reserved: TYPE_COPY         = 0x00000010,

		CNT_CODE                   = 0x00000020, // Section contains code.
		CNT_INITIALIZED_DATA       = 0x00000040, // Section contains initialized data.
		CNT_UNINITIALIZED_DATA     = 0x00000080, // Section contains uninitialized data.

		LNK_OTHER                  = 0x00000100,
		LNK_INFO                   = 0x00000200, // Section contains comments or some other type of information.
		//Reserved: TYPE_OVER      = 0x00000400,
		LNK_REMOVE                 = 0x00000800, // Section contents will not become part of image.
		LNK_COMDAT                 = 0x00001000, // Section contents comdat.

		//Reserved:                = 0x00002000,
		//Obsolete: MEM_PROTECTED  = 0x00004000,
		NO_DEFER_SPEC_EXC          = 0x00004000, // Reset speculative exceptions handling bits in the TLB entries for this section.
		GPREL                      = 0x00008000, // Section content can be accessed relative to GP
		MEM_FARDATA                = 0x00008000,
		//Obsolete: MEM_SYSHEAP    = 0x00010000,
		MEM_PURGEABLE              = 0x00020000,
		MEM_16BIT                  = 0x00020000,
		MEM_LOCKED                 = 0x00040000,
		MEM_PRELOAD                = 0x00080000,

		ALIGN_1BYTES               = 0x00100000,
		ALIGN_2BYTES               = 0x00200000,
		ALIGN_4BYTES               = 0x00300000,
		ALIGN_8BYTES               = 0x00400000,
		ALIGN_16BYTES              = 0x00500000, // Default alignment if no others are specified.
		ALIGN_32BYTES              = 0x00600000,
		ALIGN_64BYTES              = 0x00700000,
		ALIGN_128BYTES             = 0x00800000,
		ALIGN_256BYTES             = 0x00900000,
		ALIGN_512BYTES             = 0x00A00000,
		ALIGN_1024BYTES            = 0x00B00000,
		ALIGN_2048BYTES            = 0x00C00000,
		ALIGN_4096BYTES            = 0x00D00000,
		ALIGN_8192BYTES            = 0x00E00000,
		//Unused:                  = 0x00F00000,
		ALIGN_MASK                 = 0x00F00000,

		LNK_NRELOC_OVFL            = 0x01000000, // Section contains extended relocations.
		MEM_DISCARDABLE            = 0x02000000, // Section can be discarded.
		MEM_NOT_CACHED             = 0x04000000, // Section is not cacheable.
		MEM_NOT_PAGED              = 0x08000000, // Section is not pageable.
		MEM_SHARED                 = 0x10000000, // Section is shareable.
		MEM_EXECUTE                = 0x20000000, // Section is executable.
		MEM_READ                   = 0x40000000, // Section is readable.
		MEM_WRITE                  = 0x80000000, // Section is writeable.

		TLS_SCALE_INDEX            = 0x00000001, // Tls index is scaled
	};

	char     Name[8];
	uint32_t VirtualSize; // or PhysicalAddress
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	CharacteristicFlags Characteristics;
};
struct ResourceDirectory { // IMAGE_RESOURCE_DIRECTORY
	uint32_t Characteristics;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint16_t NumberOfNamedEntries;
	uint16_t NumberOfIdEntries;
//  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];
};
struct ResourceDirectoryEntry { // IMAGE_RESOURCE_DIRECTORY_ENTRY
	union {
		struct {
			uint32_t NameOffset:31;
			uint32_t NameIsString:1;
		};
		uint32_t Name;
		uint16_t Id;
	};
	union {
		uint32_t OffsetToData;
		struct {
			uint32_t OffsetToDirectory:31;
			uint32_t DataIsDirectory:1;
		};
	};
};
struct ResourceDataEntry { // IMAGE_RESOURCE_DATA_ENTRY
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t CodePage;
	uint32_t Reserved;
};
static const unsigned char* GetVersionResource(const unsigned char* data) {
	// Load and check headers
	const DOSHeader *dosh = (DOSHeader*)data;
	if (dosh->e_magic != DOSHeader::SIGNATURE)						{ return NULL; }
	int32_t peOffset = dosh->e_lfanew;
	const NTHeaders *nth = (NTHeaders*)(data+peOffset);
	if (nth->Signature != NTHeaders::SIGNATURE)						{ return NULL; }
	const FileHeader* header = &nth->FileHeader; // identical for 32 and 64 bits
	const OptionalHeader* opt = &nth->OptionalHeader;
	bool is64bit = !(header->Characteristics & FileHeader::MACHINE_32BIT);
	if ((is64bit && opt->Magic != OptionalHeader::SIGNATURE64) || (!is64bit && opt->Magic != OptionalHeader::SIGNATURE32)) { return NULL; }

	// Get the RSRC section
	const SectionHeader *sections = (SectionHeader*)(data+peOffset+sizeof(uint32_t)+sizeof(FileHeader)+header->SizeOfOptionalHeader);
	const SectionHeader *rsrcSect = NULL;
	for (uint16_t i = 0; i < header->NumberOfSections; ++i)
		if (strncmp(sections[i].Name, ".rsrc", ARRAYSIZE(sections[i].Name)) == 0) { rsrcSect = sections+i; break; }
	if (!rsrcSect || rsrcSect->PointerToRawData == 0 || rsrcSect->SizeOfRawData == 0)		{ return NULL; }

	// Get the resource within the RSRC section
	const unsigned char* rsrc = data + rsrcSect->PointerToRawData;
	const ResourceDirectory *dir = (ResourceDirectory*)rsrc;
	const ResourceDirectoryEntry *entry = (ResourceDirectoryEntry*)(dir+1);
	const ResourceDataEntry *dataEntry = (ResourceDataEntry*)(rsrc+entry->OffsetToData);
	return ((dir->NumberOfIdEntries + dir->NumberOfNamedEntries) < 1 || entry->DataIsDirectory) ? NULL : rsrc+dataEntry->OffsetToData-rsrcSect->VirtualAddress;
}
#pragma endregion

#pragma region Version Parsing Structures and Code
Version::Version() : Minor(0), Major(0), Revision(0), Build(0) { }
struct SmallVersion { uint16_t Minor, Major; };
struct FileVersionBasicInfo {
	static const uint32_t SIGNATURE = 0xFEEF04BD;

	static FileVersionBasicInfo* Get(void* ver);

	uint32_t Signature;
	SmallVersion StrucVersion;
	Version FileVersion;
	Version ProductVersion;

	typedef enum _FileFlags : uint32_t { // FLAGS
		DEBUG        = 0x00000001,
		PRERELEASE   = 0x00000002,
		PATCHED      = 0x00000004,
		PRIVATEBUILD = 0x00000008,
		INFOINFERRED = 0x00000010,
		SPECIALBUILD = 0x00000020,
	} Flags;
	Flags FileFlagsMask;
	Flags FileFlags;

	typedef enum _OS : uint32_t { // FLAGS, kinda
		UNKNOWN_OS    = 0x00000000,

		DOS           = 0x00010000,
		OS216         = 0x00020000,
		OS232         = 0x00030000,
		NT            = 0x00040000,

		WINDOWS16     = 0x00000001,
		PM16          = 0x00000002,
		PM32          = 0x00000003,
		WINDOWS32     = 0x00000004,

		DOS_WINDOWS16 = 0x00010001,
		DOS_WINDOWS32 = 0x00010004,
		OS216_PM16    = 0x00020002,
		OS232_PM32    = 0x00030003,
		NT_WINDOWS32  = 0x00040004,
	} OS;
	OS FileOS;

	typedef enum _Type : uint32_t  {
		UNKNOWN_TYPE = 0x00000000,
		APP          = 0x00000001,
		DLL          = 0x00000002,
		DRV          = 0x00000003,
		FONT         = 0x00000004,
		VXD          = 0x00000005,
		STATIC_LIB   = 0x00000007,
	} Type;
	Type FileType;

	typedef enum _SubType : uint32_t {
		UNKNOWN_SUB_TYPE      = 0x00000000,

		DRV_PRINTER           = 0x00000001,
		DRV_KEYBOARD          = 0x00000002,
		DRV_LANGUAGE          = 0x00000003,
		DRV_DISPLAY           = 0x00000004,
		DRV_MOUSE             = 0x00000005,
		DRV_NETWORK           = 0x00000006,
		DRV_SYSTEM            = 0x00000007,
		DRV_INSTALLABLE       = 0x00000008,
		DRV_SOUND             = 0x00000009,
		DRV_COMM              = 0x0000000A,
		DRV_VERSIONED_PRINTER = 0x0000000C,

		FONT_RASTER           = 0x00000001,
		FONT_VECTOR           = 0x00000002,
		FONT_TRUETYPE         = 0x00000003,
	} SubType;
	SubType FileSubtype;

	uint64_t FileDate;
};
struct Block32 {
	uint16_t size;     // size including key, value, and children
	uint16_t val_size;
	uint16_t type;     // 0x0000 for a binary value, 0x0001 for a string value
	const wchar_t* key;
	const unsigned char* value;
	//std::vector<Block32> children;
};
static const FileVersionBasicInfo *GetVersionInfo(const unsigned char* ver) {
	const uint16_t* words = (const uint16_t*)ver;
	Block32 root = { words[0], words[1], words[2], (const wchar_t*)(words+3) };
	root.value = ver + roundUpTo<sizeof(uint32_t)>(3 * sizeof(uint16_t) + (wcslen(root.key) + 1) * sizeof(wchar_t));
	if (wcscmp(root.key, L"VS_VERSION_INFO") != 0 || root.type != 0x0000 || root.val_size != 52) { return NULL; } // error!
	const FileVersionBasicInfo *v = (FileVersionBasicInfo*)root.value;
	if (v->Signature != FileVersionBasicInfo::SIGNATURE || v->StrucVersion.Major != 1 || v->StrucVersion.Minor != 0) { return NULL; } // error!
	return v;
}
#pragma endregion

const Version GetBootmanagerVersion(const void* data) {
	const unsigned char* ver = GetVersionResource((unsigned char*)data);
	if (!ver) { return Version(); }
	const FileVersionBasicInfo *fvi = GetVersionInfo(ver);
	if (!fvi) { return Version(); }
	return fvi->FileVersion;
}
