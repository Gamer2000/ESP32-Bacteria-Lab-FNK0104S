#include "World.h"

#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>

namespace BacteriaLab {

namespace {
constexpr float PI_F = 3.14159265358979323846f;
constexpr float TWO_PI_F = 2.0f * PI_F;
}

bool World::begin() {
  bacteria_ = static_cast<Bacterium*>(heap_caps_calloc(Config::MAX_BACTERIA, sizeof(Bacterium), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  food_ = static_cast<FoodSource*>(heap_caps_calloc(Config::FOOD_SOURCES, sizeof(FoodSource), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  biomass_ = static_cast<Biomass*>(heap_caps_calloc(Config::MAX_BIOMASS, sizeof(Biomass), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  return bacteria_ && food_ && biomass_;
}

void World::reset(uint32_t seed) {
  memset(bacteria_, 0, Config::MAX_BACTERIA * sizeof(Bacterium));
  memset(food_, 0, Config::FOOD_SOURCES * sizeof(FoodSource));
  memset(biomass_, 0, Config::MAX_BIOMASS * sizeof(Biomass));
  memset(toxins_, 0, sizeof(toxins_));
  stats_ = Statistics{};
  nextId_ = 1;
  random_.seed(seed);

  for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) spawnFood(i);
  if (toxinsEnabled_) {
    for (uint8_t i = 0; i < Config::TOXIN_SOURCES; ++i) spawnToxin(i);
  }
  for (uint16_t i = 0; i < Config::INITIAL_BACTERIA; ++i) {
    spawnFounder(i, random_.range(0, Config::WORLD_WIDTH),
                 random_.range(0, Config::WORLD_HEIGHT),
                 random_.range(Config::INITIAL_ENERGY_MIN, Config::INITIAL_ENERGY_MAX),
                 static_cast<Species>(i % static_cast<uint8_t>(Species::Count)));
  }
  updateStats();
}

Genome World::founderGenome(Species species) const {
  switch (species) {
    case Species::Explorer: return {19.0f, 76.0f, 1.10f, 108.0f, 160.0f, 0.72f, 0.35f, 0.70f, 0.24f, 5.5f, 0.46f, 0.28f, 58.0f, 1.05f, 0.12f, 4.0f};
    case Species::Sprinter: return {29.0f, 45.0f, 1.48f, 110.0f, 135.0f, 0.58f, 0.20f, 0.60f, 0.68f, 8.5f, 0.22f, 0.16f, 38.0f, 0.75f, 0.08f, 5.0f};
    case Species::Survivor: return {15.0f, 52.0f, 0.72f, 116.0f, 220.0f, 0.52f, 0.80f, 1.05f, 0.12f, 4.5f, 0.72f, 0.62f, 48.0f, 1.20f, 0.09f, 2.5f};
    case Species::Breeder:  return {18.0f, 55.0f, 1.08f, 91.0f, 150.0f, 0.78f, 0.50f, 0.82f, 0.42f, 6.5f, 0.62f, 0.34f, 42.0f, 0.90f, 0.10f, 4.5f};
    default:                return {20.0f, 54.0f, 1.10f, 105.0f, 150.0f, 0.65f, 0.45f, 0.78f, 0.35f, 6.0f, 0.45f, 0.30f, 46.0f, 1.00f, 0.10f, 4.0f};
  }
}

void World::spawnFounder(uint16_t slot, float x, float y, float energy, Species species) {
  Bacterium& b = bacteria_[slot];
  b.x = wrap(x, Config::WORLD_WIDTH);
  b.y = wrap(y, Config::WORLD_HEIGHT);
  b.direction = random_.range(0.0f, TWO_PI_F);
  b.energy = energy;
  b.age = 0.0f;
  b.reproductionCooldown = random_.range(0.0f, Config::REPRODUCTION_COOLDOWN_SECONDS);
  b.attackCooldown = random_.range(0.0f, Config::ATTACK_COOLDOWN_SECONDS);
  b.lastToxinSignal = 0.0f;
  b.genome = founderGenome(species);
  b.id = nextId_++;
  b.parentId = 0;
  b.lineageId = b.id;
  b.generation = 0;
  b.children = 0;
  b.carrionEaten = 0.0f;
  b.attacks = 0;
  b.kills = 0;
  b.kinKills = 0;
  b.colonyNeighbors = 0;
  b.species = species;
  b.alive = true;
}

float World::mutateValue(float value, float low, float high) {
  if (!random_.chance(Config::GENE_MUTATION_CHANCE)) return value;
  ++stats_.mutations;
  value *= 1.0f + random_.range(-Config::GENE_MUTATION_STRENGTH,
                               Config::GENE_MUTATION_STRENGTH);
  return fminf(high, fmaxf(low, value));
}

void World::mutateGenome(Genome& g) {
  g.speed = mutateValue(g.speed, 8.0f, 42.0f);
  g.sensorRange = mutateValue(g.sensorRange, 18.0f, 100.0f);
  g.metabolism = mutateValue(g.metabolism, 0.35f, 2.5f);
  g.reproductionEnergy = mutateValue(g.reproductionEnergy, 72.0f, 150.0f);
  g.maxAge = mutateValue(g.maxAge, 70.0f, 300.0f);
  g.tumbleRate = mutateValue(g.tumbleRate, 0.15f, 1.8f);
  g.carrionAffinity = mutateValue(g.carrionAffinity, 0.0f, 1.0f);
  g.carrionEfficiency = mutateValue(g.carrionEfficiency,
                                    Config::CARRION_MIN_EFFICIENCY,
                                    Config::CARRION_MAX_EFFICIENCY);
  g.aggressiveness = mutateValue(g.aggressiveness, 0.0f, 1.0f);
  g.attackPower = mutateValue(g.attackPower, 2.5f, 14.0f);
  g.colonyAffinity = mutateValue(g.colonyAffinity, 0.0f, 1.0f);
  g.toxinResistance = mutateValue(g.toxinResistance, 0.0f, 0.95f);
  g.toxinSensorRange = mutateValue(g.toxinSensorRange, 20.0f, 80.0f);
  g.toxinSensitivity = mutateValue(g.toxinSensitivity, 0.20f, 1.80f);
  g.toxinAvoidance = mutateValue(g.toxinAvoidance, 0.01f, 0.24f);
  g.toxinTumbleRate = mutateValue(g.toxinTumbleRate, 0.0f, 9.0f);
}

void World::spawnChild(uint16_t slot, Bacterium& parent) {
  Bacterium& child = bacteria_[slot];
  child.x = wrap(parent.x + random_.range(-3.0f, 3.0f), Config::WORLD_WIDTH);
  child.y = wrap(parent.y + random_.range(-3.0f, 3.0f), Config::WORLD_HEIGHT);
  child.direction = parent.direction + random_.range(-0.45f, 0.45f);
  child.energy = Config::CHILD_ENERGY;
  child.age = 0.0f;
  child.reproductionCooldown = Config::REPRODUCTION_COOLDOWN_SECONDS;
  child.attackCooldown = Config::ATTACK_COOLDOWN_SECONDS;
  child.lastToxinSignal = 0.0f;
  child.genome = parent.genome;
  mutateGenome(child.genome);
  child.species = parent.species;
  if (random_.chance(Config::SPECIES_MUTATION_CHANCE)) {
    const uint8_t offset = 1 + (random_.next() % (static_cast<uint8_t>(Species::Count) - 1));
    child.species = static_cast<Species>((static_cast<uint8_t>(parent.species) + offset) %
                                        static_cast<uint8_t>(Species::Count));
    ++stats_.mutations;
  }
  child.id = nextId_++;
  child.parentId = parent.id;
  child.lineageId = parent.lineageId;
  child.generation = parent.generation < UINT16_MAX ? parent.generation + 1 : UINT16_MAX;
  child.children = 0;
  child.carrionEaten = 0.0f;
  child.attacks = 0;
  child.kills = 0;
  child.kinKills = 0;
  child.colonyNeighbors = 0;
  child.alive = true;
  if (parent.children < UINT16_MAX) ++parent.children;
}

void World::spawnFood(uint16_t slot) {
  const float capacity = random_.range(Config::FOOD_INITIAL_MIN, Config::FOOD_INITIAL_MAX);
  food_[slot] = {random_.range(5.0f, Config::WORLD_WIDTH - 5.0f),
                 random_.range(5.0f, Config::WORLD_HEIGHT - 5.0f), capacity, capacity, true};
}

void World::spawnToxin(uint8_t slot) {
  toxins_[slot] = {random_.range(Config::TOXIN_RADIUS,
                                 Config::WORLD_WIDTH - Config::TOXIN_RADIUS),
                    random_.range(Config::TOXIN_RADIUS,
                                  Config::WORLD_HEIGHT - Config::TOXIN_RADIUS),
                    0.0f,
                    random_.range(Config::TOXIN_LIFETIME_MIN,
                                  Config::TOXIN_LIFETIME_MAX),
                    random_.range(Config::TOXIN_POTENCY_MIN,
                                  Config::TOXIN_POTENCY_MAX),
                    true};
}

void World::setToxinsEnabled(bool enabled) {
  toxinsEnabled_ = enabled;
  if (enabled) {
    for (uint8_t i = 0; i < Config::TOXIN_SOURCES; ++i) spawnToxin(i);
  } else {
    for (uint8_t i = 0; i < Config::TOXIN_SOURCES; ++i) toxins_[i].active = false;
  }
}

void World::addFoodSources(uint8_t count) {
  while (count > 0) {
    int16_t slot = -1;
    for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) {
      if (!food_[i].active) { slot = static_cast<int16_t>(i); break; }
    }
    if (slot < 0) {
      slot = 0;
      for (uint16_t i = 1; i < Config::FOOD_SOURCES; ++i) {
        if (food_[i].energy < food_[slot].energy) slot = static_cast<int16_t>(i);
      }
    }
    spawnFood(static_cast<uint16_t>(slot));
    --count;
  }
  updateStats();
}

int16_t World::findBacteriumNear(float x, float y, float radius) const {
  int16_t best = -1;
  float best2 = radius * radius;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria_[i].alive) continue;
    const float dx = bacteria_[i].x - x;
    const float dy = bacteria_[i].y - y;
    const float d2 = dx * dx + dy * dy;
    if (d2 <= best2) { best2 = d2; best = static_cast<int16_t>(i); }
  }
  return best;
}

int16_t World::findNearestBacterium(float x, float y) const {
  int16_t best = -1;
  float best2 = 1.0e30f;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria_[i].alive) continue;
    const float dx = bacteria_[i].x - x;
    const float dy = bacteria_[i].y - y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best2) {
      best2 = d2;
      best = static_cast<int16_t>(i);
    }
  }
  return best;
}

