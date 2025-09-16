#pragma once

#include <cstdint>

#define SERUM_VERSION_MAJOR 2        // X Digits
#define SERUM_VERSION_MINOR 2        // Max 2 Digits
#define SERUM_VERSION_PATCH 0        // Max 2 Digits
#define SERUM_CONCENTRATE_VERSION 7  // Max 2 Digits

#define _SERUM_STR(x) #x
#define SERUM_STR(x) _SERUM_STR(x)

#define SERUM_VERSION \
  SERUM_STR(SERUM_VERSION_MAJOR) "." SERUM_STR(SERUM_VERSION_MINOR) "." SERUM_STR(SERUM_VERSION_PATCH)
#define SERUM_MINOR_VERSION SERUM_STR(SERUM_VERSION_MAJOR) "." SERUM_STR(SERUM_VERSION_MINOR)

// Version constants
#define SERUM_V1 1
#define SERUM_V2 2

// Flag constants
#define FLAG_REQUEST_32P_FRAMES 1
#define FLAG_REQUEST_64P_FRAMES 2
#define FLAG_REQUEST_FILL_MODIFIED_ELEMENTS 4
#define FLAG_RETURNED_32P_FRAME_OK 1
#define FLAG_RETURNED_64P_FRAME_OK 2
#define FLAG_RETURNED_EXTRA_AVAILABLE 4

// Return values
#define IDENTIFY_NO_FRAME 0xffffffff
#define IDENTIFY_SAME_FRAME 0xfffffffe

#define MAX_COLOR_ROTATIONS 8
#define MAX_COLOR_ROTATIONN 16
#define MAX_LENGTH_COLOR_ROTATION 16
#define PALETTE_SIZE 768
#define MAX_SPRITES_PER_FRAME 32
#define MAX_SPRITE_DETECT_AREAS 8
#define MAX_SPRITE_WIDTH 32
#define MAX_SPRITE_HEIGHT 32
#define MAX_SPRITE_SIZE 32
#define MAX_DYNA_4COLS_PER_FRAME 256
#define MAX_DYNA_SETS_PER_SPRITE 9

// Main data structure that the library returns
struct SerumFrame
{
  // data for v1 Serum format
  uint8_t* frame;      // return the colorized frame
  uint8_t* palette;    // and its palette
  uint8_t* rotations;  // and its color rotations
  // data for v2 Serum format
  // the frame (frame32 or frame64) corresponding to the resolution of the ROM must ALWAYS be defined
  // if a frame pointer is defined, its width, rotations and rotationsinframe pointers must be defined
  uint16_t* frame32;
  uint32_t width32;  // 0 is returned if the 32p colorized frame is not available for this frame
  uint16_t* rotations32;
  uint16_t* rotationsinframe32;  // [width32*32*2] precalculated array to tell if a color is in a color rotations of the
                                 // frame ([X*Y*0]=0xffff if not part of a rotation)
  uint8_t* modifiedelements32;   // (optional) 32P pixels modified during the last rotation
  uint16_t* frame64;
  uint32_t width64;  // 0 is returned if the 64p colorized frame is not available for this frame
  uint16_t* rotations64;
  uint16_t* rotationsinframe64;  // [width64*64*2] precalculated array to tell if a color is in a color rotations of the
                                 // frame ([X*Y*0]=0xffff if not part of a rotation)
  uint8_t* modifiedelements64;   // (optional) 64P pixels modified during the last rotation
  // common data
  uint32_t SerumVersion;  // SERUM_V1 or SERUM_V2
  /// <summary>
  /// flags for return:
  /// if flags & FLAG_RETURNED_32P_FRAME_OK (after Serum_Colorize) : frame32 has been filled
  /// if flags & FLAG_RETURNED_64P_FRAME_OK (after Serum_Colorize) : frame64 has been filled
  /// if flags & FLAG_RETURNED_EXTRA_AVAILABLE (after Serum_Load) : at least 1 frame in the file has an extra resolution
  /// version none of them, display the original frame
  /// </summary>
  uint8_t flags;
  uint32_t nocolors;   // number of shades of orange in the ROM
  uint32_t ntriggers;  // number of triggers in the Serum file
  uint32_t triggerID;  // return 0xffff if no trigger for that frame, the ID of the trigger if one is set for that frame
  uint32_t frameID;    // for CDMD ingame tester
  uint32_t rotationtimer;
};
