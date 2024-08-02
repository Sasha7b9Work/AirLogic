#ifndef MF_WHEEL_H
#define MF_WHEEL_H
#include "mfcomp.h"
#include "mfsensor.h"
#include "mfvalve.h"

enum mfWheelDirection { MFWHEEL_UP, MFWHEEL_DOWN, MFWHEEL_END };
struct mfWheelPoint {
  float position = 0.0;
  uint32_t time = 0;
  bool actual = false;
  mfWheelDirection direction;
};
struct mfWheelMovement {
  float delta = 0;
  uint32_t time = 0;
};

struct mfWheelConfig {
  mfValve *up = nullptr;
  mfValve *down = nullptr;
  // for 3/6 up and down should be configured with the same valve

  bool receiverPresent = false; // do we have receiver?
  mfValve *receiver = nullptr;  // do we have receiver valve?

  bool commonExhaust = false; // should be true if no receiver present???
  // fix it

  mfBusCompControl *comp = nullptr;
  bool wabco8ch = false;
  mfSensor *sensor = nullptr;
  bool present = false;
};

struct mfWheelStatus {
  bool on = false;
  uint32_t time = TICKS();
  bool overloaded = false;
};

class mfWheel {
private:
  mfWheelStatus upState, downState;
  mfWheelPoint startPoint, speedPrevPoint, speedLastPoint;
  uint32_t speedStep = 1000;
  float lastSpeed = 0.0;
  mfWheelMovement max, min;
  uint32_t moveStartTime = 0, moveEndTime = 0;
  float moveRange = 0;
  uint32_t stuckCounter = 0;

public:
  mfWheelConfig config;
  bool &present = config.present;
  uint32_t upTimer = TICKS(), downTimer = TICKS();
  // for some kind of manual operations keep it public
  inline static std::vector<mfWheel *> instances = {};
  char name[32] = "";
  mfWheel(const char *cname, mfWheelConfig _config);
  ~mfWheel();

  void upOn();
  // start moving up
  void upOff();
  // end moving up
  bool isUpOn();
  // is moving up started?
  uint32_t upOnTime();
  // how much time lasts since it started moving up
  void downOn();
  // start moving down
  void downOff();
  // stop moving down
  bool isDownOn();
  // is moving down started?
  uint32_t downOnTime();
  // how much time lasts since it started moving down
  bool isUpOverloaded() { return upState.overloaded; };
  bool isDownOverloaded() { return downState.overloaded; };
  float getCurrentSpeed();
  bool isStartedMoving();
  bool isStoppedMoving();
  float getLastMoveRange();
  uint32_t getLastMoveStartTime();
  uint32_t getLastMoveEndTime();
  bool isStucked();
};

#define MFWHEEL(a, ...) mfWheel(a)(#a, __VA_ARGS__)

#endif