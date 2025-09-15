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

// Main data structure that the library returns
typedef struct
{
  uint8_t SerumVersion;
  uint32_t flags;
  uint32_t nocolors;
  uint32_t width32;
  uint32_t width64;
  uint32_t frameID;
  uint32_t rotationtimer;
  uint32_t ntriggers;
  uint32_t triggerID;

  // Frame data pointers
  uint8_t* frame;      // V1 format frame data
  uint8_t* palette;    // V1 format palette data
  uint8_t* rotations;  // V1 format color rotation data

  // V2 format data
  uint16_t* frame32;             // 32-line high resolution frame
  uint16_t* frame64;             // 64-line high resolution frame
  uint16_t* rotations32;         // 32-line color rotations
  uint16_t* rotations64;         // 64-line color rotations
  uint16_t* rotationsinframe32;  // 32-line rotation data in frame
  uint16_t* rotationsinframe64;  // 64-line rotation data in frame
  uint8_t* modifiedelements32;   // 32-line modified elements
  uint8_t* modifiedelements64;   // 64-line modified elements
} Serum_Frame_Struc;
