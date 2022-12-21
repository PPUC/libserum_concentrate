#ifndef SERUM_DECODE_H
#define SERUM_DECODE_H

#if defined DLL_EXPORTS
  #if defined WIN32
    #define SERUM_API(RetType) extern "C" __declspec(dllexport) RetType
  #else
    #define SERUM_API(RetType) extern "C" RetType __attribute__((visibility("default")))
  #endif
#else
  #if defined WIN32
    #define SERUM_API(RetType) extern "C" __declspec(dllimport) RetType
  #else
    #define SERUM_API(RetType) extern "C" RetType
  #endif
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
#define UINT unsigned int

const int MAX_DYNA_4COLS_PER_FRAME = 16; // max number of color sets for dynamic content for each frame
const int MAX_SPRITE_SIZE = 128; // maximum size of the sprites
const int MAX_SPRITES_PER_FRAME = 32; // maximum amount of sprites to look for per frame
const int MAX_COLOR_ROTATIONS = 8; // maximum amount of color rotations per frame
const int MAX_SPRITE_DETECT_AREAS = 4; // maximum number of areas to detect the sprite

SERUM_API(bool) Serum_Load(const char* altcolorpath, const char* romname);
SERUM_API(void) Serum_Dispose(void);
SERUM_API(void) Serum_Colorize(UINT8* frame, int width, int height, UINT8* palette);

#endif