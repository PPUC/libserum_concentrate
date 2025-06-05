#define __STDC_WANT_LIB_EXT1_ 1

#include "serum-decode.h"

#include <miniz/miniz.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <filesystem>
#include <algorithm>
#include <optional>

#include "serum-version.h"
#include "sparse-vector.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#else

#if not defined(__STDC_LIB_EXT1__)

// trivial implementation of the secure string functions if not directly
// supported by the compiler these do not perform all security checks and can be
// improved for sure
int strcpy_s(char* dest, size_t destsz, const char* src)
{
	if ((dest == NULL) || (src == NULL)) return 1;
	if (strlen(src) >= destsz) return 1;
	strcpy(dest, src);
	return 0;
}

int strcat_s(char* dest, size_t destsz, const char* src)
{
	if ((dest == NULL) || (src == NULL)) return 1;
	if (strlen(dest) + strlen(src) >= destsz) return 1;
	strcat(dest, src);
	return 0;
}

#endif
#endif

#pragma warning(disable : 4996)

const int pathbuflen = 4096;

const uint32_t MAX_NUMBER_FRAMES = 0x7fffffff;
const uint32_t IDENTIFY_SAME_FRAME = 0xfffffffe;

// header
char rname[64];
uint8_t SerumVersion = 0;
uint32_t fwidth, fheight;
uint32_t fwidthx, fheightx;
uint32_t nframes;
uint32_t nocolors, nccolors;
uint32_t ncompmasks, nmovmasks;
uint32_t nsprites;
uint16_t nbackgrounds;
// data
SparseVector<uint32_t> hashcodes(0, true);
SparseVector<uint8_t> shapecompmode(0); // @todo: find out the better default, 0: full comparison (all 4 colors), 1: shape mode (we just compare black 0 against all the 3 other colors as if it was one color)
SparseVector<uint8_t> compmaskID(255);
SparseVector<uint8_t> movrctID(0);
SparseVector<uint8_t> compmasks(0);
SparseVector<uint8_t> movrcts(0);
SparseVector<uint8_t> cpal(0);
SparseVector<uint8_t> isextraframe(0, true);
SparseVector<uint8_t> cframes(0);
SparseVector<uint16_t> cframesn(0);
SparseVector<uint16_t> cframesnx(0);
SparseVector<uint8_t> dynamasks(255);
SparseVector<uint8_t> dynamasksx(255);
SparseVector<uint8_t> dyna4cols(0); // @todo: verifiy the default value
SparseVector<uint16_t> dyna4colsn(0); // @todo: verifiy the default value
SparseVector<uint16_t> dyna4colsnx(0); // @todo: verifiy the default value
SparseVector<uint8_t> framesprites(255);
uint8_t* spritedescriptionso = NULL;
uint8_t* spritedescriptionsc = NULL;
SparseVector<uint8_t> isextrasprite(0, true);
uint8_t* spriteoriginal = NULL;
SparseVector<uint8_t> spritemaskx(255);
SparseVector<uint16_t> spritecolored(0);
SparseVector<uint16_t> spritecoloredx(0);
SparseVector<uint8_t> activeframes(1); // use 1 as default, not 0. I assume, 1 will occur more often than 0.
SparseVector<uint8_t> colorrotations(0);
SparseVector<uint16_t> colorrotationsn(0);
SparseVector<uint16_t> colorrotationsnx(0);
uint16_t* spritedetareas = NULL;
uint32_t* spritedetdwords = NULL;
uint16_t* spritedetdwordpos = NULL;
SparseVector<uint32_t> triggerIDs(0xffffffff);
SparseVector<uint16_t> framespriteBB(0);
SparseVector<uint8_t> isextrabackground(0, true);
SparseVector<uint8_t> backgroundframes(0);
SparseVector<uint16_t> backgroundframesn(0);
SparseVector<uint16_t> backgroundframesnx(0);
SparseVector<uint16_t> backgroundIDs(0xffff);
SparseVector<uint16_t> backgroundBB(0);
SparseVector<uint8_t> backgroundmask(0);
SparseVector<uint8_t> backgroundmaskx(0);
SparseVector<uint8_t> dynashadowsdiro(0);
SparseVector<uint16_t> dynashadowscolo(0);
SparseVector<uint8_t> dynashadowsdirx(0);
SparseVector<uint16_t> dynashadowscolx(0);
SparseVector<uint16_t> dynasprite4cols(0);
SparseVector<uint16_t> dynasprite4colsx(0);
SparseVector<uint8_t> dynaspritemasks(0);
SparseVector<uint8_t> dynaspritemasksx(0);

// variables
bool cromloaded = false;  // is there a crom loaded?
uint16_t lastfound = 0;     // last frame ID identified
uint32_t lastframe_full_crc = 0;
uint32_t lastframe_found = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
uint32_t lasttriggerID = 0xffffffff;  // last trigger ID found
bool isrotation = true;             // are there rotations to send
bool crc32_ready = false;           // is the crc32 table filled?
uint32_t crc32_table[256];            // initial table
bool* framechecked = NULL;          // are these frames checked?
uint16_t ignoreUnknownFramesTimeout = 0;
uint8_t maxFramesToSkip = 0;
uint8_t framesSkippedCounter = 0;
uint8_t standardPalette[PALETTE_SIZE];
uint8_t standardPaletteLength = 0;
uint32_t colorshifts[MAX_COLOR_ROTATIONS];         // how many color we shifted
uint32_t colorshiftinittime[MAX_COLOR_ROTATIONS];  // when was the tick for this
uint32_t colorshifts32[MAX_COLOR_ROTATIONN];         // how many color we shifted for extra res
uint32_t colorshiftinittime32[MAX_COLOR_ROTATIONN];  // when was the tick for this for extra res
uint32_t colorshifts64[MAX_COLOR_ROTATIONN];         // how many color we shifted for extra res
uint32_t colorshiftinittime64[MAX_COLOR_ROTATIONN];  // when was the tick for this for extra res
uint32_t colorrotseruminit; // initial time when all the rotations started
uint32_t colorrotnexttime[MAX_COLOR_ROTATIONS]; // next time of the next rotation
uint32_t colorrotnexttime32[MAX_COLOR_ROTATIONN]; // next time of the next rotation
uint32_t colorrotnexttime64[MAX_COLOR_ROTATIONN]; // next time of the next rotation
// rotation
bool enabled = true;                             // is colorization enabled?

bool isoriginalrequested = true; // are the original resolution frames requested by the caller
bool isextrarequested = false; // are the extra resolution frames requested by the caller

uint32_t rotationnextabsolutetime[MAX_COLOR_ROTATIONS]; // cumulative time for the next rotation for each color rotation

Serum_Frame_Struc mySerum; // structure to keep communicate colorization data

static std::string to_lower(const std::string& str)
{
	std::string lower_str;
	std::transform(str.begin(), str.end(), std::back_inserter(lower_str), [](unsigned char c) { return std::tolower(c); });
	return lower_str;
}

static std::optional<std::string> find_case_insensitive_file(const std::string& dir_path, const std::string& filename)
{
	if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path))
		return std::nullopt;

	std::string lower_filename = to_lower(filename);
	for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
		if (entry.is_regular_file()) {
			std::string entry_filename = entry.path().filename().string();
			if (to_lower(entry_filename) == lower_filename)
				return entry.path().string();
		}
	}
	return std::nullopt;
}

void Free_element(void** ppElement)
{
	// free a malloc block and set its pointer to NULL
	if (ppElement && *ppElement)
	{
		free(*ppElement);
		*ppElement = NULL;
	}
}

void Serum_free(void)
{
	// Free the memory for a full Serum whatever the format version
	hashcodes.clear();
	shapecompmode.clear();
	compmaskID.clear();
	compmasks.clear();
	cpal.clear();
	isextraframe.clear();
	cframesn.clear();
	cframesnx.clear();
	cframes.clear();
	dynamasks.clear();
	dynamasksx.clear();
	dyna4cols.clear();
	dyna4colsn.clear();
	dyna4colsnx.clear();
	framesprites.clear();
	Free_element((void**)&spritedescriptionso);
	Free_element((void**)&spritedescriptionsc);
	isextrasprite.clear();
	Free_element((void**)&spriteoriginal);
	spritemaskx.clear();
	spritecolored.clear();
	spritecoloredx.clear();
	activeframes.clear();
	colorrotations.clear();
	colorrotationsn.clear();
	colorrotationsnx.clear();
	Free_element((void**)&spritedetareas);
	Free_element((void**)&spritedetdwords);
	Free_element((void**)&spritedetdwordpos);
	triggerIDs.clear();
	framespriteBB.clear();
	isextrabackground.clear();
	backgroundframes.clear();
	backgroundframesn.clear();
	backgroundframesnx.clear();
	backgroundIDs.clear();
	backgroundBB.clear();
	backgroundmask.clear();
	backgroundmaskx.clear();
	dynashadowsdiro.clear();
	dynashadowscolo.clear();
	dynashadowsdirx.clear();
	dynashadowscolx.clear();
	dynasprite4cols.clear();
	dynasprite4colsx.clear();
	dynaspritemasks.clear();
	dynaspritemasksx.clear();
	Free_element((void**)&framechecked);
	Free_element((void**)&mySerum.frame);
	Free_element((void**)&mySerum.frame32);
	Free_element((void**)&mySerum.frame64);
	Free_element((void**)&mySerum.palette);
	Free_element((void**)&mySerum.rotations);
	Free_element((void**)&mySerum.rotations32);
	Free_element((void**)&mySerum.rotations64);
	Free_element((void**)&mySerum.rotationsinframe32);
	Free_element((void**)&mySerum.rotationsinframe64);
	Free_element((void**)&mySerum.modifiedelements32);
	Free_element((void**)&mySerum.modifiedelements64);
	cromloaded = false;
}

