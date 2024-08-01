#ifndef MF_SENSOR_H
#define MF_SENSOR_H
#include "apins.h"

#define SERIAL_R 620.0    // 620R
#define PARALLEL_R 1200.0 // 1200R
#define INPUT_LIMIT 3.3   // maximum input allowed for stm32 is 3.3v
#define SENS_VOLTAGE                                                           \
  (((SERIAL_R + PARALLEL_R) * INPUT_LIMIT) / PARALLEL_R) // should be about 5V

class mfSensor {
private:
  aPins *sensor = nullptr;
  // fields for calibration
  double min = 0.0;
  double max = 4095.0; // maximum 12bit adc value

  double _getPercent();

public:
  void setMin(double _min);
  void setMin(uint32_t _min);
  void setMax(double _max);
  void setMax(uint32_t _max);
  double getMinD();
  double getMaxD();
  uint32_t getMinI();
  uint32_t getMaxI();
  uint16_t getPercentI();
  inline double getPercentD();
  double getVoltage();
  double getValueD();
  uint32_t getValueI();
  void pause() { sensor->stop(); }
  void resume() { sensor->start(); }
#undef __inst__
#define __inst__ std::vector<mfSensor *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)nullptr; // pin name, filled by define, can be
                                // used for debug purposes
  mfSensor(const char *cname, aPins *_PIN);
  ~mfSensor();
};
#endif