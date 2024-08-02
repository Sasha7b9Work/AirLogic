#include "mfvalve.h"
/**/
mfValve::mfValve(const char *cname, bPins *_PIN) : valve(_PIN) {
  strncpy(name, cname, 31);
  instances.push_back(this);
  if (valve) {
    // valve->setMode(CPIN_CONTINUE);
    valve->off();
    valve->tickCallback =
        std::bind(&mfValve::tickCallback, this, std::placeholders::_1);
  }
}
void mfValve::tickCallback(bPins *pin) { /*isLocked();*/
}

mfValve::~mfValve() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}

void mfValve::setDuty(uint16_t duty) {
  if (valve)
    valve->setPWM(duty);
}
void mfValve::on(uint32_t time, std::function<void(bPins *pin)> callback) {
  if (valve && !locked) {
    valve->finishCallback = callback;
    if (common) {
      if (!entries)
        valve->on(time);
      entries++;
    } else
      valve->on(time);
  }
}
void mfValve::off(void) {
  if (valve) {
    if (common) {
      if (entries > 0)
        entries--;
      if (!entries)
        valve->off();
    } else
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
  if (common) {
    lockentries++;
    if (lockentries > 1)
      return;
  }
  locked = true;
  lockMoment = HAL_GetTick();
  lockTime = time;
  lockCallback = callback;
}
void mfValve::unlock(void) {
  if (locked) {
    if (common) {
      if (lockentries > 0)
        lockentries--;
      if (lockentries)
        return;
    }
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