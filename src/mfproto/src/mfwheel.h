#ifndef MF_WHEEL_H
#define MF_WHEEL_H
#include "mfsensor.h"
#include "mfvalve.h"

class mfWheel {
private:
  mfValve *valve = nullptr;
  mfSensor *sensor = nullptr;

public:
#undef __inst__
#define __inst__ std::vector<mfWheel *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)nullptr;
  mfWheel(const char *cname, mfSensor *_SENSOR, mfValve *_VALVE);
  ~mfWheel();
};

#endif