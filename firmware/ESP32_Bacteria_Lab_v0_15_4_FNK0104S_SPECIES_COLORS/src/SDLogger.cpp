#include "SDLogger.h"

#include <FS.h>
#include <SD_MMC.h>
#include <stdio.h>

#include "Config.h"

namespace BacteriaLab {

namespace {
constexpr int SD_MMC_CMD_PIN = 40;
constexpr int SD_MMC_CLK_PIN = 38;
constexpr int SD_MMC_D0_PIN = 39;
constexpr int SD_MMC_D1_PIN = 41;
constexpr int SD_MMC_D2_PIN = 48;
constexpr int SD_MMC_D3_PIN = 47;
}

bool SDLogger::begin(Stream& out) {
  SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN,
                 SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN);
  available_ = SD_MMC.begin("/sdcard", false, false, SDMMC_FREQ_DEFAULT, 5) &&
               SD_MMC.cardType() != CARD_NONE;
  if (!available_) {
    out.println("SD: card not available; simulation continues");
    return false;
  }
  out.print("SD: mounted, size MB=");
  out.println(static_cast<uint32_t>(SD_MMC.cardSize() / (1024ULL * 1024ULL)));
  return true;
}

bool SDLogger::chooseSnapshotPath(char* path, size_t pathSize) const {
  for (uint16_t number = 1; number <= 999; ++number) {
    snprintf(path, pathSize, "/snapshot_%03u.csv", number);
    if (!SD_MMC.exists(path)) return true;
  }
  path[0] = '\0';
  return false;
}

bool SDLogger::chooseUniversePath() {
  for (uint16_t number = 1; number <= 999; ++number) {
    snprintf(universePath_, sizeof(universePath_), "/universe_%03u.csv", number);
    if (!SD_MMC.exists(universePath_)) return true;
  }
  universePath_[0] = '\0';
  return false;
}

bool SDLogger::writeUniverseHeader() {
  fs::File file = SD_MMC.open(universePath_, FILE_WRITE);
  if (!file) return false;
  file.println("frame,seconds,object_type,id,parent_id,lineage_id,species,generation,x,y,direction,energy,age,reproduction_cooldown,attack_cooldown,last_toxin_signal,children,carrion_eaten,attacks,kills,kin_kills,colony_neighbors,speed,sensor_range,metabolism,reproduction_energy,max_age,tumble_rate,carrion_affinity,carrion_efficiency,aggressiveness,attack_power,colony_affinity,toxin_resistance,toxin_sensor_range,toxin_sensitivity,toxin_avoidance,toxin_tumble_rate,death_cause,death_second,radius,war,poison,carrion,mutation,food_auto,simulation");
  file.close();
  return true;
}

void SDLogger::snapshot(Stream& out, const World& world) {
  if (!available_) {
    out.println("SNAPSHOT ERROR: SD card unavailable");
    return;
  }

  char snapshotPath[24] = {};
  if (!chooseSnapshotPath(snapshotPath, sizeof(snapshotPath))) {
    out.println("SNAPSHOT ERROR: no free filename");
    return;
  }

  fs::File file = SD_MMC.open(snapshotPath, FILE_WRITE);
  if (!file) {
    out.println("SNAPSHOT ERROR: cannot create CSV");
    return;
  }

  file.println("seconds,id,parent_id,lineage_id,species,generation,x,y,direction,energy,age,reproduction_cooldown,attack_cooldown,last_toxin_signal,children,carrion_eaten,attacks,kills,kin_kills,colony_neighbors,speed,sensor_range,metabolism,reproduction_energy,max_age,tumble_rate,carrion_affinity,carrion_efficiency,aggressiveness,attack_power,colony_affinity,toxin_resistance,toxin_sensor_range,toxin_sensitivity,toxin_avoidance,toxin_tumble_rate");

  const uint32_t seconds =
      (world.stats().step * Config::SIMULATION_STEP_MS) / 1000;
  const Bacterium* bacteria = world.bacteria();
  uint16_t rows = 0;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    const Bacterium& b = bacteria[i];
    if (!b.alive) continue;
    const char* species = "Unknown";
    switch (b.species) {
      case Species::Explorer: species = "Explorer"; break;
      case Species::Sprinter: species = "Sprinter"; break;
      case Species::Survivor: species = "Survivor"; break;
      case Species::Breeder: species = "Breeder"; break;
      default: break;
    }

    file.print(seconds); file.print(',');
    file.print(b.id); file.print(','); file.print(b.parentId); file.print(',');
    file.print(b.lineageId); file.print(','); file.print(species); file.print(',');
    file.print(b.generation); file.print(',');
    file.print(b.x, 3); file.print(','); file.print(b.y, 3); file.print(',');
    file.print(b.direction, 5); file.print(','); file.print(b.energy, 3); file.print(',');
    file.print(b.age, 3); file.print(',');
    file.print(b.reproductionCooldown, 3); file.print(',');
    file.print(b.attackCooldown, 3); file.print(',');
    file.print(b.lastToxinSignal, 5); file.print(',');
    file.print(b.children); file.print(','); file.print(b.carrionEaten, 3); file.print(',');
    file.print(b.attacks); file.print(','); file.print(b.kills); file.print(',');
    file.print(b.kinKills); file.print(','); file.print(b.colonyNeighbors); file.print(',');
    file.print(b.genome.speed, 4); file.print(',');
    file.print(b.genome.sensorRange, 4); file.print(',');
    file.print(b.genome.metabolism, 5); file.print(',');
    file.print(b.genome.reproductionEnergy, 4); file.print(',');
    file.print(b.genome.maxAge, 4); file.print(',');
    file.print(b.genome.tumbleRate, 5); file.print(',');
    file.print(b.genome.carrionAffinity, 5); file.print(',');
    file.print(b.genome.carrionEfficiency, 5); file.print(',');
    file.print(b.genome.aggressiveness, 5); file.print(',');
    file.print(b.genome.attackPower, 4); file.print(',');
    file.print(b.genome.colonyAffinity, 5); file.print(',');
    file.print(b.genome.toxinResistance, 5); file.print(',');
    file.print(b.genome.toxinSensorRange, 4); file.print(',');
    file.print(b.genome.toxinSensitivity, 5); file.print(',');
    file.print(b.genome.toxinAvoidance, 5); file.print(',');
    file.println(b.genome.toxinTumbleRate, 5);
    ++rows;
  }
  file.close();

  out.print("SNAPSHOT SAVED: "); out.print(snapshotPath);
  out.print(" bacteria="); out.println(rows);
}

