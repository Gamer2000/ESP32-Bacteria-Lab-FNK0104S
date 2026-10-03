#include "Controls.h"

#include "Config.h"

namespace BacteriaLab {

namespace {
constexpr int8_t TOUCH_SDA = 16;
constexpr int8_t TOUCH_SCL = 15;
constexpr int8_t TOUCH_RESET = 18;
constexpr int8_t TOUCH_INTERRUPT = 17;
}

Controls::Controls()
    : touch_(TOUCH_SDA, TOUCH_SCL, TOUCH_RESET, TOUCH_INTERRUPT) {}

void Controls::begin() {
  touch_.begin();
}

ControlEvent Controls::poll() {
  const FT6336U_TouchPointType points = touch_.scan();
  if (points.touch_count == 0) {
    touchHeld_ = false;
    return {};
  }
  if (touchHeld_) return {};
  touchHeld_ = true;

  // FT6336U reports portrait coordinates; TFT rotation 1 is landscape.
  const int16_t x = static_cast<int16_t>(points.tp[0].y);
  const int16_t y = static_cast<int16_t>(Config::SCREEN_HEIGHT - 1 - points.tp[0].x);
  if (x < 0 || x >= Config::SCREEN_WIDTH || y < 0 || y >= Config::SCREEN_HEIGHT) return {};

  if (y >= Config::CONTROLS_TOP) {
    if (x >= 6 && x < 154) return {ControlAction::ToggleRun, x, y};
    if (x >= 166 && x < 314) return {ControlAction::Reset, x, y};
    if (x >= 326 && x < 474) return {ControlAction::ToggleRecord, x, y};
    return {};
  }
  return {};
}

}  // namespace BacteriaLab
