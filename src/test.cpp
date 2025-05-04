#include <iostream>

#include "serum-decode.h"

int main(int argc, const char* argv[]) {
  int width;
  int height;

  /*unsigned int numColors;
  unsigned int numTriggers;

  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " [path] [rom]" << std::endl;

    return 1;
  }

  const char* path = argv[1];
  const char* rom = argv[2];

  if (!Serum_Load(path, rom, &width, &height, &numColors, &numTriggers)) {
    std::cout << "Failed to load Serum: path=" << path << ", rom=" << rom
              << std::endl;

    Serum_Dispose();
    
    return 1;
  }

  std::cout << "Serum successfully loaded: path=" << path << ", rom=" << rom
            << ", width=" << width << ", height=" << height
            << ", numColors=" << numColors << ", numTriggers=" << numTriggers
            << std::endl;

  Serum_Dispose();*/

  return 0;
}