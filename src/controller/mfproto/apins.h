#ifndef APINS_H
#define APINS_H

#include <HardwareTimer.h>
#include <sketch.h>

#include "Kalman.h"

#ifndef APINS_MAX
#define APINS_MAX 100
#endif
class aPins {
private:
  uint16_t pin;
  static HardwareTimer *T;
  static bool timerInited;
  static uint32_t timerFreq, prevms;
  KalmanFilter *kf = nullptr;
  uint32_t value = 0;
  uint32_t lastValue = 0;
  float value_f = 0.0;
  float initMS, initNoise;

public:
#undef __inst__
#define __inst__ std::vector<aPins *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  static uint64_t timerCounter;
  bool active = true;
  char *name = (char *)nullptr; // pin name, filled by APIN define, can be
                                // used for debug purposes
  aPins(const char *cname, uint16_t _PIN, float measure_estimate = 1.0,
        float q = 0.1);
  void read(void);
  void readMany(uint32_t count = 100);
  uint32_t get(void);
  float getFloat(void);
  uint32_t getRaw(void) { return getLast(); };
  void stop(void);
  void start(void);
  void setEstimate(float est);
  void setNoise(float q);
  static void timerCallback(void); // callback for timer routines
  static void
  initTimer(TIM_TypeDef *timer = TIM1,
            uint32_t freq = 60); // by default it will use TIM1 at 60Hz,
                                 // dont forget to init it at startup
  void enableKalman();
  void disableKalman();
  uint32_t getLast(void) { return lastValue; }
  static void freeTimer(void);
  ~aPins();
};

#define APIN(a, ...) aPins(a)(#a, __VA_ARGS__)

#endif
