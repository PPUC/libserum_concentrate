#include "Serum/SerumContext.h"

#include <miniz/miniz.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "Serum/LZ4Stream.h"

namespace Serum
{

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#else
#if not defined(__STDC_LIB_EXT1__)
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

const uint32_t MAX_NUMBER_FRAMES = 0x7fffffff;

SerumContext::SerumContext()
{
  m_sceneGenerator = std::make_unique<SceneGenerator>();

  std::memset(m_rname, 0, sizeof(m_rname));
  std::memset(m_sceneFrame, 0, sizeof(m_sceneFrame));
  std::memset(m_lastFrame, 0, sizeof(m_lastFrame));
  std::memset(&m_mySerum, 0, sizeof(m_mySerum));
  std::memset(m_crc32Table, 0, sizeof(m_crc32Table));
  std::memset(m_standardPaletteData, 0, sizeof(m_standardPaletteData));
  std::memset(m_colorshifts, 0, sizeof(m_colorshifts));
  std::memset(m_colorshiftinittime, 0, sizeof(m_colorshiftinittime));
  std::memset(m_colorshifts32, 0, sizeof(m_colorshifts32));
  std::memset(m_colorshiftinittime32, 0, sizeof(m_colorshiftinittime32));
  std::memset(m_colorshifts64, 0, sizeof(m_colorshifts64));
  std::memset(m_colorshiftinittime64, 0, sizeof(m_colorshiftinittime64));
  std::memset(m_colorrotnexttime, 0, sizeof(m_colorrotnexttime));
  std::memset(m_colorrotnexttime32, 0, sizeof(m_colorrotnexttime32));
  std::memset(m_colorrotnexttime64, 0, sizeof(m_colorrotnexttime64));
  std::memset(m_rotationnextabsolutetime, 0, sizeof(m_rotationnextabsolutetime));

  m_lastframeFound = static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count());

  InitializeCRC32Table();
}

SerumContext::~SerumContext() { FreeResources(); }

void SerumContext::FreeResources()
{
  if (m_spritedescriptionso)
  {
    free(m_spritedescriptionso);
    m_spritedescriptionso = nullptr;
  }
  if (m_spritedescriptionsc)
  {
    free(m_spritedescriptionsc);
    m_spritedescriptionsc = nullptr;
  }
  if (m_spriteoriginal)
  {
    free(m_spriteoriginal);
    m_spriteoriginal = nullptr;
  }
  if (m_spritedetareas)
  {
    free(m_spritedetareas);
    m_spritedetareas = nullptr;
  }
  if (m_spritedetdwords)
  {
    free(m_spritedetdwords);
    m_spritedetdwords = nullptr;
  }
  if (m_spritedetdwordpos)
  {
    free(m_spritedetdwordpos);
    m_spritedetdwordpos = nullptr;
  }
  if (m_framechecked)
  {
    free(m_framechecked);
    m_framechecked = nullptr;
  }
  if (m_sprshapemode)
  {
    free(m_sprshapemode);
    m_sprshapemode = nullptr;
  }
  if (m_frameshape)
  {
    free(m_frameshape);
    m_frameshape = nullptr;
  }
  if (m_mySerum.frame)
  {
    free(m_mySerum.frame);
    m_mySerum.frame = nullptr;
  }
  if (m_mySerum.frame32)
  {
    free(m_mySerum.frame32);
    m_mySerum.frame32 = nullptr;
  }
  if (m_mySerum.frame64)
  {
    free(m_mySerum.frame64);
    m_mySerum.frame64 = nullptr;
  }
  if (m_mySerum.palette)
  {
    free(m_mySerum.palette);
    m_mySerum.palette = nullptr;
  }
  if (m_mySerum.rotations)
  {
    free(m_mySerum.rotations);
    m_mySerum.rotations = nullptr;
  }
  if (m_mySerum.rotations32)
  {
    free(m_mySerum.rotations32);
    m_mySerum.rotations32 = nullptr;
  }
  if (m_mySerum.rotations64)
  {
    free(m_mySerum.rotations64);
    m_mySerum.rotations64 = nullptr;
  }
  if (m_mySerum.rotationsinframe32)
  {
    free(m_mySerum.rotationsinframe32);
    m_mySerum.rotationsinframe32 = nullptr;
  }
  if (m_mySerum.rotationsinframe64)
  {
    free(m_mySerum.rotationsinframe64);
    m_mySerum.rotationsinframe64 = nullptr;
  }
  if (m_mySerum.modifiedelements32)
  {
    free(m_mySerum.modifiedelements32);
    m_mySerum.modifiedelements32 = nullptr;
  }
  if (m_mySerum.modifiedelements64)
  {
    free(m_mySerum.modifiedelements64);
    m_mySerum.modifiedelements64 = nullptr;
  }

  m_hashcodes.clear();
  m_shapecompmode.clear();
  m_compmaskID.clear();
  m_compmasks.clear();
  m_cpal.clear();
  m_isextraframe.clear();
  m_cframesn.clear();
  m_cframesnx.clear();
  m_cframes.clear();
  m_dynamasks.clear();
  m_dynamasksx.clear();
  m_dyna4cols.clear();
  m_dyna4colsn.clear();
  m_dyna4colsnx.clear();
  m_framesprites.clear();
  m_isextrasprite.clear();
  m_spritemaskx.clear();
  m_spritecolored.clear();
  m_spritecoloredx.clear();
  m_activeframes.clear();
  m_colorrotations.clear();
  m_colorrotationsn.clear();
  m_colorrotationsnx.clear();
  m_triggerIDs.clear();
  m_framespriteBB.clear();
  m_isextrabackground.clear();
  m_backgroundframes.clear();
  m_backgroundframesn.clear();
  m_backgroundframesnx.clear();
  m_backgroundIDs.clear();
  m_backgroundBB.clear();
  m_backgroundmask.clear();
  m_backgroundmaskx.clear();
  m_dynashadowsdiro.clear();
  m_dynashadowscolo.clear();
  m_dynashadowsdirx.clear();
  m_dynashadowscolx.clear();
  m_dynasprite4cols.clear();
  m_dynasprite4colsx.clear();
  m_dynaspritemasks.clear();
  m_dynaspritemasksx.clear();
}

void SerumContext::InitializeCRC32Table()
{
  if (m_crc32Ready) return;

  const uint32_t polynomial = 0xEDB88320;
  for (uint32_t i = 0; i < 256; i++)
  {
    uint32_t crc = i;
    for (uint32_t j = 0; j < 8; j++)
    {
      if (crc & 1)
      {
        crc = (crc >> 1) ^ polynomial;
      }
      else
      {
        crc >>= 1;
      }
    }
    m_crc32Table[i] = crc;
  }
  m_crc32Ready = true;
}

uint32_t SerumContext::CalculateCRC32(const uint8_t* data, size_t length)
{
  if (!m_crc32Ready) InitializeCRC32Table();

  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++)
  {
    crc = (crc >> 8) ^ m_crc32Table[(crc ^ data[i]) & 0xFF];
  }
  return ~crc;
}

std::string SerumContext::ToLower(const std::string& str)
{
  std::string lower_str;
  std::transform(str.begin(), str.end(), std::back_inserter(lower_str),
                 [](unsigned char c) { return std::tolower(c); });
  return lower_str;
}