int16_t World::findBacteriumById(uint32_t id) const {
  if (id == 0) return -1;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (bacteria_[i].alive && bacteria_[i].id == id) return static_cast<int16_t>(i);
  }
  return -1;
}

int16_t World::findNextAlive(int16_t currentIndex) const {
  const uint16_t start = currentIndex < 0
      ? 0
      : static_cast<uint16_t>((currentIndex + 1) % Config::MAX_BACTERIA);
  for (uint16_t offset = 0; offset < Config::MAX_BACTERIA; ++offset) {
    const uint16_t index = static_cast<uint16_t>((start + offset) % Config::MAX_BACTERIA);
    if (bacteria_[index].alive) return static_cast<int16_t>(index);
  }
  return -1;
}

int16_t World::findFreeBacterium() const {
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) if (!bacteria_[i].alive) return i;
  return -1;
}

int16_t World::nearestFood(float x, float y, float limit2) const {
  int16_t best = -1;
  float best2 = limit2;
  for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) {
    if (!food_[i].active || food_[i].energy <= 0.0f) continue;
    const float dx = food_[i].x - x;
    const float dy = food_[i].y - y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best2) { best2 = d2; best = i; }
  }
  return best;
}

int16_t World::nearestBiomass(float x, float y, float limit2) const {
  int16_t best = -1;
  float best2 = limit2;
  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    if (!biomass_[i].active || biomass_[i].energy <= 0.0f ||
        biomass_[i].protectedTime > 0.0f) continue;
    const float dx = biomass_[i].x - x;
    const float dy = biomass_[i].y - y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best2) { best2 = d2; best = i; }
  }
  return best;
}

