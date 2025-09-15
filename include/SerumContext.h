#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "SceneGenerator.h"
#include "SerumColorizer.h"
#include "SparseVector.h"
#include "serum-version.h"

namespace Serum
{

class SerumContext
{
 public:
  SerumContext();
  ~SerumContext();

  SerumContext(const SerumContext&) = delete;
  SerumContext& operator=(const SerumContext&) = delete;
  SerumContext(SerumContext&&) = default;
  SerumContext& operator=(SerumContext&&) = default;

  Serum_Frame_Struc* Load(const char* const altcolorpath, const char* const romname, uint8_t flags);
  void Dispose();
  uint32_t Colorize(uint8_t* frame);
  uint32_t Rotate();
  void DisableColorization();
  void EnableColorization();
  void SetIgnoreUnknownFramesTimeout(uint16_t milliseconds);
  void SetMaximumUnknownFramesToSkip(uint8_t maximum);
  void SetStandardPalette(const uint8_t* palette, const int bitDepth);

  SceneGenerator& GetSceneGenerator() { return *m_sceneGenerator; }
  const SceneGenerator& GetSceneGenerator() const { return *m_sceneGenerator; }

 public:
  std::unique_ptr<SceneGenerator> m_sceneGenerator;

  uint16_t m_sceneFrameCount = 0;
  uint16_t m_sceneCurrentFrame = 0;
  uint16_t m_sceneDurationPerFrame = 0;
  bool m_sceneInterruptable = false;
  bool m_sceneStartImmediately = false;
  uint8_t m_sceneRepeatCount = 0;
  uint8_t m_sceneEndFrame = 0;
  uint8_t m_sceneFrame[192 * 64] = {0};
  uint8_t m_lastFrame[192 * 64] = {0};

  char m_rname[64];
  uint8_t m_serumVersion = 0;
  uint32_t m_fwidth = 0;
  uint32_t m_fheight = 0;
  uint32_t m_nframes = 0;
  uint32_t m_nsprites = 0;
  uint16_t m_nbackgrounds = 0;
  bool m_cromloaded = false;
  uint32_t m_lastfound = 0;
  uint32_t m_lastframeFullCrc = 0;
  uint32_t m_lastframeFound = 0;
  uint32_t m_lasttriggerID = 0xffffffff;
  uint32_t m_lasttriggerTimestamp = 0;
  bool m_isrotation = true;
  bool m_crc32Ready = false;
  uint16_t m_ignoreUnknownFramesTimeout = 0;
  uint8_t m_maxFramesToSkip = 0;
  uint8_t m_framesSkippedCounter = 0;
  const uint8_t* m_standardPalette = nullptr;
  uint8_t m_standardPaletteLength = 0;
  uint32_t m_colorrotseruminit = 0;
  bool m_enabled = true;
  bool m_isoriginalrequested = true;
  bool m_isextrarequested = false;
  Serum_Frame_Struc m_mySerum;

  uint32_t m_fwidthx = 0;
  uint32_t m_fheightx = 0;
  uint32_t m_nocolors = 0;
  uint32_t m_nccolors = 0;
  uint32_t m_ncompmasks = 0;
  uint32_t m_nmovmasks = 0;

