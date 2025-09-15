#include <iostream>

#include "serum-decode.h"

int main(int argc, const char* argv[])
{
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " [path] [rom]" << std::endl;
    return 1;
  }

  const char* path = argv[1];
  const char* rom = argv[2];

  // Test C++ API - multiple instances supported
  Serum::Serum serum1;
  Serum::Serum serum2;

  // Test loading serum file
  if (!serum1.Load(path, rom))
  {
    std::cout << "Failed to load Serum: path=" << path << ", rom=" << rom << std::endl;
    return 1;
  }

  std::cout << "Serum successfully loaded: path=" << path << ", rom=" << rom << std::endl;
  std::cout << "Version: " << Serum::Serum::GetVersion() << std::endl;
  std::cout << "IsLoaded: " << (serum1.IsLoaded() ? "true" : "false") << std::endl;

  // Test frame data access
  const auto* frameData = serum1.GetFrameData();
  if (frameData)
  {
    std::cout << "Frame data available - SerumVersion: " << static_cast<int>(frameData->SerumVersion)
              << ", nocolors: " << frameData->nocolors << std::endl;
  }

  // Test multiple instances
  if (serum2.Load(path, rom))
  {
    std::cout << "Second instance also loaded successfully" << std::endl;
  }

  // Test scene generator (using C API functions)
  std::cout << "Scene generator depth: " << Serum_Scene_GetDepth() << std::endl;

  // Disposal happens automatically via destructors
  return 0;
}