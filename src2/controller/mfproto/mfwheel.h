#ifndef MF_WHEEL_H
#define MF_WHEEL_H
#include "mfcomp.h"
#include "mfsensor.h"
#include "mfvalve.h"

struct mfWheelConfig {
  mfValve *up = nullptr;
  mfValve *down = nullptr;
  // for 3/6 up and down should be configured with the same valve

  bool receiverPresent = false; // do we have receiver?
  mfValve *receiver = nullptr;  // do we have receiver valve?

  bool commonExhaust = false; // should be true if no receiver present???
  // fix it

  mfBusCompControl *comp = nullptr;
};

struct mfWheelStatus {
  bool on = false;
  uint32_t time = TICKS();
};

class mfWheel {
private:
  mfWheelStatus upState, downState;

public:
  mfWheelConfig config;
  // for some kind of manual operations keep it public
#undef __inst__
#define __inst__ std::vector<mfWheel *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)nullptr;
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
};

#define MFWHEEL(a, ...) mfWheel(a)(#a, __VA_ARGS__)

#endif