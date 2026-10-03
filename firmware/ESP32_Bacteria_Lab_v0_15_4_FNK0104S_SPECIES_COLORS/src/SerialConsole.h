#pragma once

#include <Arduino.h>

#include "World.h"

namespace BacteriaLab {

enum class SerialCommandType : uint8_t {
  None,
  Help,
  Stats,
  StatsOn,
  StatsOff,
  List,
  Show,
  Next,
  Pause,
  Run,
  Food,
  Reset,
  Predators,
  Deaths,
  PoisonOn,
  PoisonOff,
  PoisonStatus,
  WarOn,
  WarOff,
  CarrionOn,
  CarrionOff,
  Systems,
  Genes,
  Evolution,
  ExperimentReset,
  BiomassStatus,
  SdStatus,
  LogOn,
  LogOff,
  LogNow,
  Snapshot,
  Invalid
};

struct SerialCommand {
  SerialCommandType type;
  uint32_t id;

  SerialCommand(SerialCommandType commandType = SerialCommandType::None,
                uint32_t bacteriumId = 0)
      : type(commandType), id(bacteriumId) {}
};

class SerialConsole {
 public:
  SerialCommand poll(Stream& stream);
  void printHelp(Stream& stream) const;
  void printStats(Stream& stream, const World& world, bool running) const;
  void startList(Stream& stream);
  void continueList(Stream& stream, const World& world, uint8_t maxRows = 2);
  bool isListing() const { return listActive_; }
  void printBacterium(Stream& stream, const Bacterium& bacterium) const;
  void printPredators(Stream& stream, const World& world) const;
  void printDeathStats(Stream& stream, const World& world) const;
  void printToxinStats(Stream& stream, const World& world) const;
  void printSystems(Stream& stream, const World& world) const;
  void printGenes(Stream& stream, const World& world) const;
  void printBiomassStats(Stream& stream, const World& world) const;

 private:
  static constexpr uint8_t BUFFER_SIZE = 48;
  char buffer_[BUFFER_SIZE] = {};
  uint8_t length_ = 0;
  bool listActive_ = false;
  uint16_t listIndex_ = 0;
  uint16_t listedCount_ = 0;

  SerialCommand parse();
  static const char* speciesName(Species species);
};

}  // namespace BacteriaLab
