#include <serum-decode.h>
#include "SerumContext.h"
#include "SceneGenerator.h"

static std::unique_ptr<Serum::Serum> g_serum = nullptr;
static Serum::Serum& GetSerum()
{
   if (!g_serum)
      g_serum = std::make_unique<Serum::Serum>();
   return *g_serum;
}

// Core API
SERUM_API Serum_Frame_Struc* Serum_Load(const char* const altcolorpath, const char* const romname, uint8_t flags)
{
   return GetSerum().Load(altcolorpath, romname, flags) ? const_cast<Serum_Frame_Struc*>(GetSerum().GetFrameData()) : nullptr;
}
SERUM_API void Serum_Dispose(void) { GetSerum().Dispose(); }
SERUM_API uint32_t Serum_Colorize(uint8_t* frame) { return GetSerum().Colorize(frame); }
SERUM_API uint32_t Serum_Rotate(void) { return GetSerum().Rotate(); }

// Configuration
SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(uint16_t milliseconds) { GetSerum().SetIgnoreUnknownFramesTimeout(milliseconds); }
SERUM_API void Serum_SetMaximumUnknownFramesToSkip(uint8_t maximum) { GetSerum().SetMaximumUnknownFramesToSkip(maximum); }
SERUM_API void Serum_SetStandardPalette(const uint8_t* palette, const int bitDepth) { GetSerum().SetStandardPalette(palette, bitDepth); }
SERUM_API void Serum_DisableColorization(void) { GetSerum().DisableColorization(); }
SERUM_API void Serum_EnableColorization(void) { GetSerum().EnableColorization(); }

// Version info
SERUM_API const char* Serum_GetVersion(void)
{
   static std::string v = Serum::Serum::GetVersion();
   return v.c_str();
}
SERUM_API const char* Serum_GetMinorVersion(void)
{
   static std::string v = Serum::Serum::GetMinorVersion();
   return v.c_str();
}

// Scene API
SERUM_API bool Serum_Scene_ParseCSV(const char* const csv_filename) { return GetSerum().Scene_ParseCSV(csv_filename); }
SERUM_API bool Serum_Scene_GenerateDump(const char* const dump_filename, int id) { return GetSerum().Scene_GenerateDump(dump_filename, id); }
SERUM_API bool Serum_Scene_GetInfo(uint16_t sceneId, uint16_t* frameCount, uint16_t* durationPerFrame, bool* interruptable, bool* startImmediately, uint8_t* repeat, uint8_t* endFrame)
{
   return GetSerum().Scene_GetInfo(sceneId, frameCount, durationPerFrame, interruptable, startImmediately, repeat, endFrame);
}
SERUM_API bool Serum_Scene_GenerateFrame(uint16_t sceneId, uint16_t frameIndex, uint8_t* buffer, int group) { return GetSerum().Scene_GenerateFrame(sceneId, frameIndex, buffer, group); }
SERUM_API void Serum_Scene_SetDepth(uint8_t depth) { GetSerum().Scene_SetDepth(depth); }
SERUM_API int Serum_Scene_GetDepth(void) { return GetSerum().Scene_GetDepth(); }
SERUM_API bool Serum_Scene_IsActive(void) { return GetSerum().Scene_IsActive(); }
SERUM_API void Serum_Scene_Reset(void) { GetSerum().Scene_Reset(); }
