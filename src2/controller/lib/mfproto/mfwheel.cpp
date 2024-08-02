#include "debug.h"
extern DebugSerial debug;
#include "mfwheel.h"

mfWheel::mfWheel(const char *cname, mfWheelConfig _config) : config(_config) {
  strncpy(name, cname, 31);
  instances.push_back(this);

  if (!config.receiverPresent)   // in case we don't have receiver...
    config.commonExhaust = true; // ...common exhaust have to be used
  // but it's possible to have receiver and have common exhaust also (wabco?)
  if (config.receiver) {
    config.receiver->setCommon();
  }
}
mfWheel::~mfWheel() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}

void mfWheel::upOn() {
  moveStartTime = TICKS();
  moveRange = 0;
  if (config.sensor) {
    startPoint.position = config.sensor->getVoltage();
    startPoint.time = moveStartTime;
    startPoint.actual = true;
    startPoint.direction = MFWHEEL_UP;
    max.time = moveStartTime;
    max.delta = 0;
    min.time = moveStartTime;
    min.delta = 0;
    speedPrevPoint = startPoint;
    speedPrevPoint.position = config.sensor->getVoltage();
    lastSpeed = 0;
  }
  if (config.up)
    if (config.up->isLocked())
      return;
  if (config.comp) {
    if (!((config.receiverPresent) && (config.receiver))) {
      if (config.comp->isCompressorLocked()) {
        // debug.printf("[%s] comp locked\n", name);
        return;
      }
    } else {
      if (config.receiver)
        if (config.receiver->isLocked()) {
          // debug.printf("[%s] recv locked\n", name);
          return;
        }
    }
  }
  upState.on = true;
  upState.time = TICKS();

  if (config.commonExhaust && config.comp)
    config.comp->lockDryer();

  if (config.up) {
    config.up->on(MF_MAX_VALVE_TIME, [&](bPins *pin) {
      upState.overloaded = true;
      // debug.printf("%s up overloaded.\n", name);
    });
    // debug.printf("%s start up channel.\n", name);
  }

  if ((config.receiverPresent) && (config.receiver)) {

    config.receiver->on();
  } else {
    if (config.comp && !config.wabco8ch)
      config.comp->startCompressor();
  }
}
void mfWheel::upOff() {
  startPoint.actual = false;
  if (config.up) {
    config.up->valve->finishCallback = [&](bPins *pin) {
      upState.overloaded = false;
      // debug.printf("%s up finish callback.\n", name);
    };
    config.up->off();
    // debug.printf("%s finish up channel.\n", name);
  }

  if ((config.receiverPresent) && (config.receiver)) {
    config.receiver->off();
  } else {
    if (config.comp && !config.wabco8ch)
      config.comp->stopCompressor();
  }

  if (config.commonExhaust && config.comp)
    config.comp->unlockDryer();

  upState.on = false;
  moveEndTime = TICKS();
  if (config.sensor) {
    float endPosition = config.sensor->getVoltage();
    moveRange = startPoint.position - endPosition;
  }
}

