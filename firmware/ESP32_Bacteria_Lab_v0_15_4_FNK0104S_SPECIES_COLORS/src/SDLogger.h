#pragma once

#include <Arduino.h>
#include <FS.h>
#include "Config.h"
#include "World.h"

namespace BacteriaLab {

class SDLogger {
 public:
  bool begin(Stream& out);
  void snapshot(Stream& out, const World& world);
  bool startUniverse(Stream& out, const World& world, bool simulationRunning);
  void stopUniverse(Stream& out);
  bool universeEnabled() const { return universeEnabled_; }
  uint32_t universeFrames() const { return universeFrames_; }
  uint32_t universeRecordingSeconds(const World& world) const;
  uint16_t secondsUntilUniverseFrame(const World& world) const;
  void update(const World& world, bool simulationRunning);
  void printStatus(Stream& out) const;

 private:
  static constexpr uint32_t UNIVERSE_INTERVAL_STEPS =
      1000 / Config::SIMULATION_STEP_MS;
  bool available_ = false;
  bool universeEnabled_ = false;
  uint32_t lastUniverseStep_ = 0;
  uint32_t universeStartStep_ = 0;
  uint32_t universeFrames_ = 0;
  uint32_t universeRows_ = 0;
  char universePath_[24] = {};

  bool chooseSnapshotPath(char* path, size_t pathSize) const;
  bool chooseUniversePath();
  bool writeUniverseHeader();
  bool writeUniverseFrame(const World& world, bool simulationRunning);
  static void writeBacteriumFields(fs::File& file,
                                   const Bacterium& bacterium);
};

}  // namespace BacteriaLab
