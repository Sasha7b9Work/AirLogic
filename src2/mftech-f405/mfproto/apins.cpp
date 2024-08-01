#include <wiring.h>

#include "HardwareTimer.h"
#include <vector>

#include "Kalman.h"
#include "apins.h"

uint32_t aPins::timerFreq = 0; // timer frequency in hertz
uint32_t aPins::prevms = 0;    // not used
bool aPins::timerInited = false;
HardwareTimer *aPins::T = nullptr;
// constructor
aPins::aPins(const char *cname, uint16_t _PIN, float measure_estimate,
             float q)
    : pin(_PIN), initMS(measure_estimate), initNoise(q) {
  enableKalman();
  pinMode(_PIN, INPUT); // analog input
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
}
void aPins::enableKalman(void) {
  disableKalman();
  kf = new KalmanFilter(initMS, initMS, initNoise);
}
void aPins::disableKalman(void) {
  if (kf != nullptr) {
    delete kf;
    kf = nullptr;
  }
}
// reads current pin value and puts it to kalman filter
void aPins::read(void) {
  uint32_t readValue = analogRead(pin); // analogRead(pin);
  if (readValue > (1UL << ADC_RESOLUTION))
    return;
  lastValue = readValue;
  if (kf != nullptr) // if we have valid kalman filter instance
    value_f = kf->updateEstimate(lastValue);
  else // just read without approximation
    value_f = (value_f + lastValue) / 2.0;
  value = lrint(value_f);
}
// read pin value <count> times
void aPins::readMany(uint32_t count) {
  while (count--) {
    read();
  }
}

// enable reading this pin
void aPins::stop() { active = false; }

// disable reading this pin
void aPins::start() { active = true; }

// returns approximated value for this pin
uint32_t aPins::get(void) { return (value); }

// returns approximated raw (float) value for this pin
float aPins::getFloat(void) { return (value_f); }

// reinit estimate values for kalman
void aPins::setEstimate(float est) {
  initMS = est;
  if (kf != nullptr) {
    kf->setEstimateError(est);
    kf->setMeasurementError(est);
  }
};

// reinit noise q for kalman
void aPins::setNoise(float q) {
  initNoise = q;
  if (kf != nullptr) {
    kf->setProcessNoise(q);
  }
}

// callback for timer
void aPins::timerCallback(void) {
  for (uint16_t c = 0; c < instances.size(); c++) { // for all inited pins
    if (instances[c]->active) {                     // if this pin is active
      instances[c]->read();                         // read its value
    }
  }
}

// init timer for pins reading
void aPins::initTimer(TIM_TypeDef *timer, uint32_t freq) {
  timerFreq = freq;
  if (timerInited)
    freeTimer();
  T = new HardwareTimer(timer);
  T->pause();
  T->setMode(1, TIMER_OUTPUT_COMPARE);
  T->setOverflow(timerFreq, HERTZ_FORMAT);
  T->attachInterrupt(timerCallback);
  timerInited = true;
  T->resume();
}

// free timer and stop using it
void aPins::freeTimer(void) {
  T->pause();
  delete T;
  timerInited = false;
  timerFreq = 0;
}

// destructor
aPins::~aPins() {
  if (name != nullptr) {
    free(name);
  }
  disableKalman();
  instances.erase(_me);
  if (instances.empty() && timerInited) {
    freeTimer(); // normally it should never happen cuz all pins have to be
                 // statically defined, but if you want to malloc apins
                 // instances, it will free timer after last pin deletion
  }
}
