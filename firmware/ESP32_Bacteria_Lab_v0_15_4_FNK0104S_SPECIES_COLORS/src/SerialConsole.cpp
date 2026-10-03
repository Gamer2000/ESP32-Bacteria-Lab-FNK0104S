#include "SerialConsole.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace BacteriaLab {

SerialCommand SerialConsole::poll(Stream& stream) {
  uint8_t processed = 0;
  while (stream.available() > 0 && processed < 32) {
    ++processed;
    const int value = stream.read();
    if (value < 0) break;
    const char c = static_cast<char>(value);
    if (c == '\r') continue;
    if (c == '\n') {
      buffer_[length_] = '\0';
      SerialCommand command = parse();
      length_ = 0;
      buffer_[0] = '\0';
      return command;
    }
    if (length_ < BUFFER_SIZE - 1) buffer_[length_++] = c;
  }
  return {};
}

SerialCommand SerialConsole::parse() {
  for (uint8_t i = 0; i < length_; ++i) {
    buffer_[i] = static_cast<char>(tolower(static_cast<unsigned char>(buffer_[i])));
  }
  char* text = buffer_;
  while (*text == ' ') ++text;
  if (*text == '\0') return {};
  if (strcmp(text, "help") == 0) return {SerialCommandType::Help};
  if (strcmp(text, "stats") == 0) return {SerialCommandType::Stats};
  if (strcmp(text, "stats on") == 0) return {SerialCommandType::StatsOn};
  if (strcmp(text, "stats off") == 0) return {SerialCommandType::StatsOff};
  if (strcmp(text, "list") == 0) return {SerialCommandType::List};
  if (strcmp(text, "next") == 0) return {SerialCommandType::Next};
  if (strcmp(text, "pause") == 0) return {SerialCommandType::Pause};
  if (strcmp(text, "run") == 0) return {SerialCommandType::Run};
  if (strcmp(text, "food") == 0) return {SerialCommandType::Food};
  if (strcmp(text, "reset") == 0) return {SerialCommandType::Reset};
  if (strcmp(text, "predators") == 0) return {SerialCommandType::Predators};
  if (strcmp(text, "deaths") == 0) return {SerialCommandType::Deaths};
  if (strcmp(text, "poison on") == 0) return {SerialCommandType::PoisonOn};
  if (strcmp(text, "poison off") == 0) return {SerialCommandType::PoisonOff};
  if (strcmp(text, "poison status") == 0 || strcmp(text, "toxins") == 0) {
    return {SerialCommandType::PoisonStatus};
  }
  if (strcmp(text, "war on") == 0) return {SerialCommandType::WarOn};
  if (strcmp(text, "war off") == 0) return {SerialCommandType::WarOff};
  if (strcmp(text, "carrion on") == 0) return {SerialCommandType::CarrionOn};
  if (strcmp(text, "carrion off") == 0) return {SerialCommandType::CarrionOff};
  if (strcmp(text, "systems") == 0) return {SerialCommandType::Systems};
  if (strcmp(text, "genes") == 0) return {SerialCommandType::Genes};
  if (strcmp(text, "evolution") == 0) return {SerialCommandType::Evolution};
  if (strcmp(text, "experiment reset") == 0) return {SerialCommandType::ExperimentReset};
  if (strcmp(text, "biomass") == 0 || strcmp(text, "carrion status") == 0) {
    return {SerialCommandType::BiomassStatus};
  }
  if (strcmp(text, "sd status") == 0 || strcmp(text, "sd") == 0) return {SerialCommandType::SdStatus};
  if (strcmp(text, "log on") == 0) return {SerialCommandType::LogOn};
  if (strcmp(text, "log off") == 0) return {SerialCommandType::LogOff};
  if (strcmp(text, "log now") == 0) return {SerialCommandType::LogNow};
  if (strcmp(text, "snapshot") == 0) return {SerialCommandType::Snapshot};
  if (strncmp(text, "show", 4) == 0) {
    text += 4;
    while (*text == ' ') ++text;
    char* end = nullptr;
    const unsigned long id = strtoul(text, &end, 10);
    while (end && *end == ' ') ++end;
    if (*text != '\0' && end && *end == '\0' && id > 0) {
      return {SerialCommandType::Show, static_cast<uint32_t>(id)};
    }
  }
  return {SerialCommandType::Invalid};
}

