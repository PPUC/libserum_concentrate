#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SceneData
{
  int sceneId;
  int frameCount;
  int durationPerFrame;
  bool interruptable = false;
  bool immediateStart = false;
  int repeat = 0;  // 0 - play once, 1 - loop, >= 2 - repeat x times
  int frameGroups = 0;
  int currentGroup = 0;  // Current group for frame generation, used internally
  bool random = false;
  int autoStart = 0;  // 0 - no autostart, >= 1 - start this scene after x seconds of inactivity (no new frames), only
                      // use once, could be combined with frame groups
  int endFrame = 0;  // 0 - when scene is finished, show last frame of the scene until a new frame is matched, 1 - black
                     // screen, 2 - show last frame before scene started
};

class SceneGenerator
{
 public:
  SceneGenerator();

  bool parseCSV(const std::string& csv_filename);
  bool generateDump(const std::string& dump_filename, int id = -1);
  uint16_t getSceneId(char source, int event, int value) const;
  bool getSceneInfo(int sceneId, int& frameCount, int& durationPerFrame, bool& interruptable, bool& startImmediately,
                    int& repeat, int& endFrame) const;
  bool generateFrame(int sceneId, int frameIndex, uint8_t* buffer, int group = -1);
  void setDepth(int depth);
  int getDepth() const { return m_depth; }
  bool isActive() const { return m_active; }
  void Reset()
  {
    m_sceneData.clear();
    m_templateInitialized = false;
    initializeTemplate();
    m_active = false;
  }

 private:
  std::vector<SceneData> m_sceneData;

  const unsigned char* getCharFont(char c) const;
  void renderChar(uint8_t* buffer, char c, int x, int y) const;
  void renderString(uint8_t* buffer, const std::string& str, int x, int y) const;

  struct TextTemplate
  {
    uint8_t fullFrame[4096];
  };
  TextTemplate m_template;
  bool m_templateInitialized;

  void initializeTemplate();

  int m_depth = 2;  // Default depth for rendering
  bool m_active = false;  // Is the scene generator active?
};
