#include "mfsensor.h"
/* */
mfSensor::mfSensor(const char *cname, aPins *_PIN, float _LOW, float _HIGH)
    : sensor(_PIN) {
  strncpy(name, cname, 31);
  instances.push_back(this);
  sensor->disableKalman();
  setRange(_LOW, _HIGH);
  resume();
  sensor->enableKalman();
  sensor->readMany(); // initialize filter
  vFilt = new KalmanFilter(4095, 4095, 1);
}
mfSensor::~mfSensor() {
  if (vFilt)
    delete vFilt;
  instances.erase(std::find(instances.begin(), instances.end(), this));
}
inline float mfSensor::__getPercent(float value) {
  if (range >= 0)
    return ((value - low) / percent);
  else
    return 100 - ((value - high) / percent);
}
inline float mfSensor::_getPercent() { return __getPercent(getValueD()); }
void mfSensor::setRangeForced(float _low, float _high) {
  low = _low;
  high = _high;
  range = high - low;
  percent = std::abs(range / 100.0);
  sensor->setEstimate(::fabsf(range));
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
  setRangeForced(_low, _high);
}
void mfSensor::setRangeV(float _low, float _high) {
  setRange(_low / vMult, _high / vMult);
}

float mfSensor::getLowD() { return low; }
float mfSensor::getHighD() { return high; }
uint32_t mfSensor::getLowI() { return lrint(low); }
uint32_t mfSensor::getHighI() { return lrint(high); }
int32_t mfSensor::getPercentI() {
  return lrint(__getPercent(getVoltageA() / vMult));
}
float mfSensor::getPercentD() { return _getPercent(); }
float mfSensor::getVoltage() { return getValueD() * vMult; }
float mfSensor::getVoltageA() {
  float newVoltage = getValueI();
  if (vFilt)
    newVoltage = vFilt->updateEstimate(newVoltage);

  if (::fabsf(newVoltage - lastVoltage) > 0.01)
    lastVoltage = newVoltage;
  return lastVoltage * vMult;
}
float mfSensor::getValueD() { return sensor->getFloat(); }
uint32_t mfSensor::getValueI() { return sensor->get(); }