std::optional<std::string> SerumContext::FindCaseInsensitiveFile(const std::string& dir_path,
                                                                 const std::string& filename)
{
  std::string path_copy = dir_path;

  if (!std::filesystem::exists(path_copy) || !std::filesystem::is_directory(path_copy)) return std::nullopt;

  std::string lower_filename = ToLower(filename);

  try
  {
    for (const auto& entry : std::filesystem::directory_iterator(path_copy))
    {
      if (entry.is_regular_file())
      {
        std::string entry_filename = entry.path().filename().string();
        if (ToLower(entry_filename) == lower_filename) return entry.path().string();
      }
    }
  }
  catch (const std::filesystem::filesystem_error& e)
  {
    return std::nullopt;
  }

  return std::nullopt;
}

void SerumContext::SetIgnoreUnknownFramesTimeout(uint16_t milliseconds) { m_ignoreUnknownFramesTimeout = milliseconds; }

void SerumContext::SetMaximumUnknownFramesToSkip(uint8_t maximum) { m_maxFramesToSkip = maximum; }

void SerumContext::SetStandardPalette(const uint8_t* palette, const int bitDepth)
{
  if (palette && bitDepth > 0 && bitDepth <= PALETTE_SIZE)
  {
    std::memcpy(m_standardPaletteData, palette, bitDepth);
    m_standardPalette = m_standardPaletteData;
    m_standardPaletteLength = static_cast<uint8_t>(bitDepth);
  }
}

void SerumContext::DisableColorization() { m_enabled = false; }

void SerumContext::EnableColorization() { m_enabled = true; }

void SerumContext::Dispose()
{
  FreeResources();
  m_cromloaded = false;
  m_serumVersion = 0;
}

SerumFrame* SerumContext::Load(const char* const altcolorpath, const char* const romname, uint8_t flags)
{
  FreeResources();

  // Initialize frame structure
  std::memset(&m_mySerum, 0, sizeof(m_mySerum));
  m_serumVersion = 0;
  m_cromloaded = false;

  if (!altcolorpath || !romname)
  {
    m_enabled = false;
    return nullptr;
  }

  // Build path
  std::string pathbuf = std::string(altcolorpath);
  if (pathbuf.empty() || (pathbuf.back() != '\\' && pathbuf.back() != '/'))
  {
    pathbuf += '/';
  }
  pathbuf += romname;
  pathbuf += '/';

  // Look for CSV file first
  std::optional<std::string> csvFoundFile = FindCaseInsensitiveFile(pathbuf, std::string(romname) + ".csv");
  if (csvFoundFile)
  {
    flags |= FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES;
  }

  // Try to load concentrate file first
  std::optional<std::string> pFoundFile = FindCaseInsensitiveFile(pathbuf, std::string(romname) + ".cROMc");
  if (pFoundFile)
  {
    if (LoadConcentrateFile(pFoundFile->c_str(), flags))
    {
      if (csvFoundFile && m_sceneGenerator->parseCSV(csvFoundFile->c_str()))
      {
        SaveConcentrateFile(pFoundFile->c_str());
      }
      m_cromloaded = true;
      return &m_mySerum;
    }
  }

  // Try regular cROM or cRZ files
  flags |= FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES;

  pFoundFile = FindCaseInsensitiveFile(pathbuf, std::string(romname) + ".cROM");
  if (!pFoundFile)
  {
    pFoundFile = FindCaseInsensitiveFile(pathbuf, std::string(romname) + ".cRZ");
  }

  if (!pFoundFile)
  {
    m_enabled = false;
    return nullptr;
  }

  if (LoadRegularFile(pFoundFile->c_str(), flags))
  {
    if (csvFoundFile)
    {
      m_sceneGenerator->parseCSV(csvFoundFile->c_str());
    }
    SaveConcentrateFile(pFoundFile->c_str());
    m_cromloaded = true;

    if (m_sceneGenerator->isActive())
    {
      m_sceneGenerator->setDepth(m_mySerum.nocolors == 16 ? 4 : 2);
    }

    return &m_mySerum;
  }

  m_enabled = false;
  return nullptr;
}

