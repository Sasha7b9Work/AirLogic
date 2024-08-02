#ifndef MF_SENSOR_H
#define MF_SENSOR_H
#include "Kalman.h"
#include "apins.h"

/* schematics depended */
constexpr float SERIAL_R = 620.0;    // Serial divided value = 620 Ohm
constexpr float PARALLEL_R = 1200.0; // Parallel divider value = 1200 Ohm
constexpr float INPUT_LIMIT = 3.3;   // maximum input allowed for stm32 is 3.3v
constexpr float SENS_VOLTAGE = (((SERIAL_R + PARALLEL_R) * INPUT_LIMIT) /
                                PARALLEL_R); // should be about 5V
constexpr float MAX_ADC = ((1U << ADC_RESOLUTION) /* 12 bit ADC*/ - 1.0);
constexpr float vMult = (SENS_VOLTAGE / MAX_ADC);
#define MAX_SENSITIVITY_DIV 100000.0
#define MAX_SENSITIVITY MAX_SENSITIVITY_DIV / 1000.0

class mfSensor {
private:
  aPins *sensor = nullptr;
  // fields for calibration
  float low = 0.0;
  float high = MAX_ADC; // 4095 by default - maximum 12bit adc value
  float range = high - low;
  float percent = range / 100.0;
  inline float _getPercent();
  inline float __getPercent(float);
  uint32_t sensitivity = MAX_SENSITIVITY;
  float lastPercent = 0;
  float lastVoltage = 0;
  KalmanFilter *vFilt = nullptr;

public:
  // set range
  void setRange(float _low, float _high);
  // set range without margins check
  void setRangeForced(float _low, float _high);
  // set range (int)
  void setRangeV(float _low, float _high);
  // get lowest value
  float getLowD();
  // get highest value
  float getHighD();
  // get lowest value int
  uint32_t getLowI();
  // get highest value int
  uint32_t getHighI();
  // get current position in percents int
  int32_t getPercentI();
  // get current position in percents
  float getPercentD();
  // get current sensor voltage
  float getVoltage();
  // get current sensor voltage averaged
  float getVoltageA();
  // get current value
  float getValueD();
  // get current value int
  uint32_t getValueI();
  // get current raw value (w/o apprx)
  uint32_t getValueRaw() { return sensor->getRaw(); }
  // get value changing speed
  float getSpeed() { return sensor->getSpeed(); }
  // pause sensor reading
  void pause() { sensor->stop(); }
  // resume sensor reading
  void resume() { sensor->start(); }
  // set sensitivity of sensor
  void setSensitivity(uint32_t sens) {
    sensitivity = sens;
    sensor->setNoise((::fabsf(high - low) * (sensitivity)) /
                     MAX_SENSITIVITY_DIV);
  }
  // get current sensitivity
  uint32_t getSensitivity(void) { return sensitivity; }
  inline static std::vector<mfSensor *> instances = {};
  char name[32] = ""; // pin name, filled by define, can be
                      // used for debug purposes
  mfSensor(const char *cname, aPins *_PIN, float _LOW = 0.0,
           float _HIGH = MAX_ADC);
  ~mfSensor();
};

#define MFSENSOR(a, ...) mfSensor(a)(#a, __VA_ARGS__)
#endif
