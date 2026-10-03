#pragma once

#include <TFT_eSPI.h>

#include "World.h"

namespace BacteriaLab {

class Display {
 public:
  Display(TFT_eSPI& tft, TFT_eSprite& frame) : tft_(tft), frame_(frame) {}
  bool begin();
  void render(const World& world, bool running, bool recording,
              uint32_t recordingSeconds);

 private:
  TFT_eSPI& tft_;
  TFT_eSprite& frame_;
  void drawButton(int16_t x, int16_t width, const char* label,
                  uint16_t color, bool active);
};

}  // namespace BacteriaLab
