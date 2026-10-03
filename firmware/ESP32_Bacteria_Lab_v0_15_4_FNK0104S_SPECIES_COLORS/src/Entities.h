#pragma once

#include <stdint.h>

namespace BacteriaLab {

enum class Species : uint8_t {
  Explorer = 0,
  Sprinter = 1,
  Survivor = 2,
  Breeder = 3,
  Count = 4
};

struct Genome {
  float speed;
  float sensorRange;
  float metabolism;
  float reproductionEnergy;
  float maxAge;
  float tumbleRate;
  float carrionAffinity;
  float carrionEfficiency;
  float aggressiveness;
  float attackPower;
  float colonyAffinity;
  float toxinResistance;
  float toxinSensorRange;
  float toxinSensitivity;
  float toxinAvoidance;
  float toxinTumbleRate;
};

struct Bacterium {
  float x;
  float y;
  float direction;
  float energy;
  float age;
  float reproductionCooldown;
  float attackCooldown;
  float lastToxinSignal;
  Genome genome;
  uint32_t id;
  uint32_t parentId;
  uint32_t lineageId;
  uint16_t generation;
  uint16_t children;
  float carrionEaten;
  uint16_t attacks;
  uint16_t kills;
  uint16_t kinKills;
  uint8_t colonyNeighbors;
  Species species;
  bool alive;
};

struct FoodSource {
  float x;
  float y;
  float energy;
  float capacity;
  bool active;
};

struct ToxinSource {
  float x;
  float y;
  float age;
  float lifetime;
  float potency;
  bool active;
};

enum class DeathCause : uint8_t {
  Energy,
  Age,
  KIA,
  Toxin
};

struct Biomass {
  float x;
  float y;
  float energy;
  float capacity;
  float protectedTime;
  uint8_t radius;
  DeathCause cause;
  uint32_t bacteriumId;
  uint32_t parentId;
  uint32_t lineageId;
  uint32_t deathSecond;
  uint16_t generation;
  Species species;
  bool active;
};

struct Statistics {
  uint32_t step = 0;
  uint32_t births = 0;
  uint32_t deaths = 0;
  uint16_t alive = 0;
  float foodEnergy = 0.0f;
  float biomassEnergy = 0.0f;
  uint16_t activeBiomass = 0;
  uint32_t biomassReplacements = 0;
  uint16_t species[static_cast<uint8_t>(Species::Count)] = {};
  uint32_t mutations = 0;
  float carrionConsumed = 0.0f;
  uint32_t attacks = 0;
  uint32_t kills = 0;
  uint32_t attackDeaths = 0;
  uint32_t energyDeaths = 0;
  uint32_t ageDeaths = 0;
  uint32_t toxinDeaths = 0;
  uint32_t toxinExposures = 0;
  float toxinDamage = 0.0f;
  uint32_t toxinAvoidanceTurns = 0;
  uint32_t toxinTumbles = 0;
  uint32_t attackTransfers = 0;
  uint32_t kinAttacks = 0;
  uint32_t kinKills = 0;
  float energyStolen = 0.0f;
  uint16_t colonyMembers = 0;
  uint16_t largestLocalGroup = 0;
  uint16_t maxGeneration = 0;
};

}  // namespace BacteriaLab