SERUM_API const char* Serum_GetVersion() { return SERUM_VERSION; }

SERUM_API const char* Serum_GetMinorVersion() { return SERUM_MINOR_VERSION; }

void CRC32encode(void)  // initiating the CRC table, must be called at startup
{
	for (int i = 0; i < 256; i++)
	{
		uint32_t ch = i;
		uint32_t crc = 0;
		for (int j = 0; j < 8; j++)
		{
			uint32_t b = (ch ^ crc) & 1;
			crc >>= 1;
			if (b != 0) crc = crc ^ 0xEDB88320;
			ch >>= 1;
		}
		crc32_table[i] = crc;
	}
	crc32_ready = true;
}

uint32_t crc32_fast(uint8_t* s, uint32_t n)
// computing a buffer CRC32, "CRC32encode()" must have been called before the first use
// version with no mask nor shapemode
{
	uint32_t crc = 0xffffffff;
	for (int i = 0; i < (int)n; i++) crc = (crc >> 8) ^ crc32_table[(s[i] ^ crc) & 0xFF];
	return ~crc;
}

uint32_t crc32_fast_shape(uint8_t* s, uint32_t n)
// computing a buffer CRC32, "CRC32encode()" must have been called before the first use
// version with shapemode and no mask
{
	uint32_t crc = 0xffffffff;
	for (int i = 0; i < (int)n; i++)
	{
		uint8_t val = s[i];
		if (val > 1) val = 1;
		crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
	}
	return ~crc;
}

uint32_t crc32_fast_mask(uint8_t* source, uint8_t* mask, uint32_t n)
// computing a buffer CRC32 on the non-masked area, "CRC32encode()" must have been called before the first use
// version with a mask and no shape mode
{
	uint32_t crc = 0xffffffff;
	for (uint32_t i = 0; i < n; i++)
	{
		if (mask[i] == 0) crc = (crc >> 8) ^ crc32_table[(source[i] ^ crc) & 0xFF];
	}
	return ~crc;
}

uint32_t crc32_fast_mask_shape(uint8_t* source, uint8_t* mask, uint32_t n)
// computing a buffer CRC32 on the non-masked area, "CRC32encode()" must have been called before the first use
// version with a mask and shape mode
{
	uint32_t crc = 0xffffffff;
	for (uint32_t i = 0; i < n; i++)
	{
		if (mask[i] == 0)
		{
			uint8_t val = source[i];
			if (val > 1) val = 1;
			crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
		}
	}
	return ~crc;
}

uint32_t calc_crc32(uint8_t* source, uint8_t mask, uint32_t n, uint8_t Shape)
{
	const uint32_t pixels = fwidth * fheight;
	if (mask < 255)
	{
		uint8_t* pmask = compmasks[mask];
		if (Shape == 1) return crc32_fast_mask_shape(source, pmask, pixels);
		else return crc32_fast_mask(source, pmask, pixels);
	}
	else if (Shape == 1) return crc32_fast_shape(source, pixels);
	return crc32_fast(source, pixels);
}

bool unzip_crz(const char* const filename, const char* const extractpath, char* cromname, int cromsize)
{
	bool ok = true;
	mz_zip_archive zip_archive = {0};

	if (!mz_zip_reader_init_file(&zip_archive, filename, 0))
	{
		return false;
	}

	int num_files = mz_zip_reader_get_num_files(&zip_archive);

	if (num_files == 0 || !mz_zip_reader_get_filename(&zip_archive, 0, cromname, cromsize))
	{
		mz_zip_reader_end(&zip_archive);
		return false;
	}

	for (int i = 0; i < num_files; i++)
	{
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

		char dstPath[pathbuflen];
		if (strcpy_s(dstPath, pathbuflen, extractpath)) goto fail;
		if (strcat_s(dstPath, pathbuflen, file_stat.m_filename)) goto fail;

		mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename,
			dstPath, 0);
	}

	goto nofail;
fail:
	ok = false;
nofail:

	mz_zip_reader_end(&zip_archive);

	return ok;
}

void Full_Reset_ColorRotations(void)
{
	memset(colorshifts, 0, MAX_COLOR_ROTATIONS * sizeof(uint32_t));
	colorrotseruminit = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++) colorshiftinittime[ti] = colorrotseruminit;
	memset(colorshifts32, 0, MAX_COLOR_ROTATIONN * sizeof(uint32_t));
	memset(colorshifts64, 0, MAX_COLOR_ROTATIONN * sizeof(uint32_t));
	for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		colorshiftinittime32[ti] = colorrotseruminit;
		colorshiftinittime64[ti] = colorrotseruminit;
	}
}

uint32_t max(uint32_t v1, uint32_t v2)
{
	if (v1 > v2) return v1;
	return v2;
}

uint32_t min(uint32_t v1, uint32_t v2)
{
	if (v1 < v2) return v1;
	return v2;
}

long serum_file_length;
FILE* fconsole;

const bool IS_DEBUG_READ = false;

size_t my_fread(void* pBuffer, size_t sizeElement, size_t nElements, FILE* stream)
{
	size_t readelem = fread(pBuffer, sizeElement, nElements, stream);
	if (IS_DEBUG_READ) printf("sent elements: %llu / written elements: %llu / written bytes: %llu\r", nElements, readelem, readelem * sizeElement);
	return readelem;
}

