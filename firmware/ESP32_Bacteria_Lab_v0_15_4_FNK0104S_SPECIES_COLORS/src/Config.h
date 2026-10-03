#pragma once

#include <stdint.h>

namespace BacteriaLab::Config {

constexpr uint16_t SCREEN_WIDTH = 480;
constexpr uint16_t SCREEN_HEIGHT = 320;
constexpr uint8_t SCREEN_ROTATION = 1;
constexpr uint16_t HUD_HEIGHT = 28;
constexpr uint16_t CONTROLS_HEIGHT = 58;
constexpr uint16_t CONTROLS_TOP = SCREEN_HEIGHT - CONTROLS_HEIGHT;
constexpr float WORLD_WIDTH = 480.0f;
constexpr float WORLD_HEIGHT = static_cast<float>(SCREEN_HEIGHT - HUD_HEIGHT - CONTROLS_HEIGHT);

constexpr uint16_t INITIAL_BACTERIA = 360;
constexpr uint16_t MAX_BACTERIA = 1200;
constexpr uint16_t FOOD_SOURCES = 60;
constexpr uint8_t TOXIN_SOURCES = 10;
constexpr uint16_t MAX_BIOMASS = 384;

constexpr uint32_t SIMULATION_STEP_MS = 20;
constexpr float SIMULATION_STEP_SECONDS = 0.020f;
constexpr uint32_t FRAME_INTERVAL_MS = 50;  // 20 FPS
constexpr uint8_t MAX_SIM_STEPS_PER_LOOP = 1;

constexpr float INITIAL_ENERGY_MIN = 48.0f;
constexpr float INITIAL_ENERGY_MAX = 76.0f;
constexpr float BASE_METABOLISM = 1.15f;
constexpr float MOVE_COST = 0.055f;
constexpr float SPEED = 20.0f;
constexpr float SENSOR_RANGE = 54.0f;
constexpr float EAT_RANGE = 4.5f;
constexpr float EAT_RATE = 22.0f;
constexpr float REPRODUCTION_ENERGY = 105.0f;
constexpr float CHILD_ENERGY = 46.0f;
constexpr float MAX_AGE_SECONDS = 150.0f;
constexpr float TUMBLE_PROBABILITY_PER_SECOND = 0.65f;

constexpr float FOOD_INITIAL_MIN = 90.0f;
constexpr float FOOD_INITIAL_MAX = 180.0f;
constexpr float FOOD_RESPAWN_PER_SECOND = 0.40f;
constexpr float TOXIN_RADIUS = 14.0f;
constexpr float TOXIN_SIGNAL_RISE = 0.012f;
constexpr float TOXIN_SENSOR_COST = 0.06f;
constexpr float TOXIN_DAMAGE_PER_SECOND = 18.0f;
constexpr float TOXIN_LIFETIME_MIN = 22.0f;
constexpr float TOXIN_LIFETIME_MAX = 45.0f;
constexpr float TOXIN_POTENCY_MIN = 0.75f;
constexpr float TOXIN_POTENCY_MAX = 1.25f;
constexpr float TOXIN_RESISTANCE_COST = 0.22f;
// A cell still contains structural nutrients when its usable energy reaches zero.
constexpr float BIOMASS_BASE_ENERGY = 32.0f;
constexpr float BIOMASS_FRACTION = 0.72f;
constexpr float BIOMASS_MAX_ENERGY = 180.0f;
constexpr float BIOMASS_DECAY_PER_SECOND = 0.01f;
constexpr float BIOMASS_VISIBLE_SECONDS = 3.0f;
constexpr float CARRION_MIN_EFFICIENCY = 0.45f;
constexpr float CARRION_MAX_EFFICIENCY = 1.25f;
constexpr float ATTACK_RANGE = 5.5f;
constexpr float ATTACKS_PER_SECOND = 2.2f;
constexpr float ATTACK_COOLDOWN_SECONDS = 0.45f;
constexpr float ATTACK_ENERGY_COST = 1.8f;
constexpr float DIRECT_FEED_FRACTION = 0.32f;
constexpr float HUNT_ENERGY_FRACTION = 0.52f;
constexpr float WAR_BASE_AGGRESSION = 0.35f;
constexpr uint8_t SPATIAL_CELL_SIZE = 12;
constexpr uint8_t SPATIAL_COLS = 40;
constexpr uint8_t SPATIAL_ROWS = 20;
constexpr uint16_t SPATIAL_CELLS = SPATIAL_COLS * SPATIAL_ROWS;
constexpr float COLONY_RANGE = 16.0f;
constexpr uint8_t COLONY_MIN_NEIGHBORS = 2;
constexpr uint8_t COLONY_UPDATE_INTERVAL_STEPS = 10;

constexpr float GENE_MUTATION_CHANCE = 0.22f;
constexpr float GENE_MUTATION_STRENGTH = 0.09f;
constexpr float SPECIES_MUTATION_CHANCE = 0.018f;
constexpr float REPRODUCTION_COOLDOWN_SECONDS = 3.0f;

inline uint8_t bacteriumRadius(float energy) {
  if (energy < 40.0f) return 2;
  if (energy < 75.0f) return 3;
  if (energy < 110.0f) return 4;
  return 5;
}

}  // namespace BacteriaLab::Config
