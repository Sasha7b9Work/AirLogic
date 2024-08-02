#include "mfvalve.h"
/**/
mfValve::mfValve(const char *cname, cPins *_PIN) : valve(_PIN) {
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
  if (valve) {
    valve->setMode(CPIN_CONTINUE);
    valve->off();
    valve->tickCallback =
        std::bind(&mfValve::tickCallback, this, std::placeholders::_1);
  }
}
void mfValve::tickCallback(cPins *pin) { /*isLocked();*/
}

mfValve::~mfValve() {
  if (name)
    free(name);
  instances.erase(_me);
}

void mfValve::setDuty(uint16_t duty) {
  if (valve)
    valve->setPWM(duty);
}
void mfValve::on(uint32_t time, std::function<void(cPins *pin)> callback) {
  if (valve && !locked) {
    valve->finishCallback = callback;
    valve->on(time);
  }
}
void mfValve::off(void) {
  if (valve && !locked) {
    valve->off();
  }
}
bool mfValve::isActive(void) {
  if (valve)
    return valve->isActive();
  else
    return false;
}
void mfValve::lock(uint32_t time, std::function<void()> callback) {
  locked = true;
  lockMoment = HAL_GetTick();
  lockTime = time;
  lockCallback = callback;
}
void mfValve::unlock(void) {
  if (locked) {
    lockCallback();
    lockCallback = []() {};
  }
  locked = false;
}
bool mfValve::isLocked(void) {
  if (locked && ((HAL_GetTick() - lockMoment) >= lockTime)) {
    unlock();
  }
  return locked;
}