Serum_Frame_Struc* Serum_LoadFilev2(FILE* pfile, const uint8_t flags, bool uncompressedCROM, char* pathbuf, uint32_t sizeheader)
{
	my_fread(&fwidth, 4, 1, pfile);
	my_fread(&fheight, 4, 1, pfile);
	my_fread(&fwidthx, 4, 1, pfile);
	my_fread(&fheightx, 4, 1, pfile);
	isoriginalrequested = false;
	isextrarequested = false;
	mySerum.width32 = 0;
	mySerum.width64 = 0;
	if (fheight == 32)
	{
		if (flags & FLAG_REQUEST_32P_FRAMES)
		{
			isoriginalrequested = true;
			mySerum.width32 = fwidth;
		}
		if (flags & FLAG_REQUEST_64P_FRAMES)
		{
			isextrarequested = true;
			mySerum.width64 = fwidthx;
		}

	}
	else
	{
		if (flags & FLAG_REQUEST_64P_FRAMES)
		{
			isoriginalrequested = true;
			mySerum.width64 = fwidth;
		}
		if (flags & FLAG_REQUEST_32P_FRAMES)
		{
			isextrarequested = true;
			mySerum.width32 = fwidthx;
		}
	}
	my_fread(&nframes, 4, 1, pfile);
	my_fread(&nocolors, 4, 1, pfile);
	mySerum.nocolors = nocolors;
	if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0))
	{
		// incorrect file format
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	my_fread(&ncompmasks, 4, 1, pfile);
	my_fread(&nsprites, 4, 1, pfile);
	my_fread(&nbackgrounds, 2, 1, pfile); // nbackgrounds is a uint16_t

	spriteoriginal = (uint8_t*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	spritedetdwords = (uint32_t*)malloc(nsprites * sizeof(uint32_t) * MAX_SPRITE_DETECT_AREAS);
	spritedetdwordpos = (uint16_t*)malloc(nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS);
	spritedetareas = (uint16_t*)malloc(nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS * 4);
	dynasprite4cols = (uint16_t*)malloc(nsprites * MAX_DYNA_SETS_PER_SPRITE * nocolors * sizeof(uint16_t));
	dynasprite4colsx = (uint16_t*)malloc(nsprites * MAX_DYNA_SETS_PER_SPRITE * nocolors * sizeof(uint16_t));
	dynaspritemasks = (uint8_t*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	dynaspritemasksx = (uint8_t*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	if (flags & FLAG_REQUEST_32P_FRAMES)
	{
		mySerum.frame32 = (uint16_t*)malloc(32 * mySerum.width32 * sizeof(uint16_t));
		mySerum.rotations32 = (uint16_t*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(uint16_t));
		mySerum.rotationsinframe32 = (uint16_t*)malloc(2 * 32 * mySerum.width32 * sizeof(uint16_t));
		if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
			mySerum.modifiedelements32 = (uint8_t*)malloc(32 * mySerum.width32);
		if (!mySerum.frame32 || !mySerum.rotations32 || !mySerum.rotationsinframe32 ||
			(flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !mySerum.modifiedelements32))
		{
			Serum_free();
			fclose(pfile);
			enabled = false;
			return NULL;
		}
	}
	if (flags & FLAG_REQUEST_64P_FRAMES)
	{
		mySerum.frame64 = (uint16_t*)malloc(64 * mySerum.width64 * sizeof(uint16_t));
		mySerum.rotations64 = (uint16_t*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(uint16_t));
		mySerum.rotationsinframe64 = (uint16_t*)malloc(2 * 64 * mySerum.width64 * sizeof(uint16_t));
		if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
			mySerum.modifiedelements64 = (uint8_t*)malloc(64 * mySerum.width64);
		if (!mySerum.frame64 || !mySerum.rotations64 || !mySerum.rotationsinframe64 ||
			(flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !mySerum.modifiedelements64))
		{
			Serum_free();
			fclose(pfile);
			enabled = false;
			return NULL;
		}
	}

	hashcodes.my_fread(1, nframes, pfile);
	shapecompmode.my_fread(1, nframes, pfile);
	compmaskID.my_fread(1, nframes, pfile);
	compmasks.my_fread(fwidth * fheight, ncompmasks, pfile);
	isextraframe.my_fread(1, nframes, pfile);
	if (isextrarequested) {
		for (uint32_t ti = 0; ti < nframes; ti++)
		{
			if (isextraframe[ti][0] > 0)
			{
				mySerum.flags |= FLAG_RETURNED_EXTRA_AVAILABLE;
				break;
			}
		}
	}
	else isextraframe.clearIndex();
	cframesn.my_fread(fwidth * fheight, nframes, pfile);
	cframesnx.my_fread(fwidthx * fheightx, nframes, pfile, &isextraframe);
	dynamasks.my_fread(fwidth * fheight, nframes, pfile);
	dynamasksx.my_fread(fwidthx * fheightx, nframes, pfile, &isextraframe);
	dyna4colsn. my_fread(MAX_DYNA_SETS_PER_FRAMEN * nocolors, nframes, pfile);
	dyna4colsnx.my_fread(MAX_DYNA_SETS_PER_FRAMEN * nocolors, nframes, pfile, &isextraframe);
	isextrasprite.my_fread(1, nsprites, pfile);
	if (!isextrarequested) isextrasprite.clearIndex();
	framesprites.my_fread(MAX_SPRITES_PER_FRAME, nframes, pfile);
	my_fread(spriteoriginal, 1, nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
	spritecolored.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, nsprites, pfile);
	spritemaskx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, nsprites, pfile, &isextrasprite); // @todo: check if isextrasprite could be used as parent
	spritecoloredx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, nsprites, pfile, &isextrasprite); // @todo: check if isextrasprite could be used as parent
	activeframes.my_fread(1, nframes, pfile);
	colorrotationsn.my_fread(MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, nframes, pfile);
	colorrotationsnx.my_fread(MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, nframes, pfile, &isextraframe);
	my_fread(spritedetdwords, 4, nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetdwordpos, 2, nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetareas, 2, nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
	triggerIDs.my_fread(1, nframes, pfile);
	framespriteBB.my_fread(MAX_SPRITES_PER_FRAME * 4, nframes, pfile, &framesprites);
	isextrabackground.my_fread(1, nbackgrounds, pfile);
	if (!isextrarequested) isextrabackground.clearIndex();
	backgroundframesn.my_fread(fwidth * fheight, nbackgrounds, pfile);
	backgroundframesnx.my_fread(fwidthx * fheightx, nbackgrounds, pfile, &isextrabackground);
	backgroundIDs.my_fread(1, nframes, pfile);
	backgroundmask.my_fread(fwidth * fheight, nframes, pfile, &backgroundIDs);
	backgroundmaskx.my_fread(fwidthx * fheightx, nframes, pfile, &backgroundIDs);
	if (sizeheader >= 15 * sizeof(uint32_t))
	{
		dynashadowsdiro.my_fread(MAX_DYNA_SETS_PER_FRAMEN, nframes, pfile);
		dynashadowscolo.my_fread(MAX_DYNA_SETS_PER_FRAMEN, nframes, pfile);
		dynashadowsdirx.my_fread(MAX_DYNA_SETS_PER_FRAMEN, nframes, pfile, &isextraframe);
		dynashadowscolx.my_fread(MAX_DYNA_SETS_PER_FRAMEN, nframes, pfile, &isextraframe);
	}
	else {
		dynashadowsdiro.reserve(MAX_DYNA_SETS_PER_FRAMEN);
		dynashadowscolo.reserve(MAX_DYNA_SETS_PER_FRAMEN);
		dynashadowsdirx.reserve(MAX_DYNA_SETS_PER_FRAMEN);
		dynashadowscolx.reserve(MAX_DYNA_SETS_PER_FRAMEN);
	}

	if (sizeheader >= 18 * sizeof(uint32_t))
	{
		dynasprite4cols.my_fread(MAX_DYNA_SETS_PER_SPRITE * nocolors, nsprites, pfile);
		dynasprite4colsx.my_fread(MAX_DYNA_SETS_PER_SPRITE * nocolors, nsprites, pfile, &isextraframe);
		dynaspritemasks.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, nsprites, pfile);
		dynaspritemasksx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, nsprites, pfile, &isextraframe);
	}
	else {
		dynasprite4cols.reserve(MAX_DYNA_SETS_PER_SPRITE * nocolors);
		dynasprite4colsx.reserve(MAX_DYNA_SETS_PER_SPRITE * nocolors);
		dynaspritemasks.reserve(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
		dynaspritemasksx.reserve(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	}
	fclose(pfile);

	mySerum.ntriggers = 0;
	for (uint32_t ti = 0; ti < nframes; ti++)
	{
		if (triggerIDs[ti][0] != 0xffffffff) mySerum.ntriggers++;
	}
	framechecked = (bool*)malloc(sizeof(bool) * nframes);
	if (!framechecked)
	{
		Serum_free();
		enabled = false;
		return NULL;
	}
	if (flags & FLAG_REQUEST_32P_FRAMES)
	{
		if (fheight == 32) mySerum.width32 = fwidth;
		else mySerum.width32 = fwidthx;
	}
	else mySerum.width32 = 0;
	if (flags & FLAG_REQUEST_64P_FRAMES)
	{
		if (fheight == 32) mySerum.width64 = fwidthx;
		else mySerum.width64 = fwidth;
	}
	else mySerum.width64 = 0;

	mySerum.SerumVersion = SerumVersion = SERUM_V2;

	Full_Reset_ColorRotations();
	cromloaded = true;

	if (!uncompressedCROM)
	{
		// remove temporary file that had been extracted from compressed CRZ file
		remove(pathbuf);
	}

	if (IS_DEBUG_READ) fclose(fconsole);

	enabled = true;
	return &mySerum;
}

Serum_Frame_Struc* Serum_LoadFilev1(const char* const filename, const uint8_t flags)
{
	char pathbuf[pathbuflen];
	if (!crc32_ready) CRC32encode();

	// check if we're using an uncompressed cROM file
	const char* ext;
	bool uncompressedCROM = false;
	if ((ext = strrchr(filename, '.')) != NULL)
	{
		if (strcasecmp(ext, ".cROM") == 0)
		{
			uncompressedCROM = true;
			if (strcpy_s(pathbuf, pathbuflen, filename)) return NULL;
		}
	}

	// extract file if it is compressed
	if (!uncompressedCROM)
	{
		char cromname[pathbuflen];
		if (getenv("TMPDIR") != NULL) {
			if (strcpy_s(pathbuf, pathbuflen, getenv("TMPDIR"))) return NULL;
			size_t len = strlen(pathbuf);
			if (len > 0 && pathbuf[len - 1] != '/') {
				if (strcat_s(pathbuf, pathbuflen, "/")) return NULL;
			}
		}
		else if (strcpy_s(pathbuf, pathbuflen, filename)) return NULL;

		if (!unzip_crz(filename, pathbuf, cromname, pathbuflen)) return NULL;
		if (strcat_s(pathbuf, pathbuflen, cromname)) return NULL;
	}

	// Open cRom
	FILE* pfile;
	pfile = fopen(pathbuf, "rb");
	if (!pfile)
	{
		enabled = false;
		return NULL;
	}

	if (IS_DEBUG_READ)
	{
		fconsole = freopen("e:\\output.txt", "w", stdout);
		fseek(pfile, 0, SEEK_END);
		serum_file_length = ftell(pfile);
		fseek(pfile, 0, SEEK_SET);
	}

	// read the header to know how much memory is needed
	my_fread(rname, 1, 64, pfile);
	uint32_t sizeheader;
	my_fread(&sizeheader, 4, 1, pfile);
	// if this is a new format file, we load with Serum_LoadNewFile()
	if (sizeheader >= 14 * sizeof(uint32_t)) return Serum_LoadFilev2(pfile, flags, uncompressedCROM, pathbuf, sizeheader);
	mySerum.SerumVersion = SerumVersion = SERUM_V1;
	my_fread(&fwidth, 4, 1, pfile);
	my_fread(&fheight, 4, 1, pfile);
	// The serum file stored the number of frames as uint32_t, but in fact, the
	// number of frames will never exceed the size of uint16_t (65535)
	uint32_t nframes32;
	my_fread(&nframes32, 4, 1, pfile);
	nframes = (uint16_t)nframes32;
	my_fread(&nocolors, 4, 1, pfile);
	mySerum.nocolors = nocolors;
	my_fread(&nccolors, 4, 1, pfile);
	if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0) || (nccolors == 0))
	{
		// incorrect file format
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	my_fread(&ncompmasks, 4, 1, pfile);
	my_fread(&nmovmasks, 4, 1, pfile);
	my_fread(&nsprites, 4, 1, pfile);
	if (sizeheader >= 13 * sizeof(uint32_t))
		my_fread(&nbackgrounds, 2, 1, pfile);
	else
		nbackgrounds = 0;
	// allocate memory for the serum format
	spritedescriptionso = (uint8_t*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
	spritedescriptionsc = (uint8_t*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
	spritedetdwords = (uint32_t*)malloc(nsprites * sizeof(uint32_t) * MAX_SPRITE_DETECT_AREAS);
	spritedetdwordpos = (uint16_t*)malloc(nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS);
	spritedetareas = (uint16_t*)malloc(nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS * 4);
	mySerum.frame = (uint8_t*)malloc(fwidth * fheight);
	mySerum.palette = (uint8_t*)malloc(3 * 64);
	mySerum.rotations = (uint8_t*)malloc(MAX_COLOR_ROTATIONS * 3);
	if (
		((nsprites > 0) && (!spritedescriptionso || !spritedescriptionsc || !spritedetdwords ||
		!spritedetdwordpos || !spritedetareas)) ||
		!mySerum.frame || !mySerum.palette || !mySerum.rotations)
	{
		Serum_free();
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	// read the cRom file
	hashcodes.my_fread(1, nframes, pfile);
	shapecompmode.my_fread(1, nframes, pfile);
	compmaskID.my_fread(1, nframes, pfile);
	movrctID.my_fread(1, nframes, pfile);
	movrctID.clear(); // we don't need this anymore, but we need to read it to skip the data in the file
	compmasks.my_fread(fwidth * fheight, ncompmasks, pfile);
	movrcts.my_fread(fwidth * fheight, nmovmasks, pfile);
	movrcts.clear(); // we don't need this anymore, but we need to read it to skip the data in the file
	cpal.my_fread(3 * nccolors, nframes, pfile);
	cframes.my_fread(fwidth * fheight, nframes, pfile);
	dynamasks.my_fread(fwidth * fheight, nframes, pfile);
	dyna4cols.my_fread(MAX_DYNA_4COLS_PER_FRAME * nocolors, nframes, pfile);
	framesprites.my_fread(MAX_SPRITES_PER_FRAME, nframes, pfile);
	for (int ti = 0; ti < (int)nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE; ti++)
	{
		my_fread(&spritedescriptionsc[ti], 1, 1, pfile);
		my_fread(&spritedescriptionso[ti], 1, 1, pfile);
	}
	activeframes.my_fread(1, nframes, pfile);
	colorrotations.my_fread(3 * MAX_COLOR_ROTATIONS, nframes, pfile);
	my_fread(spritedetdwords, sizeof(uint32_t), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetdwordpos, sizeof(uint16_t), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetareas, sizeof(uint16_t), nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
	mySerum.ntriggers = 0;
	if (sizeheader >= 11 * sizeof(uint32_t))
	{
		triggerIDs.my_fread(1, nframes, pfile);
		for (uint32_t ti = 0; ti < nframes; ti++)
		{
			if (triggerIDs[ti][0] != 0xffffffff) mySerum.ntriggers++;
		}
	}
	if (sizeheader >= 12 * sizeof(uint32_t)) framespriteBB.my_fread(MAX_SPRITES_PER_FRAME * 4, nframes, pfile, &framesprites);
	else
	{
		for (uint32_t tj = 0; tj < nframes; tj++)
		{
			uint16_t tmp_framespriteBB[4 * MAX_SPRITES_PER_FRAME];
			for (uint32_t ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
			{
				tmp_framespriteBB[ti * 4] = 0;
				tmp_framespriteBB[ti * 4 + 1] = 0;
				tmp_framespriteBB[ti * 4 + 2] = fwidth - 1;
				tmp_framespriteBB[ti * 4 + 3] = fheight - 1;
			}
			framespriteBB.set(tj, tmp_framespriteBB, MAX_SPRITES_PER_FRAME * 4);
		}
	}
	if (sizeheader >= 13 * sizeof(uint32_t))
	{
		backgroundframes.my_fread(fwidth * fheight, nbackgrounds, pfile);
		backgroundIDs.my_fread(1, nframes, pfile);
		backgroundBB.my_fread(4, nframes, pfile, &backgroundIDs);
	}
	fclose(pfile);

	if (IS_DEBUG_READ) fclose(fconsole);

	// allocate memory for previous detected frame
	framechecked = (bool*)malloc(sizeof(bool) * nframes);
	if (!framechecked)
	{
		Serum_free();
		enabled = false;
		return NULL;
	}
	if (fheight == 64)
	{
		mySerum.width64 = fwidth;
		mySerum.width32 = 0;
	}
	else
	{
		mySerum.width32 = fwidth;
		mySerum.width64 = 0;
	}
	Full_Reset_ColorRotations();
	cromloaded = true;

	if (!uncompressedCROM)
	{
		// remove temporary file that had been extracted from compressed CRZ file
		remove(pathbuf);
	}

	enabled = true;
	return &mySerum;
}

SERUM_API Serum_Frame_Struc* Serum_Load(const char* const altcolorpath, const char* const romname, uint8_t flags)
{
	mySerum.SerumVersion = SerumVersion = 0;
	mySerum.flags = 0;
	mySerum.frame = NULL;
	mySerum.frame32 = NULL;
	mySerum.frame64 = NULL;
	mySerum.palette = NULL;
	mySerum.rotations = NULL;
	mySerum.rotations32 = NULL;
	mySerum.rotations64 = NULL;
	mySerum.rotationsinframe32 = NULL;
	mySerum.rotationsinframe64 = NULL;
	mySerum.modifiedelements32 = NULL;
	mySerum.modifiedelements64 = NULL;

	std::string pathbuf = std::string(altcolorpath);
	if (pathbuf.empty() || (pathbuf.back() != '\\' && pathbuf.back() != '/'))
		pathbuf += '/';
	pathbuf += romname;
	pathbuf += '/';

	std::optional<std::string> pFoundFile = find_case_insensitive_file(pathbuf, std::string(romname) + ".cROM");
	if (!pFoundFile)
		pFoundFile = find_case_insensitive_file(pathbuf, std::string(romname) + ".cRZ");
	if (!pFoundFile) {
		enabled = false;
		return NULL;
	}
	return Serum_LoadFilev1(pFoundFile->c_str(), flags);
}

SERUM_API void Serum_Dispose(void)
{
	Serum_free();
}

uint32_t Identify_Frame(uint8_t* frame)
{
	// Usually the first frame has the ID 0, but lastfound is also initialized
	// with 0. So we need a helper to be able to detect frame 0 as new.
	static bool first_match = true;

	if (!cromloaded) return IDENTIFY_NO_FRAME;
	memset(framechecked, false, nframes);
	uint16_t tj = lastfound;  // we start from the frame we last found
	const uint32_t pixels = fwidth * fheight;
	do
	{
		if (!framechecked[tj])
		{
			// calculate the hashcode for the generated frame with the mask and
			// shapemode of the current crom frame
			uint8_t mask = compmaskID[tj][0];
			uint8_t Shape = shapecompmode[tj][0];
			uint32_t Hashc = calc_crc32(frame, mask, pixels, Shape);
			// now we can compare with all the crom frames that share these same mask
			// and shapemode
			uint16_t ti = tj;
			do
			{
				if (!framechecked[ti])
				{
					if ((compmaskID[ti][0] == mask) && (shapecompmode[ti][0] == Shape))
					{
						if (Hashc == hashcodes[ti][0])
						{
							if (first_match || ti != lastfound || mask < 255)
							{
								//Reset_ColorRotations();
								lastfound = ti;
								lastframe_full_crc = crc32_fast(frame, pixels);
								first_match = false;
								return ti;  // we found the frame, we return it
							}

							uint32_t full_crc = crc32_fast(frame, pixels);
							if (full_crc != lastframe_full_crc)
							{
								lastframe_full_crc = full_crc;
								return ti;  // we found the same frame with shape as before, but
								// the full frame is different
							}
							return IDENTIFY_SAME_FRAME;  // we found the frame, but it is the
							// same full frame as before (no
							// mask)
						}
						framechecked[ti] = true;
					}
				}
				if (++ti >= nframes) ti = 0;
			} while (ti != tj);
		}
		if (++tj >= nframes) tj = 0;
	} while (tj != lastfound);

	return IDENTIFY_NO_FRAME;  // we found no corresponding frame
}

void GetSpriteSize(uint8_t nospr, int* pswid, int* pshei, uint8_t* pspro, int sswid, int sshei)
{
	*pswid = *pshei = 0;
	if (nospr >= nsprites) return;
	for (int tj = 0; tj < sshei; tj++)
	{
		for (int ti = 0; ti < sswid; ti++)
		{
			if (pspro[nospr * sswid * sshei + tj * sswid + ti] < 255)
			{
				if (tj > *pshei) *pshei = tj;
				if (ti > *pswid) *pswid = ti;
			}
		}
	}
	(*pshei)++;
	(*pswid)++;
}

bool Check_Sprites(uint8_t* Frame, uint32_t quelleframe, uint8_t* pquelsprites, uint8_t* nspr, uint16_t* pfrx, uint16_t* pfry, uint16_t* pspx, uint16_t* pspy, uint16_t* pwid, uint16_t* phei)
{
	uint8_t ti = 0;
	uint32_t mdword;
	*nspr = 0;
	int spr_width, spr_height;
	uint8_t* pspro;
	if (SerumVersion == SERUM_V2)
	{
		spr_width = MAX_SPRITE_WIDTH;
		spr_height = MAX_SPRITE_HEIGHT;
		pspro = spriteoriginal;
	}
	else
	{
		spr_width = MAX_SPRITE_SIZE;
		spr_height = MAX_SPRITE_SIZE;
		pspro = spritedescriptionso;
	}
	while ((ti < MAX_SPRITES_PER_FRAME) && (framesprites[quelleframe][ti] < 255))
	{
		uint8_t qspr = framesprites[quelleframe][ti];
		int spw, sph;
		GetSpriteSize(qspr, &spw, &sph, pspro, spr_width, spr_height);
		short minxBB = (short)(framespriteBB[quelleframe][ti * 4]);
		short minyBB = (short)(framespriteBB[quelleframe][ti * 4 + 1]);
		short maxxBB = (short)(framespriteBB[quelleframe][ti * 4 + 2]);
		short maxyBB = (short)(framespriteBB[quelleframe][ti * 4 + 3]);
		for (uint32_t tm = 0; tm < MAX_SPRITE_DETECT_AREAS; tm++)
		{
			if (spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4] == 0xffff) continue;
			// we look for the sprite in the frame sent
			for (short ty = minyBB; ty <= maxyBB; ty++)
			{
				mdword = (uint32_t)(Frame[ty * fwidth + minxBB] << 8) | (uint32_t)(Frame[ty * fwidth + minxBB + 1] << 16) |
					(uint32_t)(Frame[ty * fwidth + minxBB + 2] << 24);
				for (short tx = minxBB; tx <= maxxBB - 3; tx++)
				{
					uint32_t tj = ty * fwidth + tx;
					mdword = (mdword >> 8) | (uint32_t)(Frame[tj + 3] << 24);
					// we look for the magic dword first:
					uint16_t sddp = spritedetdwordpos[qspr * MAX_SPRITE_DETECT_AREAS + tm];
					if (mdword == spritedetdwords[qspr * MAX_SPRITE_DETECT_AREAS + tm])
					{
						short frax = (short)tx; // position in the frame of the detection dword
						short fray = (short)ty;
						short sprx = (short)(sddp % spr_width); // position in the sprite of the detection dword
						short spry = (short)(sddp / spr_width);
						// details of the det area:
						short detx = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4]; // position of the detection area in the sprite
						short dety = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 1];
						short detw = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 2]; // size of the detection area
						short deth = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 3];
						// if the detection area starts before the frame (left or top), continue:
						if ((frax - minxBB < sprx - detx) || (fray - minyBB < spry - dety)) continue;
						// position of the detection area in the frame
						int offsx = frax - sprx + detx;
						int offsy = fray - spry + dety;
						// if the detection area extends beyond the bounding box (right or bottom), continue:
						if ((offsx + detw > (int)maxxBB + 1) || (offsy + deth > (int)maxyBB + 1)) continue;
						// we can now check if the full detection area is around the found detection dword
						bool notthere = false;
						for (uint16_t tk = 0; tk < deth; tk++)
						{
							for (uint16_t tl = 0; tl < detw; tl++)
							{
								uint8_t val = pspro[qspr * spr_height * spr_width + (tk + dety) * spr_width + tl + detx];
								if (val == 255) continue;
								if (val != Frame[(tk + offsy) * fwidth + tl + offsx])
								{
									notthere = true;
									break;
								}
							}
							if (notthere == true) break;
						}
						if (!notthere)
						{
							pquelsprites[*nspr] = qspr;
							if (frax - minxBB < sprx)
							{
								pspx[*nspr] = (uint16_t)(sprx - (frax - minxBB)); // display sprite from point
								pfrx[*nspr] = (uint16_t)minxBB;
								pwid[*nspr] = MIN((uint16_t)(spw - pspx[*nspr]), (uint16_t)(maxxBB - minxBB + 1));
							}
							else
							{
								pspx[*nspr] = 0;
								pfrx[*nspr] = (uint16_t)(frax - sprx);
								pwid[*nspr] = MIN((uint16_t)(maxxBB - pfrx[*nspr] + 1), (uint16_t)spw);
							}
							if (fray - minyBB < spry)
							{
								pspy[*nspr] = (uint16_t)(spry - (fray - minyBB));
								pfry[*nspr] = (uint16_t)minyBB;
								phei[*nspr] = MIN((uint16_t)(sph - pspy[*nspr]), (uint16_t)(maxyBB - minyBB + 1));
							}
							else
							{
								pspy[*nspr] = 0;
								pfry[*nspr] = (uint16_t)(fray - spry);
								phei[*nspr] = MIN((uint16_t)(maxyBB - pfry[*nspr] + 1), (uint16_t)sph);
							}
							// we check the identical sprites as there may be duplicate due to
							// the multi detection zones
							bool identicalfound = false;
							for (uint8_t tk = 0; tk < *nspr; tk++)
							{
								if ((pquelsprites[*nspr] == pquelsprites[tk]) &&
									(pfrx[*nspr] == pfrx[tk]) && (pfry[*nspr] == pfry[tk]) &&
									(pwid[*nspr] == pwid[tk]) && (phei[*nspr] == phei[tk]))
									identicalfound = true;
							}
							if (!identicalfound)
							{
								(*nspr)++;
								if (*nspr == MAX_SPRITES_PER_FRAME) return true;
							}
						}
					}
				}
			}
		}
		ti++;
	}
	if (*nspr > 0) return true;
	return false;
}

void Colorize_Framev1(uint8_t* frame, uint32_t IDfound)
{
	uint16_t tj, ti;
	// Generate the colorized version of a frame once identified in the crom
	// frames
	for (tj = 0; tj < fheight; tj++)
	{
		for (ti = 0; ti < fwidth; ti++)
		{
			uint16_t tk = tj * fwidth + ti;

			if ((backgroundIDs[IDfound][0] < nbackgrounds) && (frame[tk] == 0) && (ti >= backgroundBB[IDfound][0]) &&
				(tj >= backgroundBB[IDfound][1]) && (ti <= backgroundBB[IDfound][2]) &&
				(tj <= backgroundBB[IDfound][3]))
				mySerum.frame[tk] = backgroundframes[backgroundIDs[IDfound][0]][tk];
			else
			{
				uint8_t dynacouche = dynamasks[IDfound][tk];
				if (dynacouche == 255) mySerum.frame[tk] = cframes[IDfound][tk];
				else mySerum.frame[tk] = dyna4cols[IDfound][dynacouche * nocolors + frame[tk]];
			}
		}
	}
}

bool CheckExtraFrameAvailable(uint32_t frID)
{
	// Check if there is an extra frame for this frame
	// (and if all the sprites and background involved are available)
	if (isextraframe[frID][0] == 0) return false;
	if (backgroundIDs[frID][0] < 0xffff && isextrabackground[backgroundIDs[frID][0]][0] == 0) return false;
	for (uint32_t ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
	{
		if (framesprites[frID][ti] < 255 && isextrasprite[framesprites[frID][ti]][0] == 0) return false;
	}
	return true;
}

bool ColorInRotation(uint32_t IDfound, uint16_t col, uint16_t* norot, uint16_t* posinrot, bool isextra)
{
	uint16_t* pcol = NULL;
	if (isextra) pcol = colorrotationsnx[IDfound];
	else pcol = colorrotationsn[IDfound];
	*norot = 0xffff;
	for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		for (uint32_t tj = 2; tj < 2u + pcol[ti * MAX_LENGTH_COLOR_ROTATION]; tj++) // val [0] is for length and val [1] is for duration in ms
		{
			if (col == pcol[ti * MAX_LENGTH_COLOR_ROTATION + tj])
			{
				*norot = ti;
				*posinrot = tj - 2; // val [0] is for length and val [1] is for duration in ms
				return true;
			}
		}
	}
	return false;
}

void CheckDynaShadow(uint16_t* pfr, uint32_t nofr, uint8_t dynacouche, uint8_t* isdynapix, uint16_t fx, uint16_t fy, uint32_t fw, uint32_t fh, bool isextra)
{
	uint8_t dsdir;
	if (isextra) dsdir = dynashadowsdirx[nofr][dynacouche];
	else dsdir = dynashadowsdiro[nofr][dynacouche];
	if (dsdir == 0) return;
	uint16_t tcol;
	if (isextra) tcol = dynashadowscolx[nofr][dynacouche];
	else tcol = dynashadowscolo[nofr][dynacouche];
	if ((dsdir & 0b1) > 0 && fx > 0 && fy > 0 && isdynapix[(fy - 1) * fw + fx - 1] == 0) // dyna shadow top left
	{
		isdynapix[(fy - 1) * fw + fx - 1] = 1;
		pfr[(fy - 1) * fw + fx - 1] = tcol;
	}
	if ((dsdir & 0b10) > 0 && fy > 0 && isdynapix[(fy - 1) * fw + fx] == 0) // dyna shadow top
	{
		isdynapix[(fy - 1) * fw + fx] = 1;
		pfr[(fy - 1) * fw + fx] = tcol;
	}
	if ((dsdir & 0b100) > 0 && fx < fw - 1 && fy > 0 && isdynapix[(fy - 1) * fw + fx + 1] == 0) // dyna shadow top right
	{
		isdynapix[(fy - 1) * fw + fx + 1] = 1;
		pfr[(fy - 1) * fw + fx + 1] = tcol;
	}
	if ((dsdir & 0b1000) > 0 && fx < fw - 1 && isdynapix[fy * fw + fx + 1] == 0) // dyna shadow right
	{
		isdynapix[fy * fw + fx + 1] = 1;
		pfr[fy * fw + fx + 1] = tcol;
	}
	if ((dsdir & 0b10000) > 0 && fx < fw - 1 && fy < fh - 1 && isdynapix[(fy + 1) * fw + fx + 1] == 0) // dyna shadow bottom right
	{
		isdynapix[(fy + 1) * fw + fx + 1] = 1;
		pfr[(fy + 1) * fw + fx + 1] = tcol;
	}
	if ((dsdir & 0b100000) > 0 && fy < fh - 1 && isdynapix[(fy + 1) * fw + fx] == 0) // dyna shadow bottom
	{
		isdynapix[(fy + 1) * fw + fx] = 1;
		pfr[(fy + 1) * fw + fx] = tcol;
	}
	if ((dsdir & 0b1000000) > 0 && fx > 0 && fy < fh - 1 && isdynapix[(fy + 1) * fw + fx - 1] == 0) // dyna shadow bottom left
	{
		isdynapix[(fy + 1) * fw + fx - 1] = 1;
		pfr[(fy + 1) * fw + fx - 1] = tcol;
	}
	if ((dsdir & 0b10000000) > 0 && fx > 0 && isdynapix[fy * fw + fx - 1] == 0) // dyna shadow left
	{
		isdynapix[fy * fw + fx - 1] = 1;
		pfr[fy * fw + fx - 1] = tcol;
	}
}

void Colorize_Framev2(uint8_t* frame, uint32_t IDfound)
{
	uint16_t tj, ti;
	// Generate the colorized version of a frame once identified in the crom
	// frames
	bool isextra = CheckExtraFrameAvailable(IDfound);
	mySerum.flags &= 0b11111100;
	uint16_t* pfr;
	uint16_t* prot;
	uint16_t* prt;
	uint32_t* cshft;
	if (mySerum.frame32) mySerum.width32 = 0;
	if (mySerum.frame64) mySerum.width64 = 0;
	uint8_t isdynapix[256 * 64];
	if (((mySerum.frame32 && fheight == 32) || (mySerum.frame64 && fheight == 64)) && isoriginalrequested)
	{
		// create the original res frame
		if (fheight == 32)
		{
			pfr = mySerum.frame32;
			mySerum.flags |= FLAG_RETURNED_32P_FRAME_OK;
			prot = mySerum.rotationsinframe32;
			mySerum.width32 = fwidth;
			prt = colorrotationsn[IDfound];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			mySerum.flags |= FLAG_RETURNED_64P_FRAME_OK;
			prot = mySerum.rotationsinframe64;
			mySerum.width64 = fwidth;
			prt = colorrotationsn[IDfound];
			cshft = colorshifts64;
		}
		memset(isdynapix, 0, fheight * fwidth);
		for (tj = 0; tj < fheight; tj++)
		{
			for (ti = 0; ti < fwidth; ti++)
			{
				uint16_t tk = tj * fwidth + ti;
				if ((backgroundIDs[IDfound][0] < nbackgrounds) && (frame[tk] == 0) && (backgroundmask[IDfound][tk] > 0))
				{
					if (isdynapix[tk] == 0)
					{
						pfr[tk] = backgroundframesn[backgroundIDs[IDfound][0]][tk];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (cshft[prot[tk * 2]] + prot[tk * 2 + 1]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
				}
				else
				{
					uint8_t dynacouche = dynamasks[IDfound][tk];
					if (dynacouche == 255)
					{
						if (isdynapix[tk] == 0)
						{
							pfr[tk] = cframesn[IDfound][tk];
							if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
								pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
						}
					}
					else
					{
						if (frame[tk] > 0)
						{
							CheckDynaShadow(pfr, IDfound, dynacouche, isdynapix, ti, tj, fwidth, fheight, false);
							isdynapix[tk] = 1;
							pfr[tk] = dyna4colsn[IDfound][dynacouche * nocolors + frame[tk]];
						}
						else if (isdynapix[tk]==0) pfr[tk] = dyna4colsn[IDfound][dynacouche * nocolors + frame[tk]];
						prot[tk * 2] = prot[tk * 2 + 1] = 0xffff;
					}
				}
			}
		}
	}
	if (isextra && ((mySerum.frame32 && fheightx == 32) || (mySerum.frame64 && fheightx == 64)) && isextrarequested)
	{
		// create the extra res frame
		if (fheightx == 32)
		{
			pfr = mySerum.frame32;
			mySerum.flags |= FLAG_RETURNED_32P_FRAME_OK;
			prot = mySerum.rotationsinframe32;
			mySerum.width32 = fwidthx;
			prt = colorrotationsnx[IDfound];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			mySerum.flags |= FLAG_RETURNED_64P_FRAME_OK;
			prot = mySerum.rotationsinframe64;
			mySerum.width64 = fwidthx;
			prt = colorrotationsnx[IDfound];
			cshft = colorshifts64;
		}
		memset(isdynapix, 0, fheightx * fwidthx);
		for (tj = 0; tj < fheightx;tj++)
		{
			for (ti = 0; ti < fwidthx; ti++)
			{
				uint16_t tk = tj * fwidthx + ti;
				uint16_t tl;
				if (fheightx == 64) tl = tj / 2 * fwidth + ti / 2;
				else tl = tj * 2 * fwidth + ti * 2;

				if ((backgroundIDs[IDfound][0] < nbackgrounds) && (frame[tl] == 0) && (backgroundmaskx[IDfound][tk] > 0))
				{
					if (isdynapix[tk] == 0)
					{
						pfr[tk] = backgroundframesnx[backgroundIDs[IDfound][0]][tk];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
						{
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
						}
					}
				}
				else
				{
					uint8_t dynacouche = dynamasksx[IDfound][tk];
					if (dynacouche == 255)
					{
						if (isdynapix[tk] == 0)
						{
							pfr[tk] = cframesnx[IDfound][tk];
							if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
							{
								pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
							}
						}
					}
					else
					{
						if (frame[tl] > 0)
						{
							CheckDynaShadow(pfr, IDfound, dynacouche, isdynapix, ti, tj, fwidthx, fheightx, true);
							isdynapix[tk] = 1;
							pfr[tk] = dyna4colsnx[IDfound][dynacouche * nocolors + frame[tl]];
						}
						else if (isdynapix[tk] == 0) pfr[tk] = dyna4colsnx[IDfound][dynacouche * nocolors + frame[tl]];
						prot[tk * 2] = prot[tk * 2 + 1] = 0xffff;
					}
				}
			}
		}
	}
}

void Colorize_Spritev1(uint8_t nosprite, uint16_t frx, uint16_t fry, uint16_t spx, uint16_t spy, uint16_t wid, uint16_t hei)
{
	for (uint16_t tj = 0; tj < hei; tj++)
	{
		for (uint16_t ti = 0; ti < wid; ti++)
		{
			if (spritedescriptionso[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx] < 255)
			{
				mySerum.frame[(fry + tj) * fwidth + frx + ti] = spritedescriptionsc[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx];
			}
		}
	}
}

void Colorize_Spritev2(uint8_t* oframe, uint8_t nosprite, uint16_t frx, uint16_t fry, uint16_t spx, uint16_t spy, uint16_t wid, uint16_t hei, uint32_t IDfound)
{
	uint16_t* pfr, * prot;
	uint16_t* prt;
	uint32_t* cshft;
	if (((mySerum.flags & FLAG_RETURNED_32P_FRAME_OK) && fheight == 32) || ((mySerum.flags & FLAG_RETURNED_64P_FRAME_OK) && fheight == 64))
	{
		if (fheight == 32)
		{
			pfr = mySerum.frame32;
			prot = mySerum.rotationsinframe32;
			prt = colorrotationsn[IDfound];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			prot = mySerum.rotationsinframe64;
			prt = colorrotationsn[IDfound];
			cshft = colorshifts64;
		}
		for (uint16_t tj = 0; tj < hei; tj++)
		{
			for (uint16_t ti = 0; ti < wid; ti++)
			{
				uint16_t tk = (fry + tj) * fwidth + frx + ti;
				uint8_t spriteref = spriteoriginal[nosprite * MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH + (tj + spy) * MAX_SPRITE_WIDTH + ti + spx];
				if (spriteref < 255) {
					uint8_t dynacouche = dynaspritemasks[nosprite][(tj + spy) * MAX_SPRITE_WIDTH + ti + spx];
					if (dynacouche == 255)
					{
						pfr[tk] = spritecolored[nosprite][(tj + spy) * MAX_SPRITE_WIDTH + ti + spx];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
					else
					{
						pfr[tk] = dynasprite4cols[nosprite][dynacouche * nocolors + spriteref];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
				}
			}
		}
	}
	if (((mySerum.flags & FLAG_RETURNED_32P_FRAME_OK) && fheightx == 32) || ((mySerum.flags & FLAG_RETURNED_64P_FRAME_OK) && fheightx == 64))
	{
		uint16_t thei, twid, tfrx, tfry, tspy, tspx;
		if (fheightx == 32)
		{
			pfr = mySerum.frame32;
			prot = mySerum.rotationsinframe32;
			thei = hei / 2;
			twid = wid / 2;
			tfrx = frx / 2;
			tfry = fry / 2;
			tspx = spx / 2;
			tspy = spy / 2;
			prt = colorrotationsnx[IDfound];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			prot = mySerum.rotationsinframe64;
			thei = hei * 2;
			twid = wid * 2;
			tfrx = frx * 2;
			tfry = fry * 2;
			tspx = spx * 2;
			tspy = spy * 2;
			prt = colorrotationsnx[IDfound];
			cshft = colorshifts64;
		}
		for (uint16_t tj = 0; tj < thei; tj++)
		{
			for (uint16_t ti = 0; ti < twid; ti++)
			{
				uint16_t tk = (tfry + tj) * fwidthx + tfrx + ti;
				if (spritemaskx[nosprite][(tj + tspy) * MAX_SPRITE_WIDTH + ti + tspx] < 255)
				{
					uint8_t dynacouche = dynaspritemasksx[nosprite][(tj + tspy) * MAX_SPRITE_WIDTH + ti + tspx];
					if (dynacouche == 255)
					{
						pfr[tk] = spritecoloredx[nosprite][(tj + tspy) * MAX_SPRITE_WIDTH + ti + tspx];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
					else
					{
						uint16_t tl;
						if (fheightx == 64) tl = (tj + fry) / 2 * fwidth + (ti + frx) / 2;
						else tl = (tj + fry) * 2 * fwidth + (ti + frx) * 2;
						pfr[tk] = dynasprite4colsx[nosprite][dynacouche * nocolors + spriteoriginal[tl]];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
				}
			}
		}
	}
}

void Copy_Frame_Palette(uint32_t nofr)
{
	memcpy(mySerum.palette, cpal[nofr], nccolors * 3);
}

SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(uint16_t milliseconds)
{
	ignoreUnknownFramesTimeout = milliseconds;
}

SERUM_API void Serum_SetMaximumUnknownFramesToSkip(uint8_t maximum)
{
	maxFramesToSkip = maximum;
}

SERUM_API void Serum_SetStandardPalette(const uint8_t* palette, const int bitDepth)
{
	int palette_length = (1 << bitDepth) * 3;
	assert(palette_length < PALETTE_SIZE);

	if (palette_length <= PALETTE_SIZE)
	{
		memcpy(standardPalette, palette, palette_length);
		standardPaletteLength = palette_length;
	}
}

uint32_t Serum_ColorizeWithMetadatav1(uint8_t* frame)
{
	// return IDENTIFY_NO_FRAME if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms
	mySerum.triggerID = 0xffffffff;

	if (!enabled)
	{
		// apply standard palette
		memcpy(mySerum.palette, standardPalette, standardPaletteLength);
		return 0;
	}

	// Let's first identify the incoming frame among the ones we have in the crom
	uint32_t frameID = Identify_Frame(frame);
	mySerum.frameID = IDENTIFY_NO_FRAME;

	if (frameID != IDENTIFY_NO_FRAME)
	{
		lastframe_found = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		if (maxFramesToSkip)
		{
			framesSkippedCounter = 0;
		}

		if (frameID == IDENTIFY_SAME_FRAME) return IDENTIFY_NO_FRAME;

		mySerum.frameID = frameID;
		mySerum.rotationtimer = 0;

		uint8_t nosprite[MAX_SPRITES_PER_FRAME], nspr;
		uint16_t frx[MAX_SPRITES_PER_FRAME], fry[MAX_SPRITES_PER_FRAME], spx[MAX_SPRITES_PER_FRAME], spy[MAX_SPRITES_PER_FRAME], wid[MAX_SPRITES_PER_FRAME], hei[MAX_SPRITES_PER_FRAME];
		memset(nosprite, 255, MAX_SPRITES_PER_FRAME);

		bool isspr = Check_Sprites(frame, (uint32_t)lastfound, nosprite, &nspr, frx, fry, spx, spy, wid, hei);
		if (((frameID < MAX_NUMBER_FRAMES) || isspr) && activeframes[lastfound][0] != 0)
		{
			Colorize_Framev1(frame, lastfound);
			Copy_Frame_Palette(lastfound);
			{
			uint32_t ti = 0;
			while (ti < nspr)
			{
				Colorize_Spritev1(nosprite[ti], frx[ti], fry[ti], spx[ti], spy[ti], wid[ti], hei[ti]);
				ti++;
			}
			}
			memcpy(mySerum.rotations, colorrotations[lastfound], MAX_COLOR_ROTATIONS * 3);
			uint32_t now = lastframe_found;
			for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
			{
				if (mySerum.rotations[ti * 3] == 255)
				{
					colorrotnexttime[ti] = 0;
					continue;
				}
				// reset the timer if the previous frame had this rotation inactive
				// or if the last init time is more than a new rotation away
				if (colorrotnexttime[ti] == 0 || (colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10) <= now) colorshiftinittime[ti] = now;
				if (mySerum.rotationtimer == 0)
				{
					mySerum.rotationtimer = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now;
					colorrotnexttime[ti] = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10;
					continue;
				}
				else if ((colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now;
				if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
			}
			if (triggerIDs[lastfound][0] != lasttriggerID) lasttriggerID = mySerum.triggerID = triggerIDs[lastfound][0];
			return (int)mySerum.rotationtimer;  // new frame
		}
	}

	uint32_t now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	if ((ignoreUnknownFramesTimeout && (now - lastframe_found) >= ignoreUnknownFramesTimeout) || (maxFramesToSkip && (frameID == IDENTIFY_NO_FRAME) && (++framesSkippedCounter >= maxFramesToSkip)))
	{
		// apply standard palette
		memcpy(mySerum.palette, standardPalette, standardPaletteLength);
		// disable render features like rotations
		for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONS * 3; ti++)
		{
			mySerum.rotations[ti] = 255;
		}
		mySerum.rotationtimer = 0;
		return 0;  // new but not colorized frame, return true
	}

	return IDENTIFY_NO_FRAME;  // no new frame, return false, client has to update rotations!
}

SERUM_API uint32_t Serum_ColorizeWithMetadatav2(uint8_t* frame)
{
	// return IDENTIFY_NO_FRAME if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms
	mySerum.triggerID = 0xffffffff;

	// Let's first identify the incoming frame among the ones we have in the crom
	uint32_t frameID = Identify_Frame(frame);
	mySerum.frameID = IDENTIFY_NO_FRAME;

	if (frameID != IDENTIFY_NO_FRAME)
	{
		// frame identified
		lastframe_found = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		if (maxFramesToSkip)
		{
			framesSkippedCounter = 0;
		}

		if (frameID == IDENTIFY_SAME_FRAME) return IDENTIFY_NO_FRAME;

		mySerum.frameID = frameID;
		mySerum.rotationtimer = 0;

		uint8_t nosprite[MAX_SPRITES_PER_FRAME], nspr;
		uint16_t frx[MAX_SPRITES_PER_FRAME], fry[MAX_SPRITES_PER_FRAME], spx[MAX_SPRITES_PER_FRAME], spy[MAX_SPRITES_PER_FRAME], wid[MAX_SPRITES_PER_FRAME], hei[MAX_SPRITES_PER_FRAME];
		memset(nosprite, 255, MAX_SPRITES_PER_FRAME);

		bool isspr = Check_Sprites(frame, lastfound, nosprite, &nspr, frx, fry, spx, spy, wid, hei);
		if (((frameID < MAX_NUMBER_FRAMES) || isspr) && activeframes[lastfound][0] != 0)
		{
			// the frame identified is not the same as the preceding
			Colorize_Framev2(frame, lastfound);
			uint32_t ti = 0;
			while (ti < nspr)
			{
				Colorize_Spritev2(frame, nosprite[ti], frx[ti], fry[ti], spx[ti], spy[ti], wid[ti], hei[ti], lastfound);
				ti++;
			}
			uint16_t* pcr32, * pcr64;
			if (fheight == 32)
			{
				pcr32 = colorrotationsn[lastfound];
				pcr64 = colorrotationsnx[lastfound];
			}
			else
			{
				pcr32 = colorrotationsnx[lastfound];
				pcr64 = colorrotationsn[lastfound];
			}
			uint32_t now = lastframe_found;
			if (mySerum.frame32)
			{
				memcpy(mySerum.rotations32, pcr32, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				//memcpy(lastrotations32, mySerum.rotations32, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
				{
					if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] == 0)
					{
						colorrotnexttime32[ti] = 0;
						continue;
					}
					// reset the timer if the previous frame had this rotation inactive
					// or if the last init time is more than a new rotation away
					if (colorrotnexttime32[ti] == 0 || (colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1]) <= now) colorshiftinittime32[ti] = now;
					if (mySerum.rotationtimer == 0)
					{
						mySerum.rotationtimer = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
						colorrotnexttime32[ti] = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1];
						continue;
					}
					else if ((colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
					if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
				}
			}
			if (mySerum.frame64)
			{
				memcpy(mySerum.rotations64, pcr64, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				//memcpy(lastrotations64, mySerum.rotations64, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
				{
					if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] == 0)
					{
						colorrotnexttime64[ti] = 0;
						continue;
					}
					// reset the timer if the previous frame had this rotation inactive
					// or if the last init time is more than a new rotation away
					if (colorrotnexttime64[ti] == 0 || (colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1]) <= now) colorshiftinittime64[ti] = now;
					if (mySerum.rotationtimer == 0)
					{
						mySerum.rotationtimer = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
						colorrotnexttime64[ti] = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1];
						continue;
					}
					else if ((colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
					if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
				}
			}
			if (triggerIDs[lastfound][0] != lasttriggerID) lasttriggerID = mySerum.triggerID = triggerIDs[lastfound][0];
			return (uint32_t)mySerum.rotationtimer;  // new frame, return true
		}
	}

	return IDENTIFY_NO_FRAME;  // no new frame, client has to update rotations!
}

SERUM_API uint32_t Serum_Colorize(uint8_t* frame)
{
	// return IDENTIFY_NO_FRAME if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms
	if (SerumVersion == SERUM_V2) return Serum_ColorizeWithMetadatav2(frame);
	else return Serum_ColorizeWithMetadatav1(frame);
}

uint32_t Calc_Next_Rotationv1(uint32_t now)
{
	uint32_t nextrot = 0xffffffff;
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
	{
		if (mySerum.rotations[ti * 3] == 255) continue;
		if (colorrotnexttime[ti] < nextrot) nextrot = colorrotnexttime[ti];
	}
	if (nextrot == 0xffffffff) return 0;
	return nextrot - now;
}

uint32_t Serum_ApplyRotationsv1(void)
{
	uint32_t isrotation = 0;
	uint32_t now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
	{
		if (mySerum.rotations[ti * 3] == 255) continue;
		uint32_t elapsed = now - colorshiftinittime[ti];
		if (elapsed >= (uint32_t)(mySerum.rotations[ti * 3 + 2] * 10))
		{
			colorshifts[ti]++;
			colorshifts[ti] %= mySerum.rotations[ti * 3 + 1];
			colorshiftinittime[ti] = now;
			colorrotnexttime[ti] = now + mySerum.rotations[ti * 3 + 2] * 10;
			isrotation = 0x10000;
			uint8_t palsave[3 * 64];
			memcpy(palsave, &mySerum.palette[mySerum.rotations[ti * 3] * 3], (size_t)mySerum.rotations[ti * 3 + 1] * 3);
			for (int tj = 0; tj < mySerum.rotations[ti * 3 + 1]; tj++)
			{
				uint32_t shift = (tj + 1) % mySerum.rotations[ti * 3 + 1];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3] = palsave[shift * 3];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3 + 1] = palsave[shift * 3 + 1];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3 + 2] = palsave[shift * 3 + 2];
			}
		}
	}
	mySerum.rotationtimer = (uint16_t)Calc_Next_Rotationv1(now); // can't be more than 65s, so val is contained in the lower word of val
	return ((uint32_t)mySerum.rotationtimer | isrotation); // if there was a rotation, returns the next time in ms to the next one and set high dword to 1
	// if not, just the delay to the next rotation
}

uint32_t Calc_Next_Rotationv2(uint32_t now)
{
	uint32_t nextrot = 0xffffffff;
	for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		if (mySerum.frame32 && mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
		{
			if (colorrotnexttime32[ti] < nextrot) nextrot = colorrotnexttime32[ti];
		}
		if (mySerum.frame64 && mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
		{
			if (colorrotnexttime64[ti] < nextrot) nextrot = colorrotnexttime64[ti];
		}
	}
	if (nextrot == 0xffffffff) return 0;
	return nextrot - now;
}

uint32_t Serum_ApplyRotationsv2(void)
{
	uint32_t isrotation = 0;
	uint32_t sizeframe;
	uint32_t now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	if (mySerum.frame32)
	{
		sizeframe = 32 * mySerum.width32;
		if (mySerum.modifiedelements32) memset(mySerum.modifiedelements32, 0, sizeframe);
		for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
		{
			if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] == 0) continue;
			uint32_t elapsed = now - colorshiftinittime32[ti];
			if (elapsed >= (uint32_t)(mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1]))
			{
				colorshifts32[ti]++;
				colorshifts32[ti] %= mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION];
				colorshiftinittime32[ti] = now;
				colorrotnexttime32[ti] = now + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1];
				isrotation |= 0x10000;
				for (uint32_t tj = 0; tj < sizeframe; tj++)
				{
					if (mySerum.rotationsinframe32[tj * 2] == ti)
					{
						// if we have a pixel which is part of this rotation, we modify it
						mySerum.frame32[tj] = mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 2 + (mySerum.rotationsinframe32[tj * 2 + 1] + colorshifts32[ti]) % mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION]];
						if (mySerum.modifiedelements32) mySerum.modifiedelements32[tj] = 1;
					}
				}
			}
		}
	}
	if (mySerum.frame64)
	{
		sizeframe = 64 * mySerum.width64;
		if (mySerum.modifiedelements64) memset(mySerum.modifiedelements64, 0, sizeframe);
		for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
		{
			if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] == 0) continue;
			uint32_t elapsed = now - colorshiftinittime64[ti];
			if (elapsed >= (uint32_t)(mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1]))
			{
				colorshifts64[ti]++;
				colorshifts64[ti] %= mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION];
				colorshiftinittime64[ti] = now;
				colorrotnexttime64[ti] = now + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1];
				isrotation |= 0x20000;
				for (uint32_t tj = 0; tj < sizeframe; tj++)
				{
					if (mySerum.rotationsinframe64[tj * 2] == ti)
					{
						// if we have a pixel which is part of this rotation, we modify it
						mySerum.frame64[tj] = mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 2 + (mySerum.rotationsinframe64[tj * 2 + 1] + colorshifts64[ti]) % mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION]];
						if (mySerum.modifiedelements64) mySerum.modifiedelements64[tj] = 1;
					}
				}
			}
		}
	}
	mySerum.rotationtimer = (uint16_t)Calc_Next_Rotationv2(now); // can't be more than 65s, so val is contained in the lower word of val
	return ((uint32_t)mySerum.rotationtimer | isrotation); // returns the time in ms untill the next rotation in the lowest word
	// if there was a rotation in the 32P frame, the first bit of the high word is set (0x10000)
	// and if there was a rotation in the 64P frame, the second bit of the high word is set (0x20000)
}

SERUM_API uint32_t Serum_Rotate(void)
{
	if (SerumVersion == SERUM_V2)
	{
		return Serum_ApplyRotationsv2();
	}
	else
	{
		return Serum_ApplyRotationsv1();
	}
	return 0;
}

SERUM_API void Serum_DisableColorization()
{
	enabled = false;
}

SERUM_API void Serum_EnableColorization()
{
	enabled = true;
}