void World::buildSpatialIndex() {
  for (uint16_t cell = 0; cell < Config::SPATIAL_CELLS; ++cell) spatialHead_[cell] = -1;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    spatialNext_[i] = -1;
    if (!bacteria_[i].alive) continue;
    uint8_t col = static_cast<uint8_t>(bacteria_[i].x / Config::SPATIAL_CELL_SIZE);
    uint8_t row = static_cast<uint8_t>(bacteria_[i].y / Config::SPATIAL_CELL_SIZE);
    if (col >= Config::SPATIAL_COLS) col = Config::SPATIAL_COLS - 1;
    if (row >= Config::SPATIAL_ROWS) row = Config::SPATIAL_ROWS - 1;
    const uint16_t cell = static_cast<uint16_t>(row) * Config::SPATIAL_COLS + col;
    spatialNext_[i] = spatialHead_[cell];
    spatialHead_[cell] = static_cast<int16_t>(i);
  }
}

int16_t World::nearbyPrey(uint16_t hunterIndex) const {
  const Bacterium& hunter = bacteria_[hunterIndex];
  int16_t best = -1;
  float best2 = Config::ATTACK_RANGE * Config::ATTACK_RANGE;
  const int baseCol = static_cast<int>(hunter.x / Config::SPATIAL_CELL_SIZE);
  const int baseRow = static_cast<int>(hunter.y / Config::SPATIAL_CELL_SIZE);
  for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
    const int row = baseRow + rowOffset;
    if (row < 0 || row >= Config::SPATIAL_ROWS) continue;
    for (int colOffset = -1; colOffset <= 1; ++colOffset) {
      const int col = baseCol + colOffset;
      if (col < 0 || col >= Config::SPATIAL_COLS) continue;
      const uint16_t cell = static_cast<uint16_t>(row * Config::SPATIAL_COLS + col);
      int16_t candidate = spatialHead_[cell];
      while (candidate >= 0) {
        if (candidate != static_cast<int16_t>(hunterIndex) && bacteria_[candidate].alive) {
          const float dx = bacteria_[candidate].x - hunter.x;
          const float dy = bacteria_[candidate].y - hunter.y;
          const float d2 = dx * dx + dy * dy;
          if (d2 <= best2) { best2 = d2; best = candidate; }
        }
        candidate = spatialNext_[candidate];
      }
    }
  }
  return best;
}

