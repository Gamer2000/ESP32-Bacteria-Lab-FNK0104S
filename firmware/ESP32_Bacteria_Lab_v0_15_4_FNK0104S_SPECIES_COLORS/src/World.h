#pragma once

#include <Arduino.h>

#include "Config.h"
#include "Entities.h"
#include "Random.h"

namespace BacteriaLab {

class World {
 public:
  bool begin();
  void reset(uint32_t seed);
  void update(float dt);
  void addFoodSources(uint8_t count);
  void setToxinsEnabled(bool enabled);
  bool toxinsEnabled() const { return toxinsEnabled_; }
  void setWarEnabled(bool enabled) { warEnabled_ = enabled; }
  bool warEnabled() const { return warEnabled_; }
  void setCarrionEnabled(bool enabled) { carrionEnabled_ = enabled; }
  bool carrionEnabled() const { return carrionEnabled_; }
  int16_t findBacteriumNear(float x, float y, float radius) const;
  int16_t findNearestBacterium(float x, float y) const;
  int16_t findBacteriumById(uint32_t id) const;
  int16_t findNextAlive(int16_t currentIndex) const;

  const Bacterium* bacteria() const { return bacteria_; }
  const FoodSource* food() const { return food_; }
  const ToxinSource* toxins() const { return toxins_; }
  const Biomass* biomass() const { return biomass_; }
  const Statistics& stats() const { return stats_; }

 private:
  Bacterium* bacteria_ = nullptr;
  FoodSource* food_ = nullptr;
  Biomass* biomass_ = nullptr;
  ToxinSource toxins_[Config::TOXIN_SOURCES] = {};
  Statistics stats_;
  Random random_;
  uint32_t nextId_ = 1;
  bool toxinsEnabled_ = false;
  bool warEnabled_ = false;
  bool carrionEnabled_ = false;
  int16_t spatialHead_[Config::SPATIAL_CELLS] = {};
  int16_t spatialNext_[Config::MAX_BACTERIA] = {};

  void spawnFounder(uint16_t slot, float x, float y, float energy, Species species);
  void spawnChild(uint16_t slot, Bacterium& parent);
  Genome founderGenome(Species species) const;
  void mutateGenome(Genome& genome);
  float mutateValue(float value, float low, float high);
  void spawnFood(uint16_t slot);
  void spawnToxin(uint8_t slot);
  void leaveBiomass(const Bacterium& bacterium, float energy, DeathCause cause);
  int16_t findFreeBacterium() const;
  int16_t nearestFood(float x, float y, float maxDistanceSquared) const;
  int16_t nearestBiomass(float x, float y, float maxDistanceSquared) const;
  void buildSpatialIndex();
  int16_t nearbyPrey(uint16_t hunterIndex) const;
  uint8_t nearbyKin(uint16_t bacteriumIndex, float& centerX, float& centerY) const;
  static float wrap(float value, float limit);
  static float angleDelta(float target, float current);
  void updateStats();
};

}  // namespace BacteriaLab
