#include "Display.h"

#include <math.h>

#include "Config.h"

namespace BacteriaLab {

bool Display::begin() {
  frame_.setColorDepth(16);
  frame_.setAttribute(PSRAM_ENABLE, true);
  return frame_.createSprite(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT) != nullptr;
}

void Display::drawButton(int16_t x, int16_t width, const char* label,
                         uint16_t color, bool active) {
  const int16_t y = Config::CONTROLS_TOP + 6;
  const uint16_t fill = active ? color : 0x18E3;
  frame_.fillRoundRect(x, y, width, 46, 7, fill);
  frame_.drawRoundRect(x, y, width, 46, 7, color);
  frame_.setTextDatum(MC_DATUM);
  frame_.setTextColor(TFT_WHITE, fill);
  frame_.drawString(label, x + width / 2, y + 23, 4);
}

void Display::render(const World& world, bool running, bool recording,
                     uint32_t recordingSeconds) {
  constexpr uint16_t BACKGROUND = 0x0841;
  constexpr uint16_t HUD = 0x10A2;
  constexpr uint16_t CONTROLS = 0x0841;
  // FNK0104S uses the standard ST7796 RGB565 palette.
  constexpr uint16_t FOOD = TFT_WHITE;
  constexpr uint16_t DEATH_ENERGY = TFT_BLACK;
  constexpr uint16_t DEATH_AGE = TFT_LIGHTGREY;
  constexpr uint16_t DEATH_KIA = TFT_ORANGE;
  constexpr uint16_t TOXIN = TFT_CYAN;
  constexpr uint16_t DEATH_TOXIN = TFT_MAGENTA;
  constexpr uint16_t EXPLORER = 0x04FF;  // bright blue-cyan
  constexpr uint16_t SPRINTER = TFT_YELLOW;
  constexpr uint16_t SURVIVOR = TFT_GREEN;
  constexpr uint16_t BREEDER = TFT_MAGENTA;

  frame_.fillSprite(BACKGROUND);
  frame_.fillRect(0, 0, Config::SCREEN_WIDTH, Config::HUD_HEIGHT, HUD);
  frame_.fillRect(0, Config::CONTROLS_TOP, Config::SCREEN_WIDTH, Config::CONTROLS_HEIGHT, CONTROLS);

  const FoodSource* food = world.food();
    for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) {
      if (!food[i].active) continue;
      frame_.fillCircle(static_cast<int16_t>(food[i].x),
                        Config::HUD_HEIGHT + static_cast<int16_t>(food[i].y),
                        6, FOOD);
    }

    const ToxinSource* toxins = world.toxins();
    for (uint8_t i = 0; i < Config::TOXIN_SOURCES; ++i) {
      if (!toxins[i].active) continue;
      frame_.fillCircle(static_cast<int16_t>(toxins[i].x),
                        Config::HUD_HEIGHT + static_cast<int16_t>(toxins[i].y),
                        6, TOXIN);
    }

    const Biomass* biomass = world.biomass();
    for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
      if (!biomass[i].active) continue;
      uint16_t deathColor = DEATH_ENERGY;
      if (biomass[i].cause == DeathCause::Age) deathColor = DEATH_AGE;
      else if (biomass[i].cause == DeathCause::KIA) deathColor = DEATH_KIA;
      else if (biomass[i].cause == DeathCause::Toxin) deathColor = DEATH_TOXIN;
      frame_.fillCircle(static_cast<int16_t>(biomass[i].x),
                        Config::HUD_HEIGHT + static_cast<int16_t>(biomass[i].y),
                        biomass[i].radius, deathColor);
      frame_.drawPixel(static_cast<int16_t>(biomass[i].x),
                       Config::HUD_HEIGHT + static_cast<int16_t>(biomass[i].y),
                       TFT_WHITE);
    }

    const Bacterium* bacteria = world.bacteria();
    for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
      if (!bacteria[i].alive) continue;
      const int16_t x = static_cast<int16_t>(bacteria[i].x);
      const int16_t y = Config::HUD_HEIGHT + static_cast<int16_t>(bacteria[i].y);
      uint16_t speciesColor = EXPLORER;
      if (bacteria[i].species == Species::Sprinter) speciesColor = SPRINTER;
      else if (bacteria[i].species == Species::Survivor) speciesColor = SURVIVOR;
      else if (bacteria[i].species == Species::Breeder) speciesColor = BREEDER;
      frame_.fillCircle(x, y, Config::bacteriumRadius(bacteria[i].energy),
                        speciesColor);
    }
  const Statistics& s = world.stats();
  frame_.setTextColor(TFT_WHITE, HUD);
  frame_.setTextDatum(TL_DATUM);
  frame_.drawString("v0.15.4", 6, 3, 2);
  frame_.setTextDatum(TR_DATUM);
  String status = "LIVE " + String(s.alive);
  if (recording) {
    const uint32_t hours = recordingSeconds / 3600;
    const uint8_t minutes = (recordingSeconds / 60) % 60;
    const uint8_t seconds = recordingSeconds % 60;
    status += " REC ";
    if (hours > 0) {
      if (hours < 10) status += '0';
      status += String(hours);
      status += ':';
    }
    if (minutes < 10) status += '0';
    status += String(minutes);
    status += ':';
    if (seconds < 10) status += '0';
    status += String(seconds);
  }
  frame_.drawString(status, Config::SCREEN_WIDTH - 4, 3, 2);

  drawButton(6, 148, running ? "PAUSE" : "RUN", TFT_GREEN, !running);
  drawButton(166, 148, "RESET", TFT_ORANGE, false);
  drawButton(326, 148, recording ? "STOP" : "REC",
             recording ? TFT_ORANGE : TFT_GREEN, recording);
  frame_.pushSprite(0, 0);
}

}  // namespace BacteriaLab