void SDLogger::writeBacteriumFields(fs::File& file, const Bacterium& b) {
  const char* species = "Unknown";
  switch (b.species) {
    case Species::Explorer: species = "Explorer"; break;
    case Species::Sprinter: species = "Sprinter"; break;
    case Species::Survivor: species = "Survivor"; break;
    case Species::Breeder: species = "Breeder"; break;
    default: break;
  }
  file.print("bacterium,");
  file.print(b.id); file.print(','); file.print(b.parentId); file.print(',');
  file.print(b.lineageId); file.print(','); file.print(species); file.print(',');
  file.print(b.generation); file.print(',');
  file.print(b.x, 3); file.print(','); file.print(b.y, 3); file.print(',');
  file.print(b.direction, 5); file.print(','); file.print(b.energy, 3); file.print(',');
  file.print(b.age, 3); file.print(',');
  file.print(b.reproductionCooldown, 3); file.print(',');
  file.print(b.attackCooldown, 3); file.print(',');
  file.print(b.lastToxinSignal, 5); file.print(',');
  file.print(b.children); file.print(','); file.print(b.carrionEaten, 3); file.print(',');
  file.print(b.attacks); file.print(','); file.print(b.kills); file.print(',');
  file.print(b.kinKills); file.print(','); file.print(b.colonyNeighbors); file.print(',');
  file.print(b.genome.speed, 4); file.print(',');
  file.print(b.genome.sensorRange, 4); file.print(',');
  file.print(b.genome.metabolism, 5); file.print(',');
  file.print(b.genome.reproductionEnergy, 4); file.print(',');
  file.print(b.genome.maxAge, 4); file.print(',');
  file.print(b.genome.tumbleRate, 5); file.print(',');
  file.print(b.genome.carrionAffinity, 5); file.print(',');
  file.print(b.genome.carrionEfficiency, 5); file.print(',');
  file.print(b.genome.aggressiveness, 5); file.print(',');
  file.print(b.genome.attackPower, 4); file.print(',');
  file.print(b.genome.colonyAffinity, 5); file.print(',');
  file.print(b.genome.toxinResistance, 5); file.print(',');
  file.print(b.genome.toxinSensorRange, 4); file.print(',');
  file.print(b.genome.toxinSensitivity, 5); file.print(',');
  file.print(b.genome.toxinAvoidance, 5); file.print(',');
  file.print(b.genome.toxinTumbleRate, 5);
  file.println(",,,,,,,,,");
}

bool SDLogger::writeUniverseFrame(const World& world, bool simulationRunning) {
  fs::File file = SD_MMC.open(universePath_, FILE_APPEND);
  if (!file) return false;
  const uint32_t frameNumber = universeFrames_ + 1;
  const uint32_t seconds =
      (world.stats().step * Config::SIMULATION_STEP_MS) / 1000;
  const Bacterium* bacteria = world.bacteria();
  const Biomass* biomass = world.biomass();
  uint16_t bacteriaRows = 0;
  uint16_t biomassRows = 0;
  file.print(frameNumber); file.print(',');
  file.print(seconds); file.print(",settings");
  for (uint8_t blank = 0; blank < 39; ++blank) file.print(',');
  file.print(world.warEnabled() ? "ON" : "OFF"); file.print(',');
  file.print(world.toxinsEnabled() ? "ON" : "OFF"); file.print(',');
  file.print(world.carrionEnabled() ? "ON" : "OFF"); file.print(',');
  file.print("ON,ON,");
  file.println(simulationRunning ? "RUN" : "PAUSE");
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria[i].alive) continue;
    file.print(frameNumber); file.print(',');
    file.print(seconds); file.print(',');
    writeBacteriumFields(file, bacteria[i]);
    ++bacteriaRows;
  }
  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    const Biomass& m = biomass[i];
    if (!m.active) continue;
    const char* species = "Unknown";
    switch (m.species) {
      case Species::Explorer: species = "Explorer"; break;
      case Species::Sprinter: species = "Sprinter"; break;
      case Species::Survivor: species = "Survivor"; break;
      case Species::Breeder: species = "Breeder"; break;
      default: break;
    }
    const char* cause = "ENERGY";
    if (m.cause == DeathCause::Age) cause = "AGE";
    else if (m.cause == DeathCause::KIA) cause = "KIA";
    else if (m.cause == DeathCause::Toxin) cause = "TOXIN";

    file.print(frameNumber); file.print(',');
    file.print(seconds); file.print(",biomass,");
    file.print(m.bacteriumId); file.print(',');
    file.print(m.parentId); file.print(',');
    file.print(m.lineageId); file.print(',');
    file.print(species); file.print(',');
    file.print(m.generation); file.print(',');
    file.print(m.x, 3); file.print(',');
    file.print(m.y, 3); file.print(",,");
    file.print(m.energy, 3);
    for (uint8_t blank = 0; blank < 27; ++blank) file.print(',');
    file.print(cause); file.print(',');
    file.print(m.deathSecond); file.print(',');
    file.print(m.radius);
    file.println(",,,,,,");
    ++biomassRows;
  }
  file.close();
  ++universeFrames_;
  universeRows_ += 1 + bacteriaRows + biomassRows;
  return true;
}

bool SDLogger::startUniverse(Stream& out, const World& world,
                             bool simulationRunning) {
  if (!available_) {
    out.println("UNIVERSE ERROR: SD card unavailable");
    return false;
  }
  if (universeEnabled_) return true;
  universeFrames_ = 0;
  universeRows_ = 0;
  if (!chooseUniversePath() || !writeUniverseHeader()) {
    out.println("UNIVERSE ERROR: cannot create CSV");
    return false;
  }
  universeStartStep_ = world.stats().step;
  lastUniverseStep_ = universeStartStep_;
  if (!writeUniverseFrame(world, simulationRunning)) {
    out.println("UNIVERSE ERROR: cannot write first frame");
    return false;
  }
  universeEnabled_ = true;
  out.print("UNIVERSE REC ON: "); out.println(universePath_);
  out.print("frame=1 bacteria="); out.println(world.stats().alive);
  return true;
}

void SDLogger::stopUniverse(Stream& out) {
  if (!universeEnabled_) return;
  universeEnabled_ = false;
  out.print("UNIVERSE REC OFF: "); out.println(universePath_);
  out.print("frames="); out.print(universeFrames_);
  out.print(" rows="); out.println(universeRows_);
}

uint16_t SDLogger::secondsUntilUniverseFrame(const World& world) const {
  if (!universeEnabled_) return 0;
  const uint32_t elapsed = world.stats().step - lastUniverseStep_;
  if (elapsed >= UNIVERSE_INTERVAL_STEPS) return 0;
  const uint32_t remainingSteps = UNIVERSE_INTERVAL_STEPS - elapsed;
  const uint32_t remainingMs = remainingSteps * Config::SIMULATION_STEP_MS;
  return static_cast<uint16_t>((remainingMs + 999) / 1000);
}

uint32_t SDLogger::universeRecordingSeconds(const World& world) const {
  if (!universeEnabled_) return 0;
  const uint32_t elapsedSteps = world.stats().step - universeStartStep_;
  return (elapsedSteps * Config::SIMULATION_STEP_MS) / 1000;
}

void SDLogger::update(const World& world, bool simulationRunning) {
  const uint32_t step = world.stats().step;
  if (universeEnabled_ &&
      step - lastUniverseStep_ >= UNIVERSE_INTERVAL_STEPS) {
    if (writeUniverseFrame(world, simulationRunning)) lastUniverseStep_ = step;
    else universeEnabled_ = false;
  }
}

void SDLogger::printStatus(Stream& out) const {
  out.print("SD="); out.println(available_ ? "READY" : "UNAVAILABLE");
  out.print("UNIVERSE="); out.println(universeEnabled_ ? "REC" : "OFF");
  out.print("universe_file=");
  out.println(universePath_[0] ? universePath_ : "none");
  out.print("frames="); out.println(universeFrames_);
  out.print("universe_rows="); out.println(universeRows_);
  if (available_) {
    out.print("used_MB=");
    out.println(static_cast<uint32_t>(SD_MMC.usedBytes() / (1024ULL * 1024ULL)));
  }
}

}  // namespace BacteriaLab
