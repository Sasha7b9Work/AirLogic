#include "mfwheel.h"

mfWheel::mfWheel(const char *cname, mfWheelConfig _config) : config(_config) {
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();

  if (!config.receiverPresent)   // in case we don't have receiver...
    config.commonExhaust = true; // ...common exhaust have to be used
  // but it's possible to have receiver and have common exhaust also (wabco?)
}
mfWheel::~mfWheel() {
  if (name)
    free(name);
  instances.erase(_me);
}

void mfWheel::upOn() {
  upState.on = true;
  upState.time = TICKS();

  if (config.commonExhaust && config.comp)
    config.comp->lockDryer();

  if (config.up)
    config.up->on();

  if (config.receiverPresent) {
    if (config.receiver)
      config.receiver->on();
  } else {
    if (config.comp)
      config.comp->startCompressor();
  }
}
void mfWheel::upOff() {
  if (config.up)
    config.up->off();

  if (config.receiverPresent) {
    if (config.receiver)
      config.receiver->off();
  } else {
    if (config.comp)
      config.comp->stopCompressor();
  }

  if (config.commonExhaust && config.comp)
    config.comp->unlockDryer();

  upState.on = false;
}

void mfWheel::downOn() {
  downState.on = true;
  downState.time = TICKS();

  if (config.down)
    config.down->on();

  if (config.comp) {
    config.comp->lockCompressor();

    if (config.commonExhaust)
      config.comp->startDryer();
  }
}
void mfWheel::downOff() {

  if (config.down)
    config.down->off();

  if (config.comp) {
    if (config.commonExhaust)
      config.comp->stopDryer();

    config.comp->unlockCompressor();
  }
  downState.on = false;
}

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