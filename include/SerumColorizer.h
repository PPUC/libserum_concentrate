#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "serum-version.h"
#include "SerumContext.h"

// Platform-specific API macros
#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#define SERUMAPI __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#define SERUMAPI __attribute__((visibility("default")))
#endif

namespace Serum
{

class SERUMAPI Serum
{
 public:
  Serum();
  ~Serum();

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
  bool Scene_GetInfo(uint16_t sceneId, uint16_t* frameCount, uint16_t* durationPerFrame, bool* interruptable,
                     bool* startImmediately, uint8_t* repeat, uint8_t* endFrame);
  bool Scene_GenerateFrame(uint16_t sceneId, uint16_t frameIndex, uint8_t* buffer, int group);
  void Scene_SetDepth(uint8_t depth);
  int Scene_GetDepth();
  bool Scene_IsActive();
  void Scene_Reset();

 private:
  std::unique_ptr<SerumContext> m_context;
  bool m_loaded;
};

}  // namespace Serum