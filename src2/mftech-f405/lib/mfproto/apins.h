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
  PinName pin_name;
  static HardwareTimer *T;
  static bool timerInited;
  static uint32_t timerFreq, prevms;
  KalmanFilter *kf = nullptr;
  KalmanFilter *sf = nullptr;
  uint32_t value = 0;
  uint32_t lastValue = 0;
  uint32_t lastAverage = 0;
  float value_f = 0.0f;
  float initMS, initNoise;
  float speedLastValue = 0.0f;
  float currentSpeed = 0.0f;
  uint32_t speedLastTime = HAL_GetTick();
#ifdef STM32F4xx
  ADC_TypeDef *ADCInstance = NP;
  uint32_t ADCChannel = 0;
  uint16_t f4_adc_read(void);
  uint16_t *dmaValue = nullptr;
#endif

public:
  inline static std::vector<aPins *> instances = {};
  static uint64_t timerCounter;
  bool active = true;
  char name[32] = ""; // pin name, filled by APIN define, can be
                      // used for debug purposes
  aPins(const char *cname, uint16_t _PIN, float measure_estimate = 1.0,
        float q = 0.1);
  void read(void);
  void readMany(uint32_t count = 100);
  uint32_t get(void);
  uint32_t getA(void);
  float getFloat(void);
  uint32_t getRaw(void) { return getLast(); };
  void stop(void);
  void start(void);
  void setEstimate(float est);
  void setNoise(float q);
  float getSpeed(void);
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
  void setDMAValue(uint16_t *value) { dmaValue = value; };
  void resetDMAValue() { dmaValue = nullptr; }
};

#define APIN(a, ...) aPins(a)(#a, __VA_ARGS__)

#endif
