#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

#include "src/Config.h"
#include "src/Controls.h"
#include "src/Display.h"
#include "src/SerialConsole.h"
#include "src/SDLogger.h"
#include "src/World.h"

TFT_eSPI tft;
TFT_eSprite frame(&tft);
BacteriaLab::World world;
BacteriaLab::Display display(tft, frame);
BacteriaLab::Controls controls;
BacteriaLab::SerialConsole console;
BacteriaLab::SDLogger sdLogger;

bool simulationRunning = true;
bool autoStatsEnabled = false;
int16_t serialBrowseIndex = -1;

uint32_t lastSimulationMs = 0;
uint32_t lastFrameMs = 0;
uint32_t lastStatsMs = 0;

struct EvolutionSample {
  uint32_t seconds;
  uint16_t alive;
  uint16_t species[4];
  uint16_t maxGeneration;
  float resistance;
  float sensorRange;
  float sensitivity;
  float avoidance;
  float tumbleRate;
};

constexpr uint8_t EVOLUTION_SAMPLES = 24;
constexpr uint32_t EVOLUTION_INTERVAL_STEPS =
    60000 / BacteriaLab::Config::SIMULATION_STEP_MS;
EvolutionSample evolutionLog[EVOLUTION_SAMPLES] = {};
uint8_t evolutionCount = 0;
uint8_t evolutionNext = 0;
uint32_t lastEvolutionStep = 0;

void recordEvolution(uint32_t now) {
  EvolutionSample& sample = evolutionLog[evolutionNext];
  sample = EvolutionSample{};
  const BacteriaLab::Statistics& stats = world.stats();
  sample.seconds = (stats.step * BacteriaLab::Config::SIMULATION_STEP_MS) / 1000;
  sample.alive = stats.alive;
  sample.maxGeneration = stats.maxGeneration;
  for (uint8_t i = 0; i < 4; ++i) sample.species[i] = stats.species[i];
  const BacteriaLab::Bacterium* bacteria = world.bacteria();
  uint16_t count = 0;
  for (uint16_t i = 0; i < BacteriaLab::Config::MAX_BACTERIA; ++i) {
    if (!bacteria[i].alive) continue;
    sample.resistance += bacteria[i].genome.toxinResistance;
    sample.sensorRange += bacteria[i].genome.toxinSensorRange;
    sample.sensitivity += bacteria[i].genome.toxinSensitivity;
    sample.avoidance += bacteria[i].genome.toxinAvoidance;
    sample.tumbleRate += bacteria[i].genome.toxinTumbleRate;
    ++count;
  }
  if (count > 0) {
    sample.resistance /= count;
    sample.sensorRange /= count;
    sample.sensitivity /= count;
    sample.avoidance /= count;
    sample.tumbleRate /= count;
  }
  evolutionNext = (evolutionNext + 1) % EVOLUTION_SAMPLES;
  if (evolutionCount < EVOLUTION_SAMPLES) ++evolutionCount;
  lastEvolutionStep = stats.step;
}

void clearEvolution(uint32_t now) {
  evolutionCount = 0;
  evolutionNext = 0;
  recordEvolution(now);
}

void printEvolution() {
  Serial.println("EVOLUTION LOG (E/S/S/B species)");
  Serial.println("sec alive species maxgen resist range sens avoid tumble");
  const uint8_t first = evolutionCount < EVOLUTION_SAMPLES ? 0 : evolutionNext;
  for (uint8_t row = 0; row < evolutionCount; ++row) {
    const EvolutionSample& s = evolutionLog[(first + row) % EVOLUTION_SAMPLES];
    Serial.print(s.seconds); Serial.print(' ');
    Serial.print(s.alive); Serial.print(' ');
    Serial.print(s.species[0]); Serial.print('/'); Serial.print(s.species[1]);
    Serial.print('/'); Serial.print(s.species[2]); Serial.print('/'); Serial.print(s.species[3]); Serial.print(' ');
    Serial.print(s.maxGeneration); Serial.print(' ');
    Serial.print(s.resistance, 3); Serial.print(' ');
    Serial.print(s.sensorRange, 1); Serial.print(' ');
    Serial.print(s.sensitivity, 3); Serial.print(' ');
    Serial.print(s.avoidance, 3); Serial.print(' ');
    Serial.println(s.tumbleRate, 3);
  }
}