  SparseVector<uint32_t> m_hashcodes{0, true};
  SparseVector<uint8_t> m_shapecompmode{0};
  SparseVector<uint8_t> m_compmaskID{255};
  SparseVector<uint8_t> m_movrctID{0};
  SparseVector<uint8_t> m_compmasks{0};
  SparseVector<uint8_t> m_movrcts{0};
  SparseVector<uint8_t> m_cpal{0};
  SparseVector<uint8_t> m_isextraframe{0, true};
  SparseVector<uint8_t> m_cframes{0, false, true};
  SparseVector<uint16_t> m_cframesn{0, false, true};
  SparseVector<uint16_t> m_cframesnx{0, false, true};
  SparseVector<uint8_t> m_dynamasks{255, false, true};
  SparseVector<uint8_t> m_dynamasksx{255, false, true};
  SparseVector<uint8_t> m_dyna4cols{0};
  SparseVector<uint16_t> m_dyna4colsn{0};
  SparseVector<uint16_t> m_dyna4colsnx{0};
  SparseVector<uint8_t> m_framesprites{255};
  uint8_t* m_spritedescriptionso = nullptr;
  uint8_t* m_spritedescriptionsc = nullptr;
  SparseVector<uint8_t> m_isextrasprite{0, true};
  uint8_t* m_spriteoriginal = nullptr;
  SparseVector<uint8_t> m_spritemaskx{255};
  SparseVector<uint16_t> m_spritecolored{0};
  SparseVector<uint16_t> m_spritecoloredx{0};
  SparseVector<uint8_t> m_activeframes{1};
  SparseVector<uint8_t> m_colorrotations{0};
  SparseVector<uint16_t> m_colorrotationsn{0};
  SparseVector<uint16_t> m_colorrotationsnx{0};
  uint16_t* m_spritedetareas = nullptr;
  uint32_t* m_spritedetdwords = nullptr;
  uint16_t* m_spritedetdwordpos = nullptr;
  SparseVector<uint32_t> m_triggerIDs{0xffffffff};
  SparseVector<uint16_t> m_framespriteBB{0};
  SparseVector<uint8_t> m_isextrabackground{0, true};
  SparseVector<uint8_t> m_backgroundframes{0, false, true};
  SparseVector<uint16_t> m_backgroundframesn{0, false, true};
  SparseVector<uint16_t> m_backgroundframesnx{0, false, true};
  SparseVector<uint16_t> m_backgroundIDs{0xffff};
  SparseVector<uint16_t> m_backgroundBB{0};
  SparseVector<uint8_t> m_backgroundmask{0, false, true};
  SparseVector<uint8_t> m_backgroundmaskx{0, false, true};
  SparseVector<uint8_t> m_dynashadowsdiro{0};
  SparseVector<uint16_t> m_dynashadowscolo{0};
  SparseVector<uint8_t> m_dynashadowsdirx{0};
  SparseVector<uint16_t> m_dynashadowscolx{0};
  SparseVector<uint16_t> m_dynasprite4cols{0};
  SparseVector<uint16_t> m_dynasprite4colsx{0};
  SparseVector<uint8_t> m_dynaspritemasks{255};
  SparseVector<uint8_t> m_dynaspritemasksx{255};
  uint8_t* m_sprshapemode = nullptr;

  uint32_t m_crc32Table[256];
  bool* m_framechecked = nullptr;
  uint8_t m_standardPaletteData[PALETTE_SIZE];
  uint32_t m_colorshifts[MAX_COLOR_ROTATIONS];
  uint32_t m_colorshiftinittime[MAX_COLOR_ROTATIONS];
  uint32_t m_colorshifts32[MAX_COLOR_ROTATIONN];
  uint32_t m_colorshiftinittime32[MAX_COLOR_ROTATIONN];
  uint32_t m_colorshifts64[MAX_COLOR_ROTATIONN];
  uint32_t m_colorshiftinittime64[MAX_COLOR_ROTATIONN];
  uint32_t m_colorrotnexttime[MAX_COLOR_ROTATIONS];
  uint32_t m_colorrotnexttime32[MAX_COLOR_ROTATIONN];
  uint32_t m_colorrotnexttime64[MAX_COLOR_ROTATIONN];
  uint32_t m_rotationnextabsolutetime[MAX_COLOR_ROTATIONS];
  uint8_t* m_frameshape = nullptr;
  bool m_firstMatch = true;

  void InitializeCRC32Table();
  uint32_t CalculateCRC32(const uint8_t* data, size_t length);
  std::string ToLower(const std::string& str);
  std::optional<std::string> FindCaseInsensitiveFile(const std::string& dir_path, const std::string& filename);
  void FreeResources();

  bool LoadConcentrateFile(const char* filename, uint8_t flags);
  bool LoadRegularFile(const char* filename, uint8_t flags);
  bool LoadFilev2(const char* filename, uint8_t flags, bool uncompressed, uint32_t sizeheader);
  bool SaveConcentrateFile(const char* filename);
  uint32_t IdentifyFrame(uint8_t* frame);
  uint32_t CalcCRC32(uint8_t* frame, uint8_t mask, uint32_t pixels, uint8_t shape);
  bool ExtractCRZ(const char* filename, const char* extractpath, char* cromname, int cromsize);

  bool AllocateMemory();
  bool ReadSeRumData(FILE* pfile, uint32_t sizeheader);
  void ResetColorRotations();
  void ColorizeFrameV1(uint8_t* frame, uint32_t frameIndex);
  void CopyFramePalette(uint32_t frameIndex);
  uint32_t CalculateNextRotationTime(uint32_t currentTime);
};

}  // namespace Serum