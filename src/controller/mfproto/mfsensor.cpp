#include "mfsensor.h"
/* */
mfSensor::mfSensor(const char *cname, aPins *_PIN, float _LOW, float _HIGH)
    : sensor(_PIN) {
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
  sensor->disableKalman();
  setRange(_LOW, _HIGH);
  resume();
  sensor->enableKalman();
  sensor->readMany(); // initialize filter
}
mfSensor::~mfSensor() {
  if (name)
    free(name);
  instances.erase(_me);
}
inline float mfSensor::_getPercent() {
  uint32_t read = getValueI();
  if (range >= 0)
    return ((read - low) / percent);
  else
    return 100 - ((read - high) / percent);
}

void mfSensor::setRange(float _low, float _high) {
  if (_low < 0)
    _low = 0;
  if (_low > MAX_ADC)
    _low = MAX_ADC;
  if (_high < 0)
    _high = 0;
  if (_high > MAX_ADC)
    _high = MAX_ADC;

  low = _low;
  high = _high;
  range = high - low;
  percent = std::abs(range / 100.0);
  sensor->setEstimate(std::abs(range));
}
void mfSensor::setRangeV(float _low, float _high) {
  setRange(_low / vMult, _high / vMult);
}

float mfSensor::getLowD() { return low; }
float mfSensor::getHighD() { return high; }
uint32_t mfSensor::getLowI() { return lrint(low); }
uint32_t mfSensor::getHighI() { return lrint(high); }
int32_t mfSensor::getPercentI() {
  float newPercent = _getPercent();
  if (::fabsf(newPercent - lastPercent) > 0.5)
    lastPercent = newPercent;
  return lrint(lastPercent);
}
float mfSensor::getPercentD() { return _getPercent(); }
float mfSensor::getVoltage() { return getValueI() * vMult; }
float mfSensor::getVoltageA() {
  uint32_t newVoltage = lrint(getValueI() * vMult * 1000);
  if (::llabs(newVoltage - lastVoltage) > 10)
    lastVoltage = newVoltage;
  return lastVoltage / 1000.0;
}
float mfSensor::getValueD() { return sensor->getFloat(); }
uint32_t mfSensor::getValueI() { return sensor->get(); }