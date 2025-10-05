#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/access.hpp>
#include "LZ4Stream.h"

struct SceneData
{
  uint16_t sceneId;
  uint16_t frameCount;
  uint16_t durationPerFrame;
  bool interruptable = false;
  bool immediateStart = false;
  uint8_t repeat = 0; // 0 - play once, 1 - loop, >= 2 - repeat x times
  uint8_t frameGroups = 0;
  uint8_t currentGroup = 0; // Current group for frame generation, used internally
  bool random = false;
  uint8_t autoStart = 0; // 0 - no autostart, >= 1 - start this scene after x seconds of inactivity (no new frames), only
                         // use once, could be combined with frame groups
  uint8_t endFrame = 0;  // 0 - when scene is finished, show last frame of the scene until a new frame is matched, 1 - black
                         // screen, 2 - show last frame before scene started

  // Serialisierungsfunktion für cereal
  template <class Archive>
  void serialize(Archive &ar)
  {
    ar(sceneId,
       frameCount,
       durationPerFrame,
       interruptable,
       immediateStart,
       repeat,
       frameGroups,
       currentGroup,
       random,
       autoStart,
       endFrame);
  }
};

class SceneGenerator
{
public:
  SceneGenerator();

  bool parseCSV(const std::string &csv_filename);
  bool generateDump(const std::string &dump_filename, int id = -1);
  bool getSceneInfo(uint16_t sceneId, uint16_t &frameCount, uint16_t &durationPerFrame, bool &interruptable, bool &startImmediately,
                    uint8_t &repeat, uint8_t &endFrame) const;
  bool getAutoStartSceneInfo(uint16_t &frameCount, uint16_t &durationPerFrame, bool &interruptable, bool &startImmediately,
                             uint8_t &repeat, uint8_t &endFrame) const;
  bool generateFrame(uint16_t sceneId, uint16_t frameIndex, uint8_t *buffer, int group = -1);
  void setDepth(uint8_t depth);
  int getDepth() const { return m_depth; }
  bool isActive() const { return m_active; }
  uint32_t getAutoStartTimer() const { return 1000 * m_autoStartTimer; }
  void Reset()
  {
    m_sceneData.clear();
    m_autoStartTimer = 0;
    m_autoStartSceneId = 0;
    m_depth = 2;
    m_templateInitialized = false;
    initializeTemplate();
    m_active = false;
  }
  void setSceneData(const std::vector<SceneData> &data)
  {
    Reset();
    m_sceneData = data;
    m_active = !m_sceneData.empty();
  }
  const std::vector<SceneData> &getSceneData() const { return m_sceneData; }

private:
  std::vector<SceneData> m_sceneData;

  const unsigned char *getCharFont(char c) const;
  void renderChar(uint8_t *buffer, char c, uint8_t x, uint8_t y) const;
  void renderString(uint8_t *buffer, const std::string &str, uint8_t x, uint8_t y) const;

  struct TextTemplate
  {
    uint8_t fullFrame[4096];
  };
  TextTemplate m_template;
  bool m_templateInitialized;

  void initializeTemplate();

  uint8_t m_depth = 2;   // Default depth for rendering
  bool m_active = false; // Is the scene generator active?

  uint8_t m_autoStartTimer = 0;    // Timer for auto-start scenes
  uint16_t m_autoStartSceneId = 0; // Scene ID to auto-start
};