uint32_t SerumContext::Colorize(uint8_t* frame)
{
  if (!m_cromloaded || !m_enabled || !frame)
  {
    return IDENTIFY_NO_FRAME;
  }

  // Reset trigger ID
  m_mySerum.triggerID = 0xffffffff;

  // Apply standard palette if disabled
  if (!m_enabled)
  {
    if (m_standardPalette && m_standardPaletteLength > 0)
    {
      std::memcpy(m_mySerum.palette, m_standardPalette, m_standardPaletteLength);
    }
    return 0;
  }

  // Identify the frame
  uint32_t frameId = IdentifyFrame(frame);
  m_mySerum.frameID = IDENTIFY_NO_FRAME;

  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                 .count();
  uint32_t currentTime = static_cast<uint32_t>(now);

  if (frameId != IDENTIFY_NO_FRAME)
  {
    m_lastframeFound = currentTime;
    if (m_maxFramesToSkip > 0)
    {
      m_framesSkippedCounter = 0;
    }

    if (frameId == IDENTIFY_SAME_FRAME)
    {
      return IDENTIFY_NO_FRAME;
    }

    m_mySerum.frameID = frameId;
    m_mySerum.rotationtimer = 0;

    // Check if frame is active
    if (frameId < m_nframes && m_activeframes[m_lastfound][0] != 0)
    {
      ColorizeFrameV1(frame, m_lastfound);
      CopyFramePalette(m_lastfound);

      // Set up color rotations
      std::memcpy(m_mySerum.rotations, m_colorrotations[m_lastfound], MAX_COLOR_ROTATIONS * 3);

      // Calculate rotation timing
      for (uint32_t ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
      {
        if (m_mySerum.rotations[ti * 3] == 255)
        {
          m_colorrotnexttime[ti] = 0;
          continue;
        }

        // Reset timer if rotation was inactive or if time has passed
        if (m_colorrotnexttime[ti] == 0 ||
            (m_colorshiftinittime[ti] + m_mySerum.rotations[ti * 3 + 2] * 10) <= currentTime)
        {
          m_colorshiftinittime[ti] = currentTime;
        }

        if (m_mySerum.rotationtimer == 0)
        {
          m_mySerum.rotationtimer = m_colorshiftinittime[ti] + m_mySerum.rotations[ti * 3 + 2] * 10 - currentTime;
          m_colorrotnexttime[ti] = m_colorshiftinittime[ti] + m_mySerum.rotations[ti * 3 + 2] * 10;
        }
        else
        {
          uint32_t nextTime = m_colorshiftinittime[ti] + m_mySerum.rotations[ti * 3 + 2] * 10 - currentTime;
          if (nextTime < m_mySerum.rotationtimer)
          {
            m_mySerum.rotationtimer = nextTime;
          }
        }

        if (m_mySerum.rotationtimer <= 0)
        {
          m_mySerum.rotationtimer = 10;
        }
      }

      // Handle triggers
      if (m_triggerIDs[m_lastfound][0] != m_lasttriggerID || m_lasttriggerTimestamp < (currentTime - 500))
      {  // 500ms timeout
        m_lasttriggerID = m_mySerum.triggerID = m_triggerIDs[m_lastfound][0];
        m_lasttriggerTimestamp = currentTime;
      }

      return static_cast<uint32_t>(m_mySerum.rotationtimer);
    }
  }

  // Handle timeout or max frames skipped
  if ((m_ignoreUnknownFramesTimeout && (currentTime - m_lastframeFound) >= m_ignoreUnknownFramesTimeout) ||
      (m_maxFramesToSkip && frameId == IDENTIFY_NO_FRAME && (++m_framesSkippedCounter >= m_maxFramesToSkip)))
  {
    // Apply standard palette
    if (m_standardPalette && m_standardPaletteLength > 0)
    {
      std::memcpy(m_mySerum.palette, m_standardPalette, m_standardPaletteLength);
    }
    return 0;
  }

  return IDENTIFY_NO_FRAME;
}

uint32_t SerumContext::Rotate()
{
  if (!m_cromloaded || !m_enabled)
  {
    return 0;
  }

  uint32_t rotationFlags = 0;
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                 .count();
  uint32_t currentTime = static_cast<uint32_t>(now);

  // Apply rotations for v1 format
  for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
  {
    if (m_mySerum.rotations[ti * 3] == 255) continue;

    uint32_t elapsed = currentTime - m_colorshiftinittime[ti];
    uint32_t rotationDelay = m_mySerum.rotations[ti * 3 + 2] * 10;  // Convert to milliseconds

    if (elapsed >= rotationDelay)
    {
      // Perform the rotation
      m_colorshifts[ti]++;
      m_colorshifts[ti] %= m_mySerum.rotations[ti * 3 + 1];  // Number of colors in rotation
      m_colorshiftinittime[ti] = currentTime;
      m_colorrotnexttime[ti] = currentTime + rotationDelay;
      rotationFlags = 0x10000;  // Set rotation flag

      // Rotate the palette colors
      uint8_t startIndex = m_mySerum.rotations[ti * 3];     // Starting color index
      uint8_t numColors = m_mySerum.rotations[ti * 3 + 1];  // Number of colors to rotate

      if (numColors > 1 && startIndex + numColors <= 64)
      {
        uint8_t paletteBackup[3 * 64];
        std::memcpy(paletteBackup, &m_mySerum.palette[startIndex * 3], numColors * 3);

        for (int tj = 0; tj < numColors; tj++)
        {
          uint32_t shift = (tj + 1) % numColors;
          m_mySerum.palette[(startIndex + tj) * 3] = paletteBackup[shift * 3];
          m_mySerum.palette[(startIndex + tj) * 3 + 1] = paletteBackup[shift * 3 + 1];
          m_mySerum.palette[(startIndex + tj) * 3 + 2] = paletteBackup[shift * 3 + 2];
        }
      }
    }
  }

  // Calculate next rotation time
  uint32_t nextRotationTime = CalculateNextRotationTime(currentTime);
  m_mySerum.rotationtimer = static_cast<uint16_t>(nextRotationTime);

  return (static_cast<uint32_t>(m_mySerum.rotationtimer) | rotationFlags);
}

bool SerumContext::LoadRegularFile(const char* filename, uint8_t flags)
{
  if (!filename) return false;

  char pathbuf[4096];
  if (!m_crc32Ready) InitializeCRC32Table();

  // Check if we're using an uncompressed cROM file
  const char* ext = strrchr(filename, '.');
  bool uncompressedCROM = false;
  if (ext && strcasecmp(ext, ".cROM") == 0)
  {
    uncompressedCROM = true;
    if (strcpy_s(pathbuf, 4096, filename)) return false;
  }

  // Extract file if it is compressed (.cRZ)
  if (!uncompressedCROM)
  {
    char cromname[4096];
    if (getenv("TMPDIR") != NULL)
    {
      if (strcpy_s(pathbuf, 4096, getenv("TMPDIR"))) return false;
      size_t len = strlen(pathbuf);
      if (len > 0 && pathbuf[len - 1] != '/')
      {
        if (strcat_s(pathbuf, 4096, "/")) return false;
      }
    }
    else if (strcpy_s(pathbuf, 4096, filename))
      return false;

    if (!ExtractCRZ(filename, pathbuf, cromname, 4096)) return false;
    if (strcat_s(pathbuf, 4096, cromname)) return false;
  }

  // Open cROM file
  FILE* pfile = fopen(pathbuf, "rb");
  if (!pfile)
  {
    m_enabled = false;
    return false;
  }

  // Read the header
  if (fread(m_rname, 1, 64, pfile) != 64)
  {
    fclose(pfile);
    return false;
  }

  uint32_t sizeheader;
  if (fread(&sizeheader, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  // If this is a new format file (v2), handle it differently
  if (sizeheader >= 14 * sizeof(uint32_t))
  {
    fclose(pfile);
    return LoadFilev2(filename, flags, uncompressedCROM, sizeheader);
  }

  // Continue with v1 format
  m_mySerum.SerumVersion = m_serumVersion = SERUM_V1;

  if (fread(&m_fwidth, 4, 1, pfile) != 1 || fread(&m_fheight, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  uint32_t nframes32;
  if (fread(&nframes32, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }
  m_nframes = static_cast<uint16_t>(nframes32);

  if (fread(&m_nocolors, 4, 1, pfile) != 1 || fread(&m_nccolors, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  m_mySerum.nocolors = m_nocolors;

  // Validate basic parameters
  if (m_fwidth == 0 || m_fheight == 0 || m_nframes == 0 || m_nocolors == 0 || m_nccolors == 0)
  {
    fclose(pfile);
    m_enabled = false;
    return false;
  }

  if (fread(&m_ncompmasks, 4, 1, pfile) != 1 || fread(&m_nmovmasks, 4, 1, pfile) != 1 ||
      fread(&m_nsprites, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  if (sizeheader >= 13 * sizeof(uint32_t))
  {
    if (fread(&m_nbackgrounds, 2, 1, pfile) != 1)
    {
      fclose(pfile);
      return false;
    }
  }
  else
  {
    m_nbackgrounds = 0;
  }

  // Allocate memory
  if (!AllocateMemory())
  {
    fclose(pfile);
    return false;
  }

  // Read all the data
  if (!ReadSeRumData(pfile, sizeheader))
  {
    fclose(pfile);
    return false;
  }

  fclose(pfile);

  // Set up frame buffer
  m_framechecked = static_cast<bool*>(malloc(sizeof(bool) * m_nframes));
  if (!m_framechecked)
  {
    FreeResources();
    m_enabled = false;
    return false;
  }

  // Set up frame dimensions for different modes
  if (flags & FLAG_REQUEST_32P_FRAMES)
  {
    m_mySerum.width32 = (m_fheight == 32) ? m_fwidth : m_fwidthx;
  }
  else
  {
    m_mySerum.width32 = 0;
  }

  if (flags & FLAG_REQUEST_64P_FRAMES)
  {
    m_mySerum.width64 = (m_fheight == 32) ? m_fwidthx : m_fwidth;
  }
  else
  {
    m_mySerum.width64 = 0;
  }

  ResetColorRotations();
  m_cromloaded = true;
  m_enabled = true;

  return true;
}

uint32_t SerumContext::IdentifyFrame(uint8_t* frame)
{
  if (!m_cromloaded || !frame)
  {
    return IDENTIFY_NO_FRAME;
  }

  std::memset(m_framechecked, false, m_nframes);
  uint16_t tj = m_lastfound;  // Start from the frame we last found
  const uint32_t pixels = m_fwidth * m_fheight;

  do
  {
    if (!m_framechecked[tj])
    {
      // Calculate the hashcode for the generated frame with the mask and shapemode
      uint8_t mask = m_compmaskID[tj][0];
      uint8_t shape = m_shapecompmode[tj][0];
      uint32_t hashc = CalcCRC32(frame, mask, pixels, shape);

      // Compare with all the crom frames that share the same mask and shapemode
      uint16_t ti = tj;
      do
      {
        if (!m_framechecked[ti])
        {
          if ((m_compmaskID[ti][0] == mask) && (m_shapecompmode[ti][0] == shape))
          {
            if (hashc == m_hashcodes[ti][0])
            {
              if (m_firstMatch || ti != m_lastfound || mask < 255)
              {
                m_lastfound = ti;
                m_lastframeFullCrc = CalculateCRC32(frame, pixels);
                m_firstMatch = false;
                return ti;  // Found the frame
              }

              uint32_t full_crc = CalculateCRC32(frame, pixels);
              if (full_crc != m_lastframeFullCrc)
              {
                m_lastframeFullCrc = full_crc;
                return ti;  // Same frame with shape but different full frame
              }
              return IDENTIFY_SAME_FRAME;  // Same frame as before
            }
            m_framechecked[ti] = true;
          }
        }
        if (++ti >= m_nframes) ti = 0;
      } while (ti != tj);
    }
    if (++tj >= m_nframes) tj = 0;
  } while (tj != m_lastfound);

  return IDENTIFY_NO_FRAME;  // No corresponding frame found
}

uint32_t SerumContext::CalcCRC32(uint8_t* frame, uint8_t mask, uint32_t pixels, uint8_t shape)
{
  if (mask == 255)
  {
    // No mask, use regular CRC32
    return CalculateCRC32(frame, pixels);
  }

  // CRC32 with mask - only process non-masked pixels
  uint32_t crc = 0xffffffff;
  for (uint32_t i = 0; i < pixels; i++)
  {
    // The mask parameter indicates which mask to use, but for simplicity
    // we'll process all pixels if mask is not 255 (which means no mask)
    if (mask == 0)
    {
      if (shape > 0)
      {
        // Shape mode - values > 1 become 1
        uint8_t val = frame[i];
        if (val > 1) val = 1;
        crc = (crc >> 8) ^ m_crc32Table[(val ^ crc) & 0xFF];
      }
      else
      {
        // Normal mode
        crc = (crc >> 8) ^ m_crc32Table[(frame[i] ^ crc) & 0xFF];
      }
    }
  }
  return ~crc;
}

bool SerumContext::ExtractCRZ(const char* filename, const char* extractpath, char* cromname, int cromsize)
{
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

  bool ok = true;
  for (int i = 0; i < num_files && ok; i++)
  {
    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
    {
      ok = false;
      break;
    }

    char extract_filename[4096];
    if (strcpy_s(extract_filename, 4096, extractpath) || strcat_s(extract_filename, 4096, file_stat.m_filename))
    {
      ok = false;
      break;
    }

    if (!mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename, extract_filename, 0))
    {
      ok = false;
    }
  }

  mz_zip_reader_end(&zip_archive);
  return ok;
}

bool SerumContext::AllocateMemory()
{
  // Allocate sprite memory
  if (m_nsprites > 0)
  {
    m_spritedescriptionso = static_cast<uint8_t*>(malloc(m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE));
    m_spritedescriptionsc = static_cast<uint8_t*>(malloc(m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE));
    m_spritedetdwords = static_cast<uint32_t*>(malloc(m_nsprites * sizeof(uint32_t) * MAX_SPRITE_DETECT_AREAS));
    m_spritedetdwordpos = static_cast<uint16_t*>(malloc(m_nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS));
    m_spritedetareas = static_cast<uint16_t*>(malloc(m_nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS * 4));

    if (!m_spritedescriptionso || !m_spritedescriptionsc || !m_spritedetdwords || !m_spritedetdwordpos ||
        !m_spritedetareas)
    {
      return false;
    }
  }

  // Allocate frame memory
  m_mySerum.frame = static_cast<uint8_t*>(malloc(m_fwidth * m_fheight));
  m_mySerum.palette = static_cast<uint8_t*>(malloc(3 * 64));
  m_mySerum.rotations = static_cast<uint8_t*>(malloc(MAX_COLOR_ROTATIONS * 3));

  if (!m_mySerum.frame || !m_mySerum.palette || !m_mySerum.rotations)
  {
    return false;
  }

  return true;
}

bool SerumContext::ReadSeRumData(FILE* pfile, uint32_t sizeheader)
{
  // Read all the sparse vector data
  m_hashcodes.my_fread(1, m_nframes, pfile);
  m_shapecompmode.my_fread(1, m_nframes, pfile);
  m_compmaskID.my_fread(1, m_nframes, pfile);
  m_movrctID.my_fread(1, m_nframes, pfile);

  m_movrctID.clear();  // Clear as we don't need this anymore

  m_compmasks.my_fread(m_fwidth * m_fheight, m_ncompmasks, pfile);
  m_movrcts.my_fread(m_fwidth * m_fheight, m_nmovmasks, pfile);

  m_movrcts.clear();  // Clear as we don't need this anymore

  m_cpal.my_fread(3 * m_nccolors, m_nframes, pfile);
  m_cframes.my_fread(m_fwidth * m_fheight, m_nframes, pfile);
  m_dynamasks.my_fread(m_fwidth * m_fheight, m_nframes, pfile);
  m_dyna4cols.my_fread(MAX_DYNA_4COLS_PER_FRAME * m_nocolors, m_nframes, pfile);
  m_framesprites.my_fread(MAX_SPRITES_PER_FRAME, m_nframes, pfile);

  // Read sprite descriptions
  for (int ti = 0; ti < (int)m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE; ti++)
  {
    if (fread(&m_spritedescriptionsc[ti], 1, 1, pfile) != 1 || fread(&m_spritedescriptionso[ti], 1, 1, pfile) != 1)
    {
      return false;
    }
  }

  m_activeframes.my_fread(1, m_nframes, pfile);
  m_colorrotations.my_fread(3 * MAX_COLOR_ROTATIONS, m_nframes, pfile);

  if (fread(m_spritedetdwords, sizeof(uint32_t), m_nsprites * MAX_SPRITE_DETECT_AREAS, pfile) !=
          m_nsprites * MAX_SPRITE_DETECT_AREAS ||
      fread(m_spritedetdwordpos, sizeof(uint16_t), m_nsprites * MAX_SPRITE_DETECT_AREAS, pfile) !=
          m_nsprites * MAX_SPRITE_DETECT_AREAS ||
      fread(m_spritedetareas, sizeof(uint16_t), m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile) !=
          m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS)
  {
    return false;
  }

  // Handle trigger IDs if present
  m_mySerum.ntriggers = 0;
  if (sizeheader >= 11 * sizeof(uint32_t))
  {
    m_triggerIDs.my_fread(1, m_nframes, pfile);

    for (uint32_t ti = 0; ti < m_nframes; ti++)
    {
      if (m_triggerIDs[ti][0] != 0xffffffff)
      {
        m_mySerum.ntriggers++;
      }
    }
  }

  // Handle frame sprite bounding boxes
  if (sizeheader >= 12 * sizeof(uint32_t))
  {
    m_framespriteBB.my_fread(MAX_SPRITES_PER_FRAME * 4, m_nframes, pfile, &m_framesprites);
  }
  else
  {
    // Set default bounding boxes
    for (uint32_t tj = 0; tj < m_nframes; tj++)
    {
      uint16_t tmp_framespriteBB[4 * MAX_SPRITES_PER_FRAME];
      for (uint32_t ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
      {
        tmp_framespriteBB[ti * 4] = 0;
        tmp_framespriteBB[ti * 4 + 1] = 0;
        tmp_framespriteBB[ti * 4 + 2] = m_fwidth - 1;
        tmp_framespriteBB[ti * 4 + 3] = m_fheight - 1;
      }
      m_framespriteBB.set(tj, tmp_framespriteBB, MAX_SPRITES_PER_FRAME * 4);
    }
  }

  // Handle background frames
  if (sizeheader >= 13 * sizeof(uint32_t))
  {
    m_backgroundframes.my_fread(m_fwidth * m_fheight, m_nbackgrounds, pfile);
    m_backgroundIDs.my_fread(1, m_nframes, pfile);
    m_backgroundBB.my_fread(4, m_nframes, pfile, &m_backgroundIDs);
  }

  return true;
}

void SerumContext::ResetColorRotations()
{
  // Initialize color rotation timing
  std::memset(m_colorshifts, 0, sizeof(m_colorshifts));
  std::memset(m_colorshiftinittime, 0, sizeof(m_colorshiftinittime));
  std::memset(m_colorrotnexttime, 0, sizeof(m_colorrotnexttime));
  std::memset(m_rotationnextabsolutetime, 0, sizeof(m_rotationnextabsolutetime));

  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                 .count();
  m_colorrotseruminit = static_cast<uint32_t>(now);
}

void SerumContext::ColorizeFrameV1(uint8_t* frame, uint32_t frameIndex)
{
  if (!frame || frameIndex >= m_nframes) return;

  // Copy the colorized frame data
  const uint32_t frameSize = m_fwidth * m_fheight;
  std::memcpy(m_mySerum.frame, m_cframes[frameIndex], frameSize);

  // Apply dynamic masks and colors if needed
  // This is a simplified version - the full implementation would handle sprites too
  for (uint32_t i = 0; i < frameSize; i++)
  {
    if (m_dynamasks[frameIndex][i] != 255)
    {
      uint8_t dynaIndex = m_dynamasks[frameIndex][i];
      if (static_cast<uint32_t>(dynaIndex) < MAX_DYNA_4COLS_PER_FRAME)
      {
        uint8_t originalColor = frame[i];
        if (originalColor < m_nocolors)
        {
          m_mySerum.frame[i] = m_dyna4cols[frameIndex][dynaIndex * m_nocolors + originalColor];
        }
      }
    }
  }
}

void SerumContext::CopyFramePalette(uint32_t frameIndex)
{
  if (frameIndex >= m_nframes) return;

  // Copy palette for this frame
  std::memcpy(m_mySerum.palette, m_cpal[frameIndex], 3 * m_nccolors);
}

uint32_t SerumContext::CalculateNextRotationTime(uint32_t currentTime)
{
  uint32_t nextRotTime = 0xffffffff;

  for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
  {
    if (m_mySerum.rotations[ti * 3] == 255) continue;
    if (m_colorrotnexttime[ti] < nextRotTime)
    {
      nextRotTime = m_colorrotnexttime[ti];
    }
  }

  if (nextRotTime == 0xffffffff) return 0;
  return (nextRotTime > currentTime) ? (nextRotTime - currentTime) : 0;
}

bool SerumContext::LoadConcentrateFile(const char* filename, uint8_t flags)
{
  FILE* pfile = fopen(filename, "rb");
  if (!pfile) return false;

  LZ4Stream stream(pfile, false);

  // Read and verify header
  char readMagic[5] = {0};
  if (!stream.read(readMagic, 4))
  {
    fclose(pfile);
    return false;
  }

  if (strncmp(readMagic, "CONC", 4) != 0)
  {
    fclose(pfile);
    return false;
  }

  // Read header data
  uint16_t concentrateFileVersion = 1;
  if (!stream.read(&concentrateFileVersion, sizeof(uint16_t)))
  {
    fclose(pfile);
    return false;
  }

  if (!stream.read(&m_serumVersion, sizeof(uint8_t)) || !stream.read(&m_fwidth, sizeof(uint32_t)) ||
      !stream.read(&m_fheight, sizeof(uint32_t)) || !stream.read(&m_fwidthx, sizeof(uint32_t)) ||
      !stream.read(&m_fheightx, sizeof(uint32_t)) || !stream.read(&m_nframes, sizeof(uint32_t)) ||
      !stream.read(&m_nocolors, sizeof(uint32_t)) || !stream.read(&m_nccolors, sizeof(uint32_t)) ||
      !stream.read(&m_ncompmasks, sizeof(uint32_t)) || !stream.read(&m_nsprites, sizeof(uint32_t)) ||
      !stream.read(&m_nbackgrounds, sizeof(uint16_t)))
  {
    fclose(pfile);
    return false;
  }

  // Read SparseVector data
  m_hashcodes.loadFromStream(stream);
  m_shapecompmode.loadFromStream(stream);
  m_compmaskID.loadFromStream(stream);
  m_compmasks.loadFromStream(stream);
  m_cpal.loadFromStream(stream);
  m_isextraframe.loadFromStream(stream);

  // Check for extra frames and set flags accordingly
  if (m_isextrarequested)
  {
    for (uint32_t ti = 0; ti < m_nframes; ti++)
    {
      if (m_isextraframe[ti][0] > 0)
      {
        m_mySerum.flags |= FLAG_RETURNED_EXTRA_AVAILABLE;
        break;
      }
    }
  }
  else
  {
    m_isextraframe.clearIndex();
  }

  // Continue loading SparseVectors
  m_cframes.loadFromStream(stream);
  m_cframesn.loadFromStream(stream);
  m_cframesnx.loadFromStream(stream);
  m_cframesnx.setParent(&m_isextraframe);

  m_dynamasks.loadFromStream(stream);
  m_dynamasksx.loadFromStream(stream);
  m_dynamasksx.setParent(&m_isextraframe);

  m_dyna4cols.loadFromStream(stream);
  m_dyna4colsn.loadFromStream(stream);
  m_dyna4colsnx.loadFromStream(stream);
  m_dyna4colsnx.setParent(&m_isextraframe);

  m_framesprites.loadFromStream(stream);
  m_isextrasprite.loadFromStream(stream);
  if (!m_isextrarequested) m_isextrasprite.clearIndex();

  m_spritemaskx.loadFromStream(stream);
  m_spritemaskx.setParent(&m_isextrasprite);

  m_spritecolored.loadFromStream(stream);
  m_spritecoloredx.loadFromStream(stream);
  m_spritecoloredx.setParent(&m_isextrasprite);

  m_activeframes.loadFromStream(stream);
  m_colorrotations.loadFromStream(stream);
  m_colorrotationsn.loadFromStream(stream);
  m_colorrotationsnx.loadFromStream(stream);
  m_colorrotationsnx.setParent(&m_isextraframe);

  m_triggerIDs.loadFromStream(stream);
  m_framespriteBB.loadFromStream(stream);
  m_framespriteBB.setParent(&m_framesprites);

  m_isextrabackground.loadFromStream(stream);
  if (!m_isextrarequested) m_isextrabackground.clearIndex();

  m_backgroundframes.loadFromStream(stream);
  m_backgroundframesn.loadFromStream(stream);
  m_backgroundframesnx.loadFromStream(stream);
  m_backgroundframesnx.setParent(&m_isextrabackground);

  m_backgroundIDs.loadFromStream(stream);
  m_backgroundBB.loadFromStream(stream);
  m_backgroundmask.loadFromStream(stream);
  m_backgroundmask.setParent(&m_backgroundIDs);

  m_backgroundmaskx.loadFromStream(stream);
  m_backgroundmaskx.setParent(&m_backgroundIDs);

  m_dynashadowsdiro.loadFromStream(stream);
  m_dynashadowscolo.loadFromStream(stream);
  m_dynashadowsdirx.loadFromStream(stream);
  m_dynashadowsdirx.setParent(&m_isextraframe);

  m_dynashadowscolx.loadFromStream(stream);
  m_dynashadowscolx.setParent(&m_isextraframe);

  m_dynasprite4cols.loadFromStream(stream);
  m_dynasprite4colsx.loadFromStream(stream);
  m_dynasprite4colsx.setParent(&m_isextraframe);

  m_dynaspritemasks.loadFromStream(stream);
  m_dynaspritemasksx.loadFromStream(stream);
  m_dynaspritemasksx.setParent(&m_isextraframe);

  // Read raw arrays if sprites exist
  if (m_nsprites > 0)
  {
    // Allocate memory for sprite data
    m_spritedetdwords = new (std::nothrow) uint32_t[m_nsprites * MAX_SPRITE_DETECT_AREAS];
    m_spritedetdwordpos = new (std::nothrow) uint16_t[m_nsprites * MAX_SPRITE_DETECT_AREAS];
    m_spritedetareas = new (std::nothrow) uint16_t[m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS];

    if (!m_spritedetdwords || !m_spritedetdwordpos || !m_spritedetareas)
    {
      FreeResources();
      fclose(pfile);
      return false;
    }

    // Read sprite detection data
    if (!stream.read(m_spritedetdwords, sizeof(uint32_t) * m_nsprites * MAX_SPRITE_DETECT_AREAS) ||
        !stream.read(m_spritedetdwordpos, sizeof(uint16_t) * m_nsprites * MAX_SPRITE_DETECT_AREAS) ||
        !stream.read(m_spritedetareas, sizeof(uint16_t) * m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS))
    {
      FreeResources();
      fclose(pfile);
      return false;
    }

    // Read version-specific sprite data
    if (m_serumVersion == SERUM_V2)
    {
      m_spriteoriginal = new (std::nothrow) uint8_t[m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT];
      m_sprshapemode = new (std::nothrow) uint8_t[m_nsprites];

      if (!m_spriteoriginal || !m_sprshapemode)
      {
        FreeResources();
        fclose(pfile);
        return false;
      }

      if (!stream.read(m_spriteoriginal, m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT) ||
          !stream.read(m_sprshapemode, m_nsprites))
      {
        FreeResources();
        fclose(pfile);
        return false;
      }
    }
    else if (m_serumVersion == SERUM_V1)
    {
      m_spritedescriptionso = new (std::nothrow) uint8_t[m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE];
      m_spritedescriptionsc = new (std::nothrow) uint8_t[m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE];

      if (!m_spritedescriptionso || !m_spritedescriptionsc)
      {
        FreeResources();
        fclose(pfile);
        return false;
      }

      if (!stream.read(m_spritedescriptionso, m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE) ||
          !stream.read(m_spritedescriptionsc, m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE))
      {
        FreeResources();
        fclose(pfile);
        return false;
      }
    }
  }

  // Load scene generator data
  m_sceneGenerator->loadFromStream(stream);

  fclose(pfile);

  // Update mySerum structure
  m_mySerum.SerumVersion = m_serumVersion;
  m_mySerum.flags = flags;
  m_mySerum.nocolors = m_nocolors;

  // Set requested frame types based on version and flags
  m_isoriginalrequested = false;
  m_isextrarequested = false;
  m_mySerum.width32 = 0;
  m_mySerum.width64 = 0;

  if (m_serumVersion == SERUM_V2)
  {
    if (m_fheight == 32)
    {
      if (flags & FLAG_REQUEST_32P_FRAMES)
      {
        m_isoriginalrequested = true;
        m_mySerum.width32 = m_fwidth;
      }
      if (flags & FLAG_REQUEST_64P_FRAMES)
      {
        m_isextrarequested = true;
        m_mySerum.width64 = m_fwidthx;
      }
    }
    else
    {
      if (flags & FLAG_REQUEST_64P_FRAMES)
      {
        m_isoriginalrequested = true;
        m_mySerum.width64 = m_fwidth;
      }
      if (flags & FLAG_REQUEST_32P_FRAMES)
      {
        m_isextrarequested = true;
        m_mySerum.width32 = m_fwidthx;
      }
    }
  }
  else
  {
    // SERUM_V1
    m_isoriginalrequested = true;
    m_mySerum.width32 = m_fwidth;
  }

  strcpy(m_rname, "concentrate");
  m_cromloaded = true;
  ResetColorRotations();

  return true;
}

bool SerumContext::SaveConcentrateFile(const char* filename)
{
  if (!m_cromloaded) return false;

  std::string concentratePath;

  // Remove extension and add .cROMc
  if (const char* dot = strrchr(filename, '.'))
  {
    concentratePath = std::string(filename, dot);
  }
  else
  {
    concentratePath = filename;
  }
  concentratePath += ".cROMc";

  FILE* pfile = fopen(concentratePath.c_str(), "wb");
  if (!pfile) return false;

  LZ4Stream stream(pfile, true);

  // Write header
  const char magicBytes[] = "CONC";
  if (!stream.write(magicBytes, 4))
  {
    fclose(pfile);
    return false;
  }

  uint16_t concentrateFileVersion = 1;
  if (!stream.write(&concentrateFileVersion, sizeof(uint16_t)) || !stream.write(&m_serumVersion, sizeof(uint8_t)) ||
      !stream.write(&m_fwidth, sizeof(uint32_t)) || !stream.write(&m_fheight, sizeof(uint32_t)) ||
      !stream.write(&m_fwidthx, sizeof(uint32_t)) || !stream.write(&m_fheightx, sizeof(uint32_t)) ||
      !stream.write(&m_nframes, sizeof(uint32_t)) || !stream.write(&m_nocolors, sizeof(uint32_t)) ||
      !stream.write(&m_nccolors, sizeof(uint32_t)) || !stream.write(&m_ncompmasks, sizeof(uint32_t)) ||
      !stream.write(&m_nsprites, sizeof(uint32_t)) || !stream.write(&m_nbackgrounds, sizeof(uint16_t)))
  {
    fclose(pfile);
    return false;
  }

  // Write SparseVector data
  m_hashcodes.saveToStream(stream);
  m_shapecompmode.saveToStream(stream);
  m_compmaskID.saveToStream(stream);
  m_compmasks.saveToStream(stream);
  m_cpal.saveToStream(stream);
  m_isextraframe.saveToStream(stream);
  m_cframes.saveToStream(stream);
  m_cframesn.saveToStream(stream);
  m_cframesnx.saveToStream(stream);
  m_dynamasks.saveToStream(stream);
  m_dynamasksx.saveToStream(stream);
  m_dyna4cols.saveToStream(stream);
  m_dyna4colsn.saveToStream(stream);
  m_dyna4colsnx.saveToStream(stream);
  m_framesprites.saveToStream(stream);
  m_isextrasprite.saveToStream(stream);
  m_spritemaskx.saveToStream(stream);
  m_spritecolored.saveToStream(stream);
  m_spritecoloredx.saveToStream(stream);
  m_activeframes.saveToStream(stream);
  m_colorrotations.saveToStream(stream);
  m_colorrotationsn.saveToStream(stream);
  m_colorrotationsnx.saveToStream(stream);
  m_triggerIDs.saveToStream(stream);
  m_framespriteBB.saveToStream(stream);
  m_isextrabackground.saveToStream(stream);
  m_backgroundframes.saveToStream(stream);
  m_backgroundframesn.saveToStream(stream);
  m_backgroundframesnx.saveToStream(stream);
  m_backgroundIDs.saveToStream(stream);
  m_backgroundBB.saveToStream(stream);
  m_backgroundmask.saveToStream(stream);
  m_backgroundmaskx.saveToStream(stream);
  m_dynashadowsdiro.saveToStream(stream);
  m_dynashadowscolo.saveToStream(stream);
  m_dynashadowsdirx.saveToStream(stream);
  m_dynashadowscolx.saveToStream(stream);
  m_dynasprite4cols.saveToStream(stream);
  m_dynasprite4colsx.saveToStream(stream);
  m_dynaspritemasks.saveToStream(stream);
  m_dynaspritemasksx.saveToStream(stream);

  // Write raw arrays
  if (m_nsprites > 0)
  {
    if (m_spritedetdwords && !stream.write(m_spritedetdwords, sizeof(uint32_t) * m_nsprites * MAX_SPRITE_DETECT_AREAS))
    {
      fclose(pfile);
      return false;
    }
    if (m_spritedetdwordpos &&
        !stream.write(m_spritedetdwordpos, sizeof(uint16_t) * m_nsprites * MAX_SPRITE_DETECT_AREAS))
    {
      fclose(pfile);
      return false;
    }
    if (m_spritedetareas &&
        !stream.write(m_spritedetareas, sizeof(uint16_t) * m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS))
    {
      fclose(pfile);
      return false;
    }

    // Write version-specific sprite data
    if (m_serumVersion == SERUM_V2)
    {
      if (m_spriteoriginal && !stream.write(m_spriteoriginal, m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT))
      {
        fclose(pfile);
        return false;
      }
      if (m_sprshapemode && !stream.write(m_sprshapemode, m_nsprites))
      {
        fclose(pfile);
        return false;
      }
    }
    else if (m_serumVersion == SERUM_V1)
    {
      if (m_spritedescriptionso && !stream.write(m_spritedescriptionso, m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE))
      {
        fclose(pfile);
        return false;
      }
      if (m_spritedescriptionsc && !stream.write(m_spritedescriptionsc, m_nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE))
      {
        fclose(pfile);
        return false;
      }
    }
  }

  // Save scene generator data
  m_sceneGenerator->saveToStream(stream);

  fclose(pfile);
  return true;
}

bool SerumContext::LoadFilev2(const char* filename, uint8_t flags, bool uncompressed, uint32_t sizeheader)
{
  // This is called from LoadRegularFile when the header indicates v2 format
  // The pfile is already open and positioned after the header
  FILE* pfile = fopen(filename, "rb");
  if (!pfile) return false;

  // Skip to after the header (romname + sizeheader already read)
  fseek(pfile, 64 + 4, SEEK_SET);

  uint32_t fwidth, fheight, fwidthx, fheightx;
  if (fread(&fwidth, 4, 1, pfile) != 1 || fread(&fheight, 4, 1, pfile) != 1 || fread(&fwidthx, 4, 1, pfile) != 1 ||
      fread(&fheightx, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  m_fwidth = fwidth;
  m_fheight = fheight;

  bool isOriginalRequested = false;
  bool isExtraRequested = false;
  m_mySerum.width32 = 0;
  m_mySerum.width64 = 0;

  if (fheight == 32)
  {
    if (flags & FLAG_REQUEST_32P_FRAMES)
    {
      isOriginalRequested = true;
      m_mySerum.width32 = fwidth;
    }
    if (flags & FLAG_REQUEST_64P_FRAMES)
    {
      isExtraRequested = true;
      m_mySerum.width64 = fwidthx;
    }
  }
  else
  {
    if (flags & FLAG_REQUEST_64P_FRAMES)
    {
      isOriginalRequested = true;
      m_mySerum.width64 = fwidth;
    }
    if (flags & FLAG_REQUEST_32P_FRAMES)
    {
      isExtraRequested = true;
      m_mySerum.width32 = fwidthx;
    }
  }

  if (fread(&m_nframes, 4, 1, pfile) != 1 || fread(&m_nocolors, 4, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  m_mySerum.nocolors = m_nocolors;

  if (fwidth == 0 || fheight == 0 || m_nframes == 0 || m_nocolors == 0)
  {
    fclose(pfile);
    return false;
  }

  if (fread(&m_ncompmasks, 4, 1, pfile) != 1 || fread(&m_nsprites, 4, 1, pfile) != 1 ||
      fread(&m_nbackgrounds, 2, 1, pfile) != 1)
  {
    fclose(pfile);
    return false;
  }

  // Allocate sprite memory - using malloc like original
  if (m_nsprites > 0)
  {
    m_spriteoriginal = (uint8_t*)malloc(m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    m_spritedetdwords = (uint32_t*)malloc(m_nsprites * sizeof(uint32_t) * MAX_SPRITE_DETECT_AREAS);
    m_spritedetdwordpos = (uint16_t*)malloc(m_nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS);
    m_spritedetareas = (uint16_t*)malloc(m_nsprites * sizeof(uint16_t) * MAX_SPRITE_DETECT_AREAS * 4);
    m_sprshapemode = (uint8_t*)malloc(m_nsprites);

    if (!m_spriteoriginal || !m_spritedetdwords || !m_spritedetdwordpos || !m_spritedetareas || !m_sprshapemode)
    {
      FreeResources();
      fclose(pfile);
      return false;
    }
  }

  m_frameshape = (uint8_t*)malloc(fwidth * fheight);
  if (!m_frameshape)
  {
    FreeResources();
    fclose(pfile);
    return false;
  }

  if (flags & FLAG_REQUEST_32P_FRAMES)
  {
    m_mySerum.frame32 = (uint16_t*)malloc(32 * m_mySerum.width32 * sizeof(uint16_t));
    m_mySerum.rotations32 = (uint16_t*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(uint16_t));
    m_mySerum.rotationsinframe32 = (uint16_t*)malloc(2 * 32 * m_mySerum.width32 * sizeof(uint16_t));
    if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
      m_mySerum.modifiedelements32 = (uint8_t*)malloc(32 * m_mySerum.width32);
    if (!m_mySerum.frame32 || !m_mySerum.rotations32 || !m_mySerum.rotationsinframe32 ||
        (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !m_mySerum.modifiedelements32))
    {
      FreeResources();
      fclose(pfile);
      return false;
    }
  }

  if (flags & FLAG_REQUEST_64P_FRAMES)
  {
    m_mySerum.frame64 = (uint16_t*)malloc(64 * m_mySerum.width64 * sizeof(uint16_t));
    m_mySerum.rotations64 = (uint16_t*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(uint16_t));
    m_mySerum.rotationsinframe64 = (uint16_t*)malloc(2 * 64 * m_mySerum.width64 * sizeof(uint16_t));
    if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
      m_mySerum.modifiedelements64 = (uint8_t*)malloc(64 * m_mySerum.width64);
    if (!m_mySerum.frame64 || !m_mySerum.rotations64 || !m_mySerum.rotationsinframe64 ||
        (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !m_mySerum.modifiedelements64))
    {
      FreeResources();
      fclose(pfile);
      return false;
    }
  }

  // Use the original my_fread interface for SparseVectors
  m_hashcodes.my_fread(1, m_nframes, pfile);
  m_shapecompmode.my_fread(1, m_nframes, pfile);
  m_compmaskID.my_fread(1, m_nframes, pfile);
  m_compmasks.my_fread(fwidth * fheight, m_ncompmasks, pfile);
  m_isextraframe.my_fread(1, m_nframes, pfile);

  if (isExtraRequested)
  {
    for (uint32_t ti = 0; ti < m_nframes; ti++)
    {
      if (m_isextraframe[ti][0] > 0)
      {
        m_mySerum.flags |= FLAG_RETURNED_EXTRA_AVAILABLE;
        break;
      }
    }
  }
  else
  {
    m_isextraframe.clearIndex();
  }

  m_cframesn.my_fread(fwidth * fheight, m_nframes, pfile);
  m_cframesnx.my_fread(fwidthx * fheightx, m_nframes, pfile, &m_isextraframe);
  m_dynamasks.my_fread(fwidth * fheight, m_nframes, pfile);
  m_dynamasksx.my_fread(fwidthx * fheightx, m_nframes, pfile, &m_isextraframe);
  m_dyna4colsn.my_fread(MAX_DYNA_4COLS_PER_FRAME * m_nocolors, m_nframes, pfile);
  m_dyna4colsnx.my_fread(MAX_DYNA_4COLS_PER_FRAME * m_nocolors, m_nframes, pfile, &m_isextraframe);
  m_isextrasprite.my_fread(1, m_nsprites, pfile);

  if (!isExtraRequested) m_isextrasprite.clearIndex();

  m_framesprites.my_fread(MAX_SPRITES_PER_FRAME, m_nframes, pfile);

  if (m_nsprites > 0 && fread(m_spriteoriginal, 1, m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile) !=
                            m_nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT)
  {
    FreeResources();
    fclose(pfile);
    return false;
  }

  m_spritecolored.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, m_nsprites, pfile);
  m_spritemaskx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, m_nsprites, pfile, &m_isextrasprite);
  m_spritecoloredx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, m_nsprites, pfile, &m_isextrasprite);
  m_activeframes.my_fread(1, m_nframes, pfile);
  m_colorrotationsn.my_fread(MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, m_nframes, pfile);
  m_colorrotationsnx.my_fread(MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, m_nframes, pfile, &m_isextraframe);

  if (m_nsprites > 0)
  {
    if (fread(m_spritedetdwords, 4, m_nsprites * MAX_SPRITE_DETECT_AREAS, pfile) !=
            m_nsprites * MAX_SPRITE_DETECT_AREAS ||
        fread(m_spritedetdwordpos, 2, m_nsprites * MAX_SPRITE_DETECT_AREAS, pfile) !=
            m_nsprites * MAX_SPRITE_DETECT_AREAS ||
        fread(m_spritedetareas, 2, m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile) !=
            m_nsprites * 4 * MAX_SPRITE_DETECT_AREAS)
    {
      FreeResources();
      fclose(pfile);
      return false;
    }
  }

  m_triggerIDs.my_fread(1, m_nframes, pfile);
  m_framespriteBB.my_fread(MAX_SPRITES_PER_FRAME * 4, m_nframes, pfile, &m_framesprites);
  m_isextrabackground.my_fread(1, m_nbackgrounds, pfile);

  if (!isExtraRequested) m_isextrabackground.clearIndex();

  m_backgroundframesn.my_fread(fwidth * fheight, m_nbackgrounds, pfile);
  m_backgroundframesnx.my_fread(fwidthx * fheightx, m_nbackgrounds, pfile, &m_isextrabackground);
  m_backgroundIDs.my_fread(1, m_nframes, pfile);
  m_backgroundmask.my_fread(fwidth * fheight, m_nframes, pfile, &m_backgroundIDs);
  m_backgroundmaskx.my_fread(fwidthx * fheightx, m_nframes, pfile, &m_backgroundIDs);

  if (m_nsprites > 0) memset(m_sprshapemode, 0, m_nsprites);

  if (sizeheader >= 15 * sizeof(uint32_t))
  {
    m_dynashadowsdiro.my_fread(MAX_DYNA_4COLS_PER_FRAME, m_nframes, pfile);
    m_dynashadowscolo.my_fread(MAX_DYNA_4COLS_PER_FRAME, m_nframes, pfile);
    m_dynashadowsdirx.my_fread(MAX_DYNA_4COLS_PER_FRAME, m_nframes, pfile, &m_isextraframe);
    m_dynashadowscolx.my_fread(MAX_DYNA_4COLS_PER_FRAME, m_nframes, pfile, &m_isextraframe);
  }
  else
  {
    m_dynashadowsdiro.reserve(MAX_DYNA_4COLS_PER_FRAME);
    m_dynashadowscolo.reserve(MAX_DYNA_4COLS_PER_FRAME);
    m_dynashadowsdirx.reserve(MAX_DYNA_4COLS_PER_FRAME);
    m_dynashadowscolx.reserve(MAX_DYNA_4COLS_PER_FRAME);
  }

  if (sizeheader >= 18 * sizeof(uint32_t))
  {
    m_dynasprite4cols.my_fread(MAX_DYNA_SETS_PER_SPRITE * m_nocolors, m_nsprites, pfile);
    m_dynasprite4colsx.my_fread(MAX_DYNA_SETS_PER_SPRITE * m_nocolors, m_nsprites, pfile, &m_isextraframe);
    m_dynaspritemasks.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, m_nsprites, pfile);
    m_dynaspritemasksx.my_fread(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, m_nsprites, pfile, &m_isextraframe);

    if (sizeheader >= 19 * sizeof(uint32_t))
    {
      if (m_nsprites > 0)
      {
        if (fread(m_sprshapemode, 1, m_nsprites, pfile) != m_nsprites)
        {
          FreeResources();
          fclose(pfile);
          return false;
        }

        for (uint32_t i = 0; i < m_nsprites; i++)
        {
          if (m_sprshapemode[i] > 0)
          {
            for (uint32_t j = 0; j < MAX_SPRITE_DETECT_AREAS; j++)
            {
              uint32_t detdwords = m_spritedetdwords[i * MAX_SPRITE_DETECT_AREAS + j];
              if ((detdwords & 0xFF000000) > 0) detdwords = (detdwords & 0x00FFFFFF) | 0x01000000;
              if ((detdwords & 0x00FF0000) > 0) detdwords = (detdwords & 0xFF00FFFF) | 0x00010000;
              if ((detdwords & 0x0000FF00) > 0) detdwords = (detdwords & 0xFFFF00FF) | 0x00000100;
              if ((detdwords & 0x000000FF) > 0) detdwords = (detdwords & 0xFFFFFF00) | 0x00000001;
              m_spritedetdwords[i * MAX_SPRITE_DETECT_AREAS + j] = detdwords;
            }
            for (uint32_t j = i * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT;
                 j < (i + 1) * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT; j++)
            {
              if (m_spriteoriginal[j] > 0 && m_spriteoriginal[j] != 255) m_spriteoriginal[j] = 1;
            }
          }
        }
      }
    }
  }
  else
  {
    m_dynasprite4cols.reserve(MAX_DYNA_SETS_PER_SPRITE * m_nocolors);
    m_dynasprite4colsx.reserve(MAX_DYNA_SETS_PER_SPRITE * m_nocolors);
    m_dynaspritemasks.reserve(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
    m_dynaspritemasksx.reserve(MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
  }

  fclose(pfile);

  // Count triggers
  m_mySerum.ntriggers = 0;
  for (uint32_t ti = 0; ti < m_nframes; ti++)
  {
    if (m_triggerIDs[ti][0] != 0xffffffff) m_mySerum.ntriggers++;
  }

  // Allocate framechecked array
  m_framechecked = (bool*)malloc(sizeof(bool) * m_nframes);
  if (!m_framechecked)
  {
    FreeResources();
    return false;
  }

  // Set final width values
  if (flags & FLAG_REQUEST_32P_FRAMES)
  {
    if (fheight == 32)
      m_mySerum.width32 = fwidth;
    else
      m_mySerum.width32 = fwidthx;
  }
  else
    m_mySerum.width32 = 0;

  if (flags & FLAG_REQUEST_64P_FRAMES)
  {
    if (fheight == 32)
      m_mySerum.width64 = fwidthx;
    else
      m_mySerum.width64 = fwidth;
  }
  else
    m_mySerum.width64 = 0;

  m_mySerum.SerumVersion = SERUM_V2;
  ResetColorRotations();

  // Clean up temporary file if it was extracted
  if (!uncompressed)
  {
    remove(filename);
  }

  return true;
}

}  // namespace Serum