uint8_t World::nearbyKin(uint16_t bacteriumIndex, float& centerX, float& centerY) const {
  const Bacterium& b = bacteria_[bacteriumIndex];
  const float range2 = Config::COLONY_RANGE * Config::COLONY_RANGE;
  float sumX = 0.0f;
  float sumY = 0.0f;
  uint8_t count = 0;
  const int baseCol = static_cast<int>(b.x / Config::SPATIAL_CELL_SIZE);
  const int baseRow = static_cast<int>(b.y / Config::SPATIAL_CELL_SIZE);
  for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
    const int row = baseRow + rowOffset;
    if (row < 0 || row >= Config::SPATIAL_ROWS) continue;
    for (int colOffset = -1; colOffset <= 1; ++colOffset) {
      const int col = baseCol + colOffset;
      if (col < 0 || col >= Config::SPATIAL_COLS) continue;
      int16_t candidate = spatialHead_[row * Config::SPATIAL_COLS + col];
      while (candidate >= 0) {
        const Bacterium& relative = bacteria_[candidate];
        if (candidate != static_cast<int16_t>(bacteriumIndex) && relative.alive &&
            relative.lineageId == b.lineageId) {
          const float dx = relative.x - b.x;
          const float dy = relative.y - b.y;
          if (dx * dx + dy * dy <= range2) {
            sumX += relative.x;
            sumY += relative.y;
            if (count < UINT8_MAX) ++count;
          }
        }
        candidate = spatialNext_[candidate];
      }
    }
  }
  centerX = count > 0 ? sumX / count : b.x;
  centerY = count > 0 ? sumY / count : b.y;
  return count;
}

