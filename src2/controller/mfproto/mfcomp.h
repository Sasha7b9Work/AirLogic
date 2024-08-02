#ifndef MF_COMP_H
#define MF_COMP_H
#include "Kalman.h"
#include "apins.h"
#include "mfbus.h"
#include <atomic>
#include <cPins.h>

struct MF_CC_SETTINGS {
  uint16_t onPressure = 120;
  uint16_t offPressure = 150;
  bool dryerEnable = true;
  uint32_t dryerTime = 1000 * 1;
  bool tempControl = true;
  uint16_t overheatOffTemp = 23;
  uint16_t overheatOnTemp = 22;
  uint16_t overheatWarnTemp = 21;
  uint16_t maxPSI = 200;
  uint32_t maxCompTime = 1000 * 60 * 10;        // 10 minutes
  uint32_t compressorLockTime = 1000 * 60 * 10; // 10 minutes
  uint32_t compressorHeatLockTime = 1000 * 10;       // 10 sec
};
struct MF_CC_LOCK_T {
  volatile uint32_t time = 0;
  volatile uint32_t moment = HAL_GetTick();
  volatile bool lock = false;
  std::function<void()> callback = []() {};
};
enum MF_CC_LOCK_TYPE {
  LOCK_HEAT = 0,
  LOCK_LIMIT,
  LOCK_DRY,
  LOCK_EXT,
  LOCK_END
};

enum mfCCSignals {
  MFBUS_CC_ENABLE = MFBUS_CC_RESERVED,
  MFBUS_CC_SETTINGS,
  MFBUS_CC_GET_SETTINGS,
  MFBUS_CC_START_COMPRESSOR,
  MFBUS_CC_STOP_COMPRESSOR,
  MFBUS_CC_IS_COMP_RUNNING,
  MFBUS_CC_START_DRYER,
  MFBUS_CC_STOP_DRYER,
  MFBUS_CC_IS_DRYER_RUNNING,
  MFBUS_CC_GET_PRESSURE,
  MFBUS_CC_GET_TEMP,
  MFBUS_CC_GET_STATE,
  MFBUS_CC_LOCK_COMP,
  MFBUS_CC_LOCK_DRY,
  MFBUS_CC_UNLOCK_COMP,
  MFBUS_CC_UNLOCK_DRY,
  MFBUS_CC_IS_COMP_LOCKED,
  MFBUS_CC_IS_DRY_LOCKED
};

enum MF_CC_STATE {
  MF_CC_ENABLED = 0,
  MF_CC_OVER_LIMIT,
  MF_CC_OVERHEAT,
  MF_CC_PUMP_ON,
  MF_CC_DRY_ON,
  MF_CC_DRYER_REQUIRED,
  MF_CC_DRY_LOCK,
  MF_CC_WARN_TEMP
};
class mfCompControl {
private:
  MF_CC_LOCK_T pumpLocks[LOCK_END];
  MF_CC_LOCK_T dryLock;

  /* schematics depended */
  static constexpr auto SERIAL_R = 620.0; // Serial divided value = 620 Ohm
  static constexpr auto PARALLEL_R =
      1200.0; // Parallel divider value = 1200 Ohm
  static constexpr auto INPUT_LIMIT =
      3.3; // maximum input allowed for stm32 is 3.3v
  static constexpr auto SENS_VOLTAGE =
      (((SERIAL_R + PARALLEL_R) * INPUT_LIMIT) /
       PARALLEL_R); // should be about 5V
  static constexpr auto MAX_SENSOR_VOLTAGE = 4.5;
  KalmanFilter tempKalman = KalmanFilter(200, 200, 0.01);
  float lastPSI = 0.0;
  void pumpCallback(cPins *pin);
  void dryCallback(cPins *pin);
  uint8_t busAnswerCallback(mfBus *instance, MF_MESSAGE *msg,
                            MF_MESSAGE *newmsg);
  void tickCompCallback(cPins *pin);
  void tickDryCallback(cPins *pin);

public:
  MF_CC_SETTINGS settings;
  aPins *temp = nullptr;
  aPins *pressure = nullptr;
  cPins *pump = nullptr;
  cPins *dry = nullptr;
  volatile inline static std::atomic<uint32_t> state = 0;
  std::atomic_flag pumpMutex = ATOMIC_FLAG_INIT;
  std::atomic_flag dryMutex = ATOMIC_FLAG_INIT;
  mfCompControl(aPins *_TEMP, aPins *_PRESSURE, cPins *_PUMP, cPins *_DRY);
  ~mfCompControl();

  uint16_t getPSI(float voltage);
  uint16_t getPSI(aPins &sensor);
  float getVolt(aPins &sensor);
  static void setFlag(MF_CC_STATE flag);
  static void clearFlag(MF_CC_STATE flag);
  static bool checkFlag(MF_CC_STATE flag);

  void startCompressor(uint32_t time);
  void startCompressor();
  void stopCompressor();
  void startDryer(uint32_t time = ~0UL);
  void stopDryer();
  uint16_t getPressure(void);
  int32_t getTemp(void);
  float getTempF(void);
  void lockPump(
      MF_CC_LOCK_TYPE type, uint32_t time,
      std::function<void()> callback = []() {});
  void unlockPump(MF_CC_LOCK_TYPE type);
  void unlockPump();
  bool isPumpLocked();
  void lockDryer(
      uint32_t time, std::function<void()> callback = []() {});
  void unlockDryer();
  bool isDryerLocked();
  void processBus(mfBus &BUS);
};

class mfBusCompControl { // class to control compressor thru the bus
private:
  mfBus &bus;
  uint8_t device;
  char *name;
  uint32_t lastState = 0;

public:
  inline static std::vector<mfBusCompControl *> instances = {};
  std::vector<mfBusCompControl *>::iterator me;
  MF_CC_SETTINGS settings;
  mfBusCompControl(const char *_name, mfBus &_bus, uint8_t _device);
  ~mfBusCompControl(void);
  bool enable(bool status);
  bool enable() { return enable(true); }
  bool disable() { return enable(false); }
  void setup();
  void setup(MF_CC_SETTINGS _settings);
  void readSettings(void);
  bool startCompressor(uint32_t time = ~0UL);
  bool stopCompressor();
  bool isCompressorRunning();
  bool startDryer(uint32_t time = ~0UL);
  bool stopDryer();
  bool isDryerRunning();
  uint32_t getPressure();
  int32_t getTemp();
  uint32_t getState();
  bool checkFlag(MF_CC_STATE flag);
  void lockCompressor();
  void unlockCompressor();
  bool isCompressorLocked();
  void lockDryer();
  void unlockDryer();
  bool isDryerLocked();
};
#endif