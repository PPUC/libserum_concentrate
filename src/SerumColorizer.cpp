#include <sstream>

#include "SerumColorizer.h"
#include "SerumContext.h"
#include "serum-version.h"

namespace Serum
{

Serum::Serum() : m_context(std::make_unique<SerumContext>()), m_loaded(false) {}

Serum::~Serum() = default;

bool Serum::Load(const std::string& altcolorpath, const std::string& romname, uint8_t flags)
{
  if (m_loaded)
  {
    Dispose();
  }

  auto* result = m_context->Load(altcolorpath.c_str(), romname.c_str(), flags);
  m_loaded = (result != nullptr);
  return m_loaded;
}

void Serum::Dispose()
{
  if (m_loaded)
  {
    m_context->Dispose();
    m_loaded = false;
  }
}

uint32_t Serum::Colorize(uint8_t* frame)
{
  if (!m_loaded || !frame)
  {
    return static_cast<uint32_t>(0xffffffff);
  }
  return m_context->Colorize(frame);
}

uint32_t Serum::Rotate()
{
  if (!m_loaded)
  {
    return static_cast<uint32_t>(0);
  }
  return m_context->Rotate();
}

void Serum::EnableColorization() { m_context->EnableColorization(); }

void Serum::DisableColorization() { m_context->DisableColorization(); }

bool Serum::IsColorizationEnabled() const { return true; }

void Serum::SetIgnoreUnknownFramesTimeout(uint16_t milliseconds)
{
  m_context->SetIgnoreUnknownFramesTimeout(milliseconds);
}

void Serum::SetMaximumUnknownFramesToSkip(uint8_t maximum) { m_context->SetMaximumUnknownFramesToSkip(maximum); }

void Serum::SetStandardPalette(const uint8_t* palette, int bitDepth)
{
  m_context->SetStandardPalette(palette, bitDepth);
}

const Serum_Frame_Struc* Serum::GetFrameData() const
{
  if (m_loaded && m_context)
  {
    return &(m_context->m_mySerum);
  }
  return nullptr;
}

bool Serum::IsLoaded() const { return m_loaded; }

uint8_t Serum::GetSerumVersion() const { return static_cast<uint8_t>(0); }

uint32_t Serum::GetFrameWidth() const { return static_cast<uint32_t>(0); }

uint32_t Serum::GetFrameHeight() const { return static_cast<uint32_t>(0); }

uint32_t Serum::GetFrameCount() const { return static_cast<uint32_t>(0); }

std::string Serum::GetVersion()
{
  std::ostringstream oss;
  oss << SERUM_VERSION_MAJOR << "." << SERUM_VERSION_MINOR << "." << SERUM_VERSION_PATCH;
  return oss.str();
}

std::string Serum::GetMinorVersion()
{
  std::ostringstream oss;
  oss << SERUM_VERSION_MAJOR << "." << SERUM_VERSION_MINOR;
  return oss.str();
}

// Scene API
bool Serum::Scene_ParseCSV(const char* csv_filename)
{
  if (!m_context || !m_context->m_sceneGenerator) return false;
  return m_context->GetSceneGenerator().parseCSV(csv_filename);
}

bool Serum::Scene_GenerateDump(const char* dump_filename, int id)
{
  if (!m_context || !m_context->m_sceneGenerator) return false;
  return m_context->GetSceneGenerator().generateDump(dump_filename, id);
}

bool Serum::Scene_GetInfo(uint16_t sceneId, uint16_t* frameCount, uint16_t* durationPerFrame, bool* interruptable,
                          bool* startImmediately, uint8_t* repeat, uint8_t* endFrame)
{
  if (!m_context || !m_context->m_sceneGenerator) return false;
  return m_context->GetSceneGenerator().getSceneInfo(sceneId, *frameCount, *durationPerFrame, *interruptable,
                                                     *startImmediately, *repeat, *endFrame);
}

bool Serum::Scene_GenerateFrame(uint16_t sceneId, uint16_t frameIndex, uint8_t* buffer, int group)
{
  if (!m_context || !m_context->m_sceneGenerator) return false;
  return m_context->GetSceneGenerator().generateFrame(sceneId, frameIndex, buffer, group);
}

void Serum::Scene_SetDepth(uint8_t depth)
{
  if (m_context && m_context->m_sceneGenerator) m_context->GetSceneGenerator().setDepth(depth);
}

int Serum::Scene_GetDepth()
{
  if (!m_context || !m_context->m_sceneGenerator) return 0;
  return m_context->GetSceneGenerator().getDepth();
}

bool Serum::Scene_IsActive()
{
  if (!m_context || !m_context->m_sceneGenerator) return false;
  return m_context->GetSceneGenerator().isActive();
}

void Serum::Scene_Reset()
{
  if (m_context && m_context->m_sceneGenerator) m_context->GetSceneGenerator().Reset();
}

}  // namespace Serum