void World::leaveBiomass(const Bacterium& bacterium, float energy, DeathCause cause) {
  int16_t weakest = 0;
  bool foundFree = false;
  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    if (!biomass_[i].active) { weakest = i; foundFree = true; break; }
    if (biomass_[i].energy < biomass_[weakest].energy) weakest = i;
  }
  if (!foundFree) ++stats_.biomassReplacements;
  const float stored = fminf(Config::BIOMASS_MAX_ENERGY,
                             Config::BIOMASS_BASE_ENERGY +
                             fmaxf(0.0f, energy) * Config::BIOMASS_FRACTION);
  const uint8_t deathRadius = Config::bacteriumRadius(fmaxf(0.0f, energy));
  const uint32_t deathSecond =
      (stats_.step * Config::SIMULATION_STEP_MS) / 1000;
  biomass_[weakest] = {bacterium.x, bacterium.y, stored, stored,
                       Config::BIOMASS_VISIBLE_SECONDS, deathRadius, cause,
                       bacterium.id, bacterium.parentId, bacterium.lineageId,
                       deathSecond, bacterium.generation, bacterium.species, true};
}

void World::update(float dt) {
  ++stats_.step;
  const float eat2 = Config::EAT_RANGE * Config::EAT_RANGE;
  buildSpatialIndex();

  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    Bacterium& b = bacteria_[i];
    if (!b.alive) continue;

    b.age += dt;
    if (b.reproductionCooldown > 0.0f) b.reproductionCooldown -= dt;
    if (b.attackCooldown > 0.0f) b.attackCooldown -= dt;
    const float resistanceCost = 1.0f + Config::TOXIN_RESISTANCE_COST * b.genome.toxinResistance;
    const float sensorCost = Config::TOXIN_SENSOR_COST *
                             (b.genome.toxinSensorRange / 46.0f) *
                             (0.5f + 0.5f * b.genome.toxinSensitivity) +
                             0.03f * (b.genome.toxinAvoidance / 0.10f) +
                             0.01f * (b.genome.toxinTumbleRate / 4.0f);
    b.energy -= (b.genome.metabolism * resistanceCost + sensorCost +
                 Config::MOVE_COST * b.genome.speed) * dt;

    const float sensor2 = b.genome.sensorRange * b.genome.sensorRange;
    const int16_t foodIndex = nearestFood(b.x, b.y, sensor2);
    const int16_t bioIndex = carrionEnabled_ ? nearestBiomass(b.x, b.y, sensor2) : -1;
    const bool hungry = b.energy < b.genome.reproductionEnergy * Config::HUNT_ENERGY_FRACTION;
    const bool canHunt = warEnabled_ && hungry;
    const int16_t preyIndex = canHunt ? nearbyPrey(i) : -1;
    float kinCenterX = b.x;
    float kinCenterY = b.y;
    const bool colonyUpdateDue =
        ((stats_.step + b.id) % Config::COLONY_UPDATE_INTERVAL_STEPS) == 0;
    if (colonyUpdateDue) b.colonyNeighbors = nearbyKin(i, kinCenterX, kinCenterY);
    float targetX = 0.0f;
    float targetY = 0.0f;
    bool hasTarget = false;
    bool targetIsBiomass = false;

    if (foodIndex >= 0) {
      targetX = food_[foodIndex].x; targetY = food_[foodIndex].y; hasTarget = true;
    }
    if (bioIndex >= 0) {
      const float fdx = hasTarget ? targetX - b.x : 1e6f;
      const float fdy = hasTarget ? targetY - b.y : 1e6f;
      const float bdx = biomass_[bioIndex].x - b.x;
      const float bdy = biomass_[bioIndex].y - b.y;
      const float affinityBias = 0.55f + b.genome.carrionAffinity;
      if (!hasTarget || (bdx * bdx + bdy * bdy) / affinityBias < fdx * fdx + fdy * fdy) {
        targetX = biomass_[bioIndex].x; targetY = biomass_[bioIndex].y;
        hasTarget = true; targetIsBiomass = true;
      }
    }
    if (hasTarget) {
      const float desired = atan2f(targetY - b.y, targetX - b.x);
      const float attraction = targetIsBiomass
                                   ? 0.08f + 0.16f * b.genome.carrionAffinity
                                   : 0.13f;
      b.direction += angleDelta(desired, b.direction) * attraction;
    }
    if (colonyUpdateDue && b.colonyNeighbors > 0 && b.genome.colonyAffinity > 0.05f) {
      const float kinDirection = atan2f(kinCenterY - b.y, kinCenterX - b.x);
      b.direction += angleDelta(kinDirection, b.direction) *
                     (0.035f + 0.075f * b.genome.colonyAffinity);
    }

    if (preyIndex >= 0 && bacteria_[preyIndex].alive && b.attackCooldown <= 0.0f) {
      Bacterium& victim = bacteria_[preyIndex];
      const bool attackingKin = victim.lineageId == b.lineageId;
      const float dx = victim.x - b.x;
      const float dy = victim.y - b.y;
      if (dx * dx + dy * dy <= Config::ATTACK_RANGE * Config::ATTACK_RANGE &&
          random_.chance((Config::WAR_BASE_AGGRESSION +
                          (1.0f - Config::WAR_BASE_AGGRESSION) * b.genome.aggressiveness) *
                         Config::ATTACKS_PER_SECOND * dt)) {
        const float victimEnergyBefore = fmaxf(0.0f, victim.energy);
        const float damage = fminf(victimEnergyBefore, b.genome.attackPower);
        victim.energy -= b.genome.attackPower;
        const float transferred = damage * Config::DIRECT_FEED_FRACTION;
        b.energy += transferred;
        stats_.energyStolen += transferred;
        ++stats_.attackTransfers;
        b.energy -= Config::ATTACK_ENERGY_COST;
        b.attackCooldown = Config::ATTACK_COOLDOWN_SECONDS;
        if (b.attacks < UINT16_MAX) ++b.attacks;
        ++stats_.attacks;
        if (attackingKin) ++stats_.kinAttacks;
        if (victim.energy <= 0.0f) {
          leaveBiomass(victim, fmaxf(4.0f, victimEnergyBefore - transferred),
                       DeathCause::KIA);
          victim.alive = false;
          if (b.kills < UINT16_MAX) ++b.kills;
          ++stats_.kills;
          if (attackingKin) {
            if (b.kinKills < UINT16_MAX) ++b.kinKills;
            ++stats_.kinKills;
          }
          ++stats_.attackDeaths;
          ++stats_.deaths;
        }
      }
    }
    if (random_.chance(b.genome.tumbleRate * dt)) {
      b.direction += random_.range(-1.35f, 1.35f);
    }

    float toxinSignal = 0.0f;
    float avoidX = 0.0f;
    float avoidY = 0.0f;
    if (toxinsEnabled_) {
      const float sensorRange2 = b.genome.toxinSensorRange * b.genome.toxinSensorRange;
      for (uint8_t toxinIndex = 0; toxinIndex < Config::TOXIN_SOURCES; ++toxinIndex) {
        if (!toxins_[toxinIndex].active) continue;
        const float awayX = b.x - toxins_[toxinIndex].x;
        const float awayY = b.y - toxins_[toxinIndex].y;
        const float distance2 = awayX * awayX + awayY * awayY;
        if (distance2 <= 0.0001f || distance2 > sensorRange2) continue;
        const float distance = sqrtf(distance2);
        const float localSignal = toxins_[toxinIndex].potency *
                                  (1.0f - distance / b.genome.toxinSensorRange) *
                                  b.genome.toxinSensitivity;
        toxinSignal += localSignal;
        avoidX += awayX / distance * localSignal;
        avoidY += awayY / distance * localSignal;
      }
    }
    if (toxinSignal > 0.0f && (avoidX != 0.0f || avoidY != 0.0f)) {
      const float sensitivity = 1.0f - b.genome.toxinResistance;
      const float safeDirection = atan2f(avoidY, avoidX);
      b.direction += angleDelta(safeDirection, b.direction) *
                     b.genome.toxinAvoidance * sensitivity;
      ++stats_.toxinAvoidanceTurns;
      if (toxinSignal > b.lastToxinSignal + Config::TOXIN_SIGNAL_RISE &&
          random_.chance(b.genome.toxinTumbleRate * sensitivity * dt)) {
        b.direction += random_.range(-PI_F, PI_F);
        ++stats_.toxinTumbles;
      }
    }
    b.lastToxinSignal = toxinSignal;

    b.x = wrap(b.x + cosf(b.direction) * b.genome.speed * dt, Config::WORLD_WIDTH);
    b.y = wrap(b.y + sinf(b.direction) * b.genome.speed * dt, Config::WORLD_HEIGHT);

    bool lethalToxin = false;
    const float toxinRadius2 = Config::TOXIN_RADIUS * Config::TOXIN_RADIUS;
    for (uint8_t toxinIndex = 0; toxinIndex < Config::TOXIN_SOURCES; ++toxinIndex) {
      if (!toxins_[toxinIndex].active) continue;
      const float toxinDx = toxins_[toxinIndex].x - b.x;
      const float toxinDy = toxins_[toxinIndex].y - b.y;
      const float toxinDistance2 = toxinDx * toxinDx + toxinDy * toxinDy;
      if (toxinDistance2 > toxinRadius2) continue;
      const float falloff = 1.0f - sqrtf(toxinDistance2) / Config::TOXIN_RADIUS;
      const float damage = Config::TOXIN_DAMAGE_PER_SECOND * toxins_[toxinIndex].potency *
                           falloff * (1.0f - b.genome.toxinResistance) * dt;
      b.energy -= damage;
      stats_.toxinDamage += damage;
      ++stats_.toxinExposures;
      if (b.energy <= 0.0f) lethalToxin = true;
    }

    if (lethalToxin) {
      leaveBiomass(b, 4.0f, DeathCause::Toxin);
      b.alive = false;
      ++stats_.deaths;
      ++stats_.toxinDeaths;
      continue;
    }

    if (hasTarget) {
      const float dx = targetX - b.x;
      const float dy = targetY - b.y;
      if (dx * dx + dy * dy <= eat2) {
        const float wanted = Config::EAT_RATE * dt;
        float* energy = targetIsBiomass ? &biomass_[bioIndex].energy : &food_[foodIndex].energy;
        const float eaten = fminf(*energy, wanted);
        *energy -= eaten;
        if (targetIsBiomass) {
          const float gained = eaten * b.genome.carrionEfficiency;
          b.energy += gained;
          b.carrionEaten += eaten;
          stats_.carrionConsumed += eaten;
        } else {
          b.energy += eaten;
        }
        if (*energy <= 0.01f) {
          if (targetIsBiomass) biomass_[bioIndex].active = false;
          else food_[foodIndex].active = false;
        }
      }
    }

    if (b.energy >= b.genome.reproductionEnergy && b.reproductionCooldown <= 0.0f) {
      const int16_t child = findFreeBacterium();
      if (child >= 0) {
        b.energy -= Config::CHILD_ENERGY;
        b.reproductionCooldown = Config::REPRODUCTION_COOLDOWN_SECONDS;
        spawnChild(child, b);
        ++stats_.births;
      }
    }

    if (b.energy <= 0.0f || b.age >= b.genome.maxAge) {
      const DeathCause cause = b.energy <= 0.0f ? DeathCause::Energy : DeathCause::Age;
      leaveBiomass(b, fmaxf(4.0f, b.energy), cause);
      b.alive = false;
      ++stats_.deaths;
      if (b.energy <= 0.0f) ++stats_.energyDeaths;
      else ++stats_.ageDeaths;
    }
  }

  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    if (!biomass_[i].active) continue;
    if (biomass_[i].protectedTime > 0.0f) {
      biomass_[i].protectedTime -= dt;
      continue;
    }
    biomass_[i].energy -= Config::BIOMASS_DECAY_PER_SECOND * dt;
    if (biomass_[i].energy <= 0.0f) biomass_[i].active = false;
  }
  for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) {
    if (!food_[i].active && random_.chance(Config::FOOD_RESPAWN_PER_SECOND * dt / Config::FOOD_SOURCES)) spawnFood(i);
  }
  for (uint8_t i = 0; toxinsEnabled_ && i < Config::TOXIN_SOURCES; ++i) {
    toxins_[i].age += dt;
    if (toxins_[i].age >= toxins_[i].lifetime) spawnToxin(i);
  }
  updateStats();
}