void mfWheel::downOn() {
  moveStartTime = TICKS();
  moveRange = 0;
  if (config.sensor) {
    startPoint.position = config.sensor->getVoltage();
    startPoint.time = moveStartTime;
    startPoint.actual = true;
    startPoint.direction = MFWHEEL_DOWN;
    max.time = moveStartTime;
    max.delta = 0;
    min.time = moveStartTime;
    min.delta = 0;
    speedPrevPoint = startPoint;
    speedPrevPoint.position = config.sensor->getVoltage();
    lastSpeed = 0;
  }
  if (config.down)
    if (config.down->isLocked())
      return;
  if (config.comp)
    if (config.commonExhaust) {
      if (config.comp->isDryerLocked())
        return;
    }
  downState.on = true;
  downState.time = TICKS();

  if (config.down) {
    config.down->on(MF_MAX_VALVE_TIME, [&](bPins *pin) {
      downState.overloaded = true;
      // debug.printf("%s down overloaded.\n", name);
    });
    // debug.printf("%s start down channel.\n", name);
  }

  if (config.comp) {
    config.comp->lockCompressor();
    if (config.receiver) {
      config.receiver->off();
      config.receiver->lock();
    }

    if (config.commonExhaust)
      config.comp->startDryer();
  }
}
void mfWheel::downOff() {
  startPoint.actual = false;
  if (config.down) {
    config.down->valve->finishCallback = [&](bPins *pin) {
      downState.overloaded = false;
      // debug.printf("%s finish down callback.\n", name);
    };
    config.down->off();
    // debug.printf("%s finish down channel.\n", name);
  }

  if (config.comp) {
    if (config.commonExhaust)
      config.comp->stopDryer();

    if (config.receiver) {
      if (config.receiver->isLocked())
        config.receiver->unlock();
    }

    config.comp->unlockCompressor();
  }
  downState.on = false;
  moveEndTime = TICKS();
  if (config.sensor) {
    float endPosition = config.sensor->getVoltage();
    moveRange = startPoint.position - endPosition;
  }
}
float mfWheel::getLastMoveRange() { return moveRange; }
uint32_t mfWheel::getLastMoveStartTime() { return moveStartTime; }
uint32_t mfWheel::getLastMoveEndTime() { return moveStartTime; }
bool mfWheel::isUpOn() { return upState.on; }
bool mfWheel::isDownOn() { return downState.on; }
uint32_t mfWheel::upOnTime() {
  if (isUpOn())
    return TICKS() - upState.time;
  else
    return 0UL; // its off
}
uint32_t mfWheel::downOnTime() {
  if (isDownOn())
    return TICKS() - downState.time;
  else
    return 0UL; // its off
}
float mfWheel::getCurrentSpeed() {
  if (!config.sensor)
    return 0.0;
  uint32_t curtime = TICKS();
  if (curtime - speedPrevPoint.time >= speedStep) {
    speedLastPoint.position = config.sensor->getVoltage();
    speedLastPoint.time = curtime;
    lastSpeed = 1000 * (speedLastPoint.position - speedPrevPoint.position) /
                (speedLastPoint.time - speedPrevPoint.time);
    speedPrevPoint = speedLastPoint;
    if (abs(lastSpeed) < 0.01)
      stuckCounter++;
    else
      stuckCounter = 0;
  }
  return lastSpeed;
}
bool mfWheel::isStucked() {
  if (!startPoint.actual) {
    stuckCounter = 0;
    return false;
  }
  if (TICKS() - startPoint.time < 2000) {
    stuckCounter = 0;
    return false;
  }
  if (stuckCounter > 3)
    return true;
  else
    return false;
}
bool mfWheel::isStartedMoving() {
  if (!startPoint.actual) {
    max.time = TICKS();
    max.delta = 0;
    min.time = TICKS();
    min.delta = 0;
    return false;
  }
  mfWheelPoint current;
  current.position = config.sensor->getVoltage();
  current.time = TICKS();
  float diff = (startPoint.direction == MFWHEEL_UP)
                   ? (current.position - startPoint.position)
                   : (startPoint.position - current.position);
  if (diff > max.delta) {
    max.delta = diff;
    max.time = TICKS();
    // debug.printf("%s: a(%.4f-%.4f)\n", name, min.delta, max.delta);
  }
  if (diff < min.delta) {
    min.delta = diff;
    min.time = TICKS();
    // debug.printf("%s: i(%.4f-%.4f)\n", name, min.delta, max.delta);
  }
  mfWheelPoint middle;
  middle.time = TICKS();
  middle.position = startPoint.position + (min.delta + max.delta) / 2;
  if (fabsf(middle.position - startPoint.position) >= 0.5)
    return true;
  else
    return false;
}
bool mfWheel::isStoppedMoving() {
  if (!startPoint.actual)
    return false;
  if (!isStartedMoving())
    return false;

  return false; // fixme
  if ((TICKS() - min.time > 2000) && (TICKS() - max.time > 2000))
    return true;
  else
    return false;
}