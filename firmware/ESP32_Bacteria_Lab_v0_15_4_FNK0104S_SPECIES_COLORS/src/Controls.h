#pragma once

#include <FT6336U.h>
#include <stdint.h>

namespace BacteriaLab {

enum class ControlAction : uint8_t {
  None,
  ToggleRun,
  Reset,
  ToggleRecord
};

struct ControlEvent {
  ControlAction action;
  int16_t x;
  int16_t y;

  ControlEvent(ControlAction eventAction = ControlAction::None,
               int16_t eventX = 0,
               int16_t eventY = 0)
      : action(eventAction), x(eventX), y(eventY) {}
};

class Controls {
 public:
  Controls();
  void begin();
  ControlEvent poll();

 private:
  FT6336U touch_;
  bool touchHeld_ = false;
};

}  // namespace BacteriaLab