float World::wrap(float value, float limit) {
  if (value < 0.0f) return value + limit;
  if (value >= limit) return value - limit;
  return value;
}

float World::angleDelta(float target, float current) {
  float d = target - current;
  while (d > PI_F) d -= TWO_PI_F;
  while (d < -PI_F) d += TWO_PI_F;
  return d;
}

void World::updateStats() {
  stats_.alive = 0;
  stats_.foodEnergy = 0.0f;
  stats_.biomassEnergy = 0.0f;
  stats_.activeBiomass = 0;
  stats_.maxGeneration = 0;
  stats_.colonyMembers = 0;
  stats_.largestLocalGroup = 0;
  for (uint8_t i = 0; i < static_cast<uint8_t>(Species::Count); ++i) stats_.species[i] = 0;
  for (uint16_t i = 0; i < Config::MAX_BACTERIA; ++i) {
    if (!bacteria_[i].alive) continue;
    ++stats_.alive;
    ++stats_.species[static_cast<uint8_t>(bacteria_[i].species)];
    if (bacteria_[i].generation > stats_.maxGeneration) stats_.maxGeneration = bacteria_[i].generation;
    if (bacteria_[i].colonyNeighbors >= Config::COLONY_MIN_NEIGHBORS) ++stats_.colonyMembers;
    if (bacteria_[i].colonyNeighbors + 1 > stats_.largestLocalGroup) {
      stats_.largestLocalGroup = bacteria_[i].colonyNeighbors + 1;
    }
  }
  for (uint16_t i = 0; i < Config::FOOD_SOURCES; ++i) if (food_[i].active) stats_.foodEnergy += food_[i].energy;
  for (uint16_t i = 0; i < Config::MAX_BIOMASS; ++i) {
    if (!biomass_[i].active) continue;
    stats_.biomassEnergy += biomass_[i].energy;
    ++stats_.activeBiomass;
  }
}

}  // namespace BacteriaLab
