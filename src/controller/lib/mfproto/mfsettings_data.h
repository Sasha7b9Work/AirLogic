#ifndef MF_SETTINGS_DATA_H
#define MF_SETTINGS_DATA_H

#include "mftranslation.h"

enum axleType {
  MF_FRONT_AXLE = 1,
  MF_REAR_AXLE = 2,
  MF_ALL_AXLES = MF_FRONT_AXLE | MF_REAR_AXLE
};
enum valveType {
  MF_3VALVES = 0,
  MF_4VALVES = 1,
  MF_6VALVES = 2,
  MF_8VALVES = 3,
  MF_ALL_AXLES_BIT = 1U << 1U
};
enum dryerType { MF_DRYER_NO = 0, MF_DRYER_YES = 1, MF_DRYER_COMP = 2 };

enum buttonColorType {
  MF_BLACK = 0x000000,
  MF_WHITE = 0xFFFFFF,
  MF_RED = 0xFF0000,
  MF_GREEN = 0x00FF00,
  MF_BLUE = 0x0000FF,
  MF_YELLOW = 0xFFFF00,
  MF_VIOLET = 0xFF00FF,
  MF_CYAN = 0x00FFFF,
  MF_ORANGE = 0xFF7F00,
  MF_SALAD = 0x7FFF00,
  MF_PINK = 0xFF007F,
  MF_INDIGO = 0x7F00FF,
  MF_AZURE = 0x007FFF,
  MF_SPRING = 0x00FF7F,
  MF_CORAL = 0xFF7F7F,
  MF_LIGHTBLUE = 0x7F7FFF,
  MF_LIGHTGREEN = 0x7FFF7F,
  MF_WITCHHAZEL = 0xFFFF7F,
  MF_FUCHSIA = 0xFF7FFF,
  MF_AQUAMARINE = 0x7FFFFF
};

enum mfCompOnOffValues {
  MF_COMP_ONOFF_STEP = 10,
  MF_COMP_OFF_START = 100,
  MF_COMP_OFF_END = 300,
  MF_COMP_ON_START = 70,
  MF_COMP_ON_END = 210
};

typedef struct /*__attribute__((aligned(256)))*/ {
  /* struct want to be aligned to flash page size */
  uint32_t crc32;
  uint32_t version, version_prev;

  MF_LANGUAGE language = ENGLISH;
  axleType axles = MF_ALL_AXLES;
  valveType valves = MF_8VALVES;

  bool receiverPresent = true;
  uint16_t compOnPressure = 120;
  uint16_t compOffPressure = 150;
  uint16_t compEnablePressure = 30;
  uint16_t compTempSensor = 0;
  uint8_t compTimeLimit = 0;
  uint16_t compMaxPSI = 300;

  dryerType dryer = MF_DRYER_NO;
  uint8_t dryerExhaustTime = 2;

  bool speedSensEnabled = false;
  uint16_t speedthreshold[2] = {60, 100};
  bool speedSensCalibrated = false;
  double speedSensCalibration = 0;

  bool engineSensEnabled = false;
  double engineSensVoltage = 13.0;

  bool upOnStart = false;

  bool calibration = false;
  bool autoHeightCorrection = true;

  bool buttonManualMode = false;

  buttonColorType buttonOkColor = MF_WHITE;
  buttonColorType buttonUpDownColor = MF_ORANGE;
  uint8_t buttonBrightness = 100;

  uint8_t sensitivity = 2;
  uint8_t sensitivityRange = 8;
  uint8_t delay = 5;
  uint16_t backLight = 60;

  bool wifiEnabled = false;
  bool btEnabled = false;

  uint8_t speedLock = 60;

  double voltageCorrection = 0.0;

  uint8_t presets[6][4] = {{10, 10, 10, 10}, {30, 30, 30, 30},
                           {50, 50, 50, 50}, {70, 70, 70, 70},
                           {90, 90, 90, 90}, {40, 40, 40, 40}};
  /* preset 5 is for speed treshold 2 */

  bool compressorEnabled = true;

  /* holder for sensors raw calbration data */
  float sensorRawMin[4] = {0.0, 0.0, 0.0, 0.0};
  float sensorRawMax[4] = {4095.0, 4095.0, 4095.0, 4095.0};
  float sensorAutoMin[4] = {0.0, 0.0, 0.0, 0.0};
  float sensorAutoMax[4] = {4095.0, 4095.0, 4095.0, 4095.0};

  int16_t lastPosition[4] = {50, 50, 50, 50};
  uint8_t lastPreset = 7;

  float adaptivesUp[4] = {0, 0, 0, 0};
  float adaptivesDown[4] = {0, 0, 0, 0};

  uint8_t valvesDuty = 100;

  uint8_t ridePreset[4] = {50, 50, 50, 50};
  bool serviceMode = false;

} MF_SETTINGS;

#endif