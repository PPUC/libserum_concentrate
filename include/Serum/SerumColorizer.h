#pragma once

#include <memory>
#include <string>
#include <cstdint>

// Platform-specific API macros
#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#define SERUMAPI __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#define SERUMAPI __attribute__((visibility("default")))
#endif

// Constants
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
   uint8_t* frame; // V1 format frame data
   uint8_t* palette; // V1 format palette data
   uint8_t* rotations; // V1 format color rotation data

   // V2 format data
   uint16_t* frame32; // 32-line high resolution frame
   uint16_t* frame64; // 64-line high resolution frame
   uint16_t* rotations32; // 32-line color rotations
   uint16_t* rotations64; // 64-line color rotations
   uint16_t* rotationsinframe32; // 32-line rotation data in frame
   uint16_t* rotationsinframe64; // 64-line rotation data in frame
   uint8_t* modifiedelements32; // 32-line modified elements
   uint8_t* modifiedelements64; // 64-line modified elements
} Serum_Frame_Struc;

namespace Serum
{

// Forward declarations
class SerumContext;

class SERUMAPI Serum
{
public:
   Serum();
   ~Serum();

   Serum(const Serum&) = delete;
   Serum& operator=(const Serum&) = delete;
   Serum(Serum&&) = default;
   Serum& operator=(Serum&&) = default;

   bool Load(const std::string& altcolorpath, const std::string& romname, uint8_t flags = 0);
   void Dispose();

   uint32_t Colorize(uint8_t* frame);
   uint32_t Rotate();

   void EnableColorization();
   void DisableColorization();
   bool IsColorizationEnabled() const;

   void SetIgnoreUnknownFramesTimeout(uint16_t milliseconds);
   void SetMaximumUnknownFramesToSkip(uint8_t maximum);
   void SetStandardPalette(const uint8_t* palette, int bitDepth);

   const Serum_Frame_Struc* GetFrameData() const;

   bool IsLoaded() const;
   uint8_t GetSerumVersion() const;
   uint32_t GetFrameWidth() const;
   uint32_t GetFrameHeight() const;
   uint32_t GetFrameCount() const;

   static std::string GetVersion();
   static std::string GetMinorVersion();

   // Scene API
   bool Scene_ParseCSV(const char* csv_filename);
   bool Scene_GenerateDump(const char* dump_filename, int id);
   bool Scene_GetInfo(uint16_t sceneId, uint16_t* frameCount, uint16_t* durationPerFrame, bool* interruptable, bool* startImmediately, uint8_t* repeat, uint8_t* endFrame);
   bool Scene_GenerateFrame(uint16_t sceneId, uint16_t frameIndex, uint8_t* buffer, int group);
   void Scene_SetDepth(uint8_t depth);
   int Scene_GetDepth();
   bool Scene_IsActive();
   void Scene_Reset();

private:
   std::unique_ptr<SerumContext> m_context;
   bool m_loaded;
};

} // namespace Serum