const char* SerialConsole::speciesName(Species species) {
  switch (species) {
    case Species::Explorer: return "Explorer";
    case Species::Sprinter: return "Sprinter";
    case Species::Survivor: return "Survivor";
    case Species::Breeder: return "Breeder";
    default: return "Unknown";
  }
}

void SerialConsole::printHelp(Stream& out) const {
  out.println("Commands: help, stats, stats on, stats off, list,");
  out.println("          show ID, next,");
  out.println("          predators, deaths, poison on, poison off,");
  out.println("          poison status, war on/off, carrion on/off,");
  out.println("          systems, genes, biomass, carrion status,");
  out.println("          sd status, log on/off, log now, snapshot,");
  out.println("          evolution, experiment reset,");
  out.println("          pause, run, food, reset");
}

void SerialConsole::printStats(Stream& out, const World& world, bool running) const {
  const Statistics& s = world.stats();
  out.print("state="); out.print(running ? "RUN" : "PAUSE");
  out.print(" step="); out.print(s.step);
  out.print(" alive="); out.print(s.alive);
  out.print(" species=");
  out.print(s.species[0]); out.print('/'); out.print(s.species[1]); out.print('/');
  out.print(s.species[2]); out.print('/'); out.print(s.species[3]);
  out.print(" maxgen="); out.print(s.maxGeneration);
  out.print(" births="); out.print(s.births);
  out.print(" deaths="); out.print(s.deaths);
  out.print(" causes="); out.print(s.energyDeaths); out.print('/');
  out.print(s.ageDeaths); out.print('/'); out.print(s.attackDeaths); out.print('/');
  out.print(s.toxinDeaths);
  out.print(" mutations="); out.print(s.mutations);
  out.print(" carrion="); out.println(s.carrionConsumed, 1);
  out.print("combat attacks="); out.print(s.attacks);
  out.print(" kills="); out.print(s.kills);
  out.print(" attack_deaths="); out.print(s.attackDeaths);
  out.print(" transfers="); out.print(s.attackTransfers);
  out.print(" stolen="); out.println(s.energyStolen, 1);
  out.print("war kin_attacks="); out.print(s.kinAttacks);
  out.print(" kin_kills="); out.println(s.kinKills);
  out.print("colonies members="); out.print(s.colonyMembers);
  out.print(" largest_local_group="); out.println(s.largestLocalGroup);
  out.print("biomass active="); out.print(s.activeBiomass);
  out.print(" energy="); out.println(s.biomassEnergy, 1);
}

void SerialConsole::startList(Stream& out) {
  listActive_ = true;
  listIndex_ = 0;
  listedCount_ = 0;
  out.println("ID       TYPE       GEN  ENERGY  AGE");
}

void SerialConsole::continueList(Stream& out, const World& world, uint8_t maxRows) {
  if (!listActive_) return;
  uint8_t rowsThisPass = 0;
  const Bacterium* bacteria = world.bacteria();
  while (listIndex_ < Config::MAX_BACTERIA && rowsThisPass < maxRows) {
    const Bacterium& b = bacteria[listIndex_++];
    if (!b.alive) continue;
    out.print(b.id); out.print("  ");
    out.print(speciesName(b.species)); out.print("  ");
    out.print(b.generation); out.print("    ");
    out.print(b.energy, 1); out.print("    ");
    out.println(b.age, 1);
    ++listedCount_;
    ++rowsThisPass;
  }
  if (listIndex_ >= Config::MAX_BACTERIA) {
    listActive_ = false;
    out.print("Listed "); out.print(listedCount_);
    out.print(" living bacteria; alive now "); out.println(world.stats().alive);
  }
}

void SerialConsole::printBacterium(Stream& out, const Bacterium& b) const {
  out.println("--- BACTERIUM SNAPSHOT ---");
  out.print("ID: "); out.println(b.id);
  out.print("Parent: "); out.println(b.parentId);
  out.print("Generation: "); out.println(b.generation);
  out.print("Children: "); out.println(b.children);
  out.print("Species: "); out.println(speciesName(b.species));
  out.print("Energy: "); out.println(b.energy, 2);
  out.print("Radius: "); out.println(Config::bacteriumRadius(b.energy));
  out.print("Age: "); out.println(b.age, 2);
  out.print("Speed: "); out.println(b.genome.speed, 2);
  out.print("Sensor: "); out.println(b.genome.sensorRange, 2);
  out.print("Metabolism: "); out.println(b.genome.metabolism, 3);
  out.print("Reproduction: "); out.println(b.genome.reproductionEnergy, 2);
  out.print("Max age: "); out.println(b.genome.maxAge, 2);
  out.print("Tumble: "); out.println(b.genome.tumbleRate, 3);
  out.print("Carrion affinity: "); out.println(b.genome.carrionAffinity, 3);
  out.print("Carrion efficiency: "); out.println(b.genome.carrionEfficiency, 3);
  out.print("Carrion eaten: "); out.println(b.carrionEaten, 2);
  out.print("Aggressiveness: "); out.println(b.genome.aggressiveness, 3);
  out.print("Attack power: "); out.println(b.genome.attackPower, 2);
  out.print("Attacks: "); out.println(b.attacks);
  out.print("Kills: "); out.println(b.kills);
  out.print("Kin kills: "); out.println(b.kinKills);
  out.print("Lineage: "); out.println(b.lineageId);
  out.print("Colony affinity: "); out.println(b.genome.colonyAffinity, 3);
  out.print("Nearby kin: "); out.println(b.colonyNeighbors);
  out.print("Toxin resistance: "); out.println(b.genome.toxinResistance, 3);
  out.print("Toxin sensor range: "); out.println(b.genome.toxinSensorRange, 2);
  out.print("Toxin sensitivity: "); out.println(b.genome.toxinSensitivity, 3);
  out.print("Toxin avoidance: "); out.println(b.genome.toxinAvoidance, 3);
  out.print("Toxin tumble rate: "); out.println(b.genome.toxinTumbleRate, 3);
  const float toxinSensorCost = Config::TOXIN_SENSOR_COST *
                                (b.genome.toxinSensorRange / 46.0f) *
                                (0.5f + 0.5f * b.genome.toxinSensitivity) +
                                0.03f * (b.genome.toxinAvoidance / 0.10f) +
                                0.01f * (b.genome.toxinTumbleRate / 4.0f);
  out.print("Toxin sensor cost: "); out.println(toxinSensorCost, 3);
  out.print("Toxin signal: "); out.println(b.lastToxinSignal, 3);
}

void SerialConsole::printPredators(Stream& out, const World& world) const {
  const Bacterium* bacteria = world.bacteria();
  out.println("TOP PREDATORS: ID TYPE KILLS ATTACKS AGGRESSION");
  int16_t selected[10];
  for (uint8_t i = 0; i < 10; ++i) selected[i] = -1;
  for (uint8_t rank = 0; rank < 10; ++rank) {
    int16_t best = -1;
    for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
      if (!bacteria[i].alive || bacteria[i].kills == 0) continue;
      bool alreadyUsed = false;
      for (uint8_t previous = 0; previous < rank; ++previous) {
        if (selected[previous] == static_cast<int16_t>(i)) alreadyUsed = true;
      }
      if (alreadyUsed) continue;
      if (best < 0 || bacteria[i].kills > bacteria[best].kills ||
          (bacteria[i].kills == bacteria[best].kills && bacteria[i].attacks > bacteria[best].attacks)) {
        best = static_cast<int16_t>(i);
      }
    }
    if (best < 0) {
      if (rank == 0) out.println("No living killers yet");
      break;
    }
    selected[rank] = best;
    const Bacterium& b = bacteria[best];
    out.print(b.id); out.print(' '); out.print(speciesName(b.species)); out.print(' ');
    out.print(b.kills); out.print(' '); out.print(b.attacks); out.print(' ');
    out.println(b.genome.aggressiveness, 3);
  }
}

void SerialConsole::printDeathStats(Stream& out, const World& world) const {
  const Statistics& s = world.stats();
  const float total = s.deaths > 0 ? static_cast<float>(s.deaths) : 1.0f;
  out.println("DEATH CAUSES");
  out.print("ENERGY: "); out.print(s.energyDeaths); out.print(" (");
  out.print(100.0f * s.energyDeaths / total, 1); out.println("%)");
  out.print("AGE:    "); out.print(s.ageDeaths); out.print(" (");
  out.print(100.0f * s.ageDeaths / total, 1); out.println("%)");
  out.print("KIA:    "); out.print(s.attackDeaths); out.print(" (");
  out.print(100.0f * s.attackDeaths / total, 1); out.println("%)");
  out.print("TOXIN:  "); out.print(s.toxinDeaths); out.print(" (");
  out.print(100.0f * s.toxinDeaths / total, 1); out.println("%)");
  out.print("TOTAL:  "); out.println(s.deaths);
}

void SerialConsole::printToxinStats(Stream& out, const World& world) const {
  const Statistics& s = world.stats();
  float resistanceSum = 0.0f;
  uint16_t living = 0;
  const Bacterium* bacteria = world.bacteria();
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria[i].alive) continue;
    resistanceSum += bacteria[i].genome.toxinResistance;
    ++living;
  }
  out.print("poison="); out.print(world.toxinsEnabled() ? "ON" : "OFF");
  out.print(" sources="); out.print(world.toxinsEnabled() ? Config::TOXIN_SOURCES : 0);
  out.print(" deaths="); out.print(s.toxinDeaths);
  out.print(" exposures="); out.print(s.toxinExposures);
  out.print(" damage="); out.print(s.toxinDamage, 1);
  out.print(" avoid_turns="); out.print(s.toxinAvoidanceTurns);
  out.print(" tumbles="); out.print(s.toxinTumbles);
  out.print(" mean_resistance=");
  out.println(living > 0 ? resistanceSum / living : 0.0f, 3);
}

void SerialConsole::printSystems(Stream& out, const World& world) const {
  out.println("EXPERIMENT SYSTEMS");
  out.print("POISON  "); out.println(world.toxinsEnabled() ? "ON" : "OFF");
  out.print("WAR     "); out.println(world.warEnabled() ? "ON" : "OFF");
  out.print("CARRION "); out.println(world.carrionEnabled() ? "ON" : "OFF");
  out.println("MUTATION ON");
  out.println("FOOD_AUTO ON");
}

void SerialConsole::printGenes(Stream& out, const World& world) const {
  const Bacterium* bacteria = world.bacteria();
  float sums[5] = {};
  float mins[5] = {1.0e9f, 1.0e9f, 1.0e9f, 1.0e9f, 1.0e9f};
  float maxs[5] = {};
  uint16_t count = 0;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria[i].alive) continue;
    const float values[5] = {bacteria[i].genome.toxinResistance,
                             bacteria[i].genome.toxinSensorRange,
                             bacteria[i].genome.toxinSensitivity,
                             bacteria[i].genome.toxinAvoidance,
                             bacteria[i].genome.toxinTumbleRate};
    for (uint8_t gene = 0; gene < 5; ++gene) {
      sums[gene] += values[gene];
      if (values[gene] < mins[gene]) mins[gene] = values[gene];
      if (values[gene] > maxs[gene]) maxs[gene] = values[gene];
    }
    ++count;
  }
  out.print("ADAPTIVE GENES living="); out.println(count);
  const char* names[5] = {"resistance", "sensor_range", "sensitivity", "avoidance", "tumble_rate"};
  for (uint8_t gene = 0; gene < 5; ++gene) {
    out.print(names[gene]); out.print(" mean=");
    out.print(count > 0 ? sums[gene] / count : 0.0f, 3);
    out.print(" min="); out.print(count > 0 ? mins[gene] : 0.0f, 3);
    out.print(" max="); out.println(count > 0 ? maxs[gene] : 0.0f, 3);
  }
}

void SerialConsole::printBiomassStats(Stream& out, const World& world) const {
  const Biomass* biomass = world.biomass();
  float total = 0.0f;
  float maximum = 0.0f;
  uint16_t active = 0;
  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    if (!biomass[i].active) continue;
    total += biomass[i].energy;
    if (biomass[i].energy > maximum) maximum = biomass[i].energy;
    ++active;
  }
  out.println("PERSISTENT BIOMASS");
  out.print("feeding="); out.println(world.carrionEnabled() ? "ON" : "OFF");
  out.print("active="); out.print(active); out.print('/'); out.println(Config::MAX_BIOMASS);
  out.print("energy_total="); out.println(total, 1);
  out.print("energy_mean="); out.println(active > 0 ? total / active : 0.0f, 1);
  out.print("energy_max="); out.println(maximum, 1);
  out.print("consumed="); out.println(world.stats().carrionConsumed, 1);
  out.print("replaced="); out.println(world.stats().biomassReplacements);
  out.print("decay_per_second="); out.println(Config::BIOMASS_DECAY_PER_SECOND, 3);
}

}  // namespace BacteriaLab
