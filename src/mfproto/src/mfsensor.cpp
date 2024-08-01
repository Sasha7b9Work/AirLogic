#include "mfsensor.h"
/* */
mfSensor::mfSensor(const char *cname, aPins *_PIN) : sensor(_PIN) {
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
  resume();
  sensor->readMany(); // initialize filter
}
mfSensor::~mfSensor() {
  if (name)
    free(name);
  instances.erase(_me);
}
double mfSensor::_getPercent() {
  double curValue = getValueD();
  if (curValue >= max)
    return 100.0;
  if (curValue <= min)
    return 0.0;
  double range = max - min;
  double percent = range / 100.0;
  return ((curValue - min) / percent);
}
void mfSensor::setMin(double _min) {
  if (_min < 0.0)
    _min = 0.0;
  min = _min;
  if (min > max)
    max = min;
}
void mfSensor::setMin(uint32_t _min) { setMin(1.0 * _min); }
void mfSensor::setMax(double _max) {
  if (_max > 4095.0)
    _max = 4095.0;
  max = _max;
  if (max < min)
    min = max;
}
void mfSensor::setMax(uint32_t _max) { setMax(1.0 * _max); }
double mfSensor::getMinD() { return min; }
double mfSensor::getMaxD() { return max; }
uint32_t mfSensor::getMinI() { return round(min); }
uint32_t mfSensor::getMaxI() { return round(max); }
uint16_t mfSensor::getPercentI() { return round(_getPercent()); }
inline double mfSensor::getPercentD() { return _getPercent(); }
double mfSensor::getVoltage() { return getValueD() * (SENS_VOLTAGE / 4096); }
double mfSensor::getValueD() { return sensor->getFloat(); }
uint32_t mfSensor::getValueI() { return sensor->get(); }