void openSerialSnapshot(int16_t index, uint32_t now) {
  if (index < 0) {
    Serial.println("Bacterium not found");
    return;
  }
  serialBrowseIndex = index;
  console.printBacterium(Serial, world.bacteria()[index]);
}

void handleSerial(uint32_t now) {
  const BacteriaLab::SerialCommand command = console.poll(Serial);
  switch (command.type) {
    case BacteriaLab::SerialCommandType::Help:
      console.printHelp(Serial);
      break;
    case BacteriaLab::SerialCommandType::Stats:
      console.printStats(Serial, world, simulationRunning);
      break;
    case BacteriaLab::SerialCommandType::StatsOn:
      autoStatsEnabled = true;
      lastStatsMs = now;
      console.printStats(Serial, world, simulationRunning);
      Serial.println("Automatic statistics ON");
      break;
    case BacteriaLab::SerialCommandType::StatsOff:
      autoStatsEnabled = false;
      Serial.println("Automatic statistics OFF");
      break;
    case BacteriaLab::SerialCommandType::List:
      console.startList(Serial);
      break;
    case BacteriaLab::SerialCommandType::Show:
      openSerialSnapshot(world.findBacteriumById(command.id), now);
      break;
    case BacteriaLab::SerialCommandType::Next:
      openSerialSnapshot(world.findNextAlive(serialBrowseIndex), now);
      break;
    case BacteriaLab::SerialCommandType::Pause:
      simulationRunning = false;
      lastSimulationMs = now;
      Serial.println("Simulation PAUSED");
      break;
    case BacteriaLab::SerialCommandType::Run:
      simulationRunning = true;
      lastSimulationMs = now;
      Serial.println("Simulation RUNNING");
      break;
    case BacteriaLab::SerialCommandType::Food:
      world.addFoodSources(4);
      Serial.println("Added 4 food sources");
      break;
    case BacteriaLab::SerialCommandType::Reset:
      if (sdLogger.universeEnabled()) sdLogger.stopUniverse(Serial);
      world.reset(esp_random());
      simulationRunning = true;
      serialBrowseIndex = -1;
      lastSimulationMs = now;
      Serial.println("World RESET");
      break;
    case BacteriaLab::SerialCommandType::Predators:
      console.printPredators(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::Deaths:
      console.printDeathStats(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::PoisonOn:
      world.setToxinsEnabled(true);
      Serial.println("Poison ON");
      break;
    case BacteriaLab::SerialCommandType::PoisonOff:
      world.setToxinsEnabled(false);
      Serial.println("Poison OFF");
      break;
    case BacteriaLab::SerialCommandType::PoisonStatus:
      console.printToxinStats(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::WarOn:
      world.setWarEnabled(true);
      Serial.println("War ON");
      break;
    case BacteriaLab::SerialCommandType::WarOff:
      world.setWarEnabled(false);
      Serial.println("War OFF");
      break;
    case BacteriaLab::SerialCommandType::CarrionOn:
      world.setCarrionEnabled(true);
      Serial.println("Carrion feeding ON");
      break;
    case BacteriaLab::SerialCommandType::CarrionOff:
      world.setCarrionEnabled(false);
      Serial.println("Carrion feeding OFF");
      break;
    case BacteriaLab::SerialCommandType::Systems:
      console.printSystems(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::Genes:
      console.printGenes(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::Evolution:
      recordEvolution(now);
      printEvolution();
      break;
    case BacteriaLab::SerialCommandType::ExperimentReset:
      if (sdLogger.universeEnabled()) sdLogger.stopUniverse(Serial);
      world.reset(esp_random());
      simulationRunning = true;
      serialBrowseIndex = -1;
      lastSimulationMs = now;
      clearEvolution(now);
      Serial.println("Experiment RESET: world and evolution log cleared");
      console.printSystems(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::BiomassStatus:
      console.printBiomassStats(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::SdStatus:
      sdLogger.printStatus(Serial);
      break;
    case BacteriaLab::SerialCommandType::LogOn:
      sdLogger.startUniverse(Serial, world, simulationRunning);
      break;
    case BacteriaLab::SerialCommandType::LogOff:
      sdLogger.stopUniverse(Serial);
      break;
    case BacteriaLab::SerialCommandType::LogNow:
      Serial.println("Full frames are automatic every second while REC is ON");
      break;
    case BacteriaLab::SerialCommandType::Snapshot:
      sdLogger.snapshot(Serial, world);
      break;
    case BacteriaLab::SerialCommandType::Invalid:
      Serial.println("Unknown command. Type: help");
      break;
    default:
      break;
  }
}

void fatalError(const char* message) {
  Serial.printf("FATAL: %s\n", message);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(message, tft.width() / 2, tft.height() / 2, 2);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  tft.init();
  tft.setRotation(BacteriaLab::Config::SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);

  if (!psramFound()) {
    fatalError("PSRAM NOT FOUND");
  }
  if (!world.begin()) {
    fatalError("WORLD ALLOCATION FAILED");
  }
  if (!display.begin()) {
    fatalError("FRAMEBUFFER FAILED");
  }
  controls.begin();

  world.reset(esp_random());
  const uint32_t now = millis();
  lastSimulationMs = lastFrameMs = lastStatsMs = now;
  clearEvolution(now);
  Serial.println("ESP32 Bacteria Lab v0.15.4 FNK0104S Species Colors");
  Serial.printf("PSRAM: %u bytes free\n", ESP.getFreePsram());
  console.printHelp(Serial);
  console.printSystems(Serial, world);
  sdLogger.begin(Serial);
}

void loop() {
  const uint32_t now = millis();
  handleSerial(now);
  console.continueList(Serial, world, 2);

  const BacteriaLab::ControlEvent event = controls.poll();
  switch (event.action) {
    case BacteriaLab::ControlAction::ToggleRun:
      simulationRunning = !simulationRunning;
      lastSimulationMs = now;
      break;
    case BacteriaLab::ControlAction::Reset:
      if (sdLogger.universeEnabled()) sdLogger.stopUniverse(Serial);
      world.reset(esp_random());
      simulationRunning = true;
      lastSimulationMs = now;
      break;
    case BacteriaLab::ControlAction::ToggleRecord:
      if (sdLogger.universeEnabled()) {
        sdLogger.stopUniverse(Serial);
      } else {
        sdLogger.startUniverse(Serial, world, simulationRunning);
      }
      break;
    default:
      break;
  }

  // Fixed simulation step keeps biology independent of display speed.
  uint8_t simulationSteps = 0;
  while (simulationRunning &&
         now - lastSimulationMs >= BacteriaLab::Config::SIMULATION_STEP_MS &&
         simulationSteps < BacteriaLab::Config::MAX_SIM_STEPS_PER_LOOP) {
    lastSimulationMs += BacteriaLab::Config::SIMULATION_STEP_MS;
    world.update(BacteriaLab::Config::SIMULATION_STEP_SECONDS);
    ++simulationSteps;
  }
  if (simulationRunning &&
      now - lastSimulationMs >= BacteriaLab::Config::SIMULATION_STEP_MS) {
    // Drop accumulated lag instead of entering an endless catch-up spiral.
    lastSimulationMs = now;
  }
  if (!simulationRunning) lastSimulationMs = now;

  sdLogger.update(world, simulationRunning);

  if (now - lastFrameMs >= BacteriaLab::Config::FRAME_INTERVAL_MS) {
    lastFrameMs = now;
    display.render(world, simulationRunning, sdLogger.universeEnabled(),
                   sdLogger.universeRecordingSeconds(world));
  }

  if (autoStatsEnabled && !console.isListing() && now - lastStatsMs >= 5000) {
    lastStatsMs = now;
    console.printStats(Serial, world, simulationRunning);
  }

  if (simulationRunning &&
      world.stats().step - lastEvolutionStep >= EVOLUTION_INTERVAL_STEPS) {
    recordEvolution(now);
  }

  delay(1);
}
