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
  double value_f = 0.0;

public:
#undef __inst__
#define __inst__ std::vector<aPins *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  bool active = true;
  char *name = (char *)nullptr; // pin name, filled by APIN define, can be
                                // used for debug purposes
  aPins(const char *cname, uint16_t _PIN, double measure_estimate = 1.0,
        double q = 0.1);
  void read(void);
  void readMany(uint32_t count = 100);
  uint32_t get(void);
  double getFloat(void);
  void stop(void);
  void start(void);
  void setEstimate(double est);
  void setNoise(double q);
  static void timerCallback(void); // callback for timer routines
  static void
  initTimer(TIM_TypeDef *timer = TIM4,
            uint32_t freq = 60); // by default it will use TIM1 at 60Hz,
                                 // dont forget to init it at startup
  static void freeTimer(void);
  ~aPins();
};

#define APIN(a, ...) aPins(a)(#a, __VA_ARGS__)

#endif
