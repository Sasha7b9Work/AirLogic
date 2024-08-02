#include <wiring.h>

#include "stm32_def.h"
#include "stm32yyxx_ll_gpio.h"

#include <vector>

#include <HardwareTimer.h>

#include "bpins.h"

uint64_t bPins::timerCounter = 0;
uint32_t bPins::timerFreq = 1;
uint32_t bPins::prevms = 0;
HardwareTimer *bPins::T = nullptr;
bool bPins::timerInited = false;

bPins::bPins(const char *cname, uint16_t _PIN, uint8_t ch, uint8_t hs)
    : port(get_GPIO_Port(STM_PORT(digitalPinToPinName(_PIN)))),
      pin(STM_GPIO_PIN(digitalPinToPinName(_PIN))), pinHW(_PIN), channel(ch),
      highState(hs & 1U), lowState(!(hs & 1U)) {
  uint8_t isOD = hs & 2U;
  uint32_t ll_pin = STM_LL_GPIO_PIN(digitalPinToPinName(pinHW));
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(pinHW)));
  LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(
      port, ll_pin, isOD ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL);
#ifndef STM32F1xx
  LL_GPIO_SetPinPull(port, ll_pin, LL_GPIO_PULL_NO);
#endif
  LL_GPIO_SetPinSpeed(port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
  reset();
  strcpy(name, cname);
  instances.push_back(this);
}
__attribute__((optimize(3))) inline void bPins::write(uint8_t value) {
  port->BSRR = (pin) << (!(value & 1U) << 4);
}
// only to read output ports, input ports should be read from IDR
__attribute__((optimize(3))) inline uint8_t bPins::read(void) {
  return port->ODR & (pin);
}
__attribute__((optimize(3))) inline void bPins::reset(void) { write(lowState); }
__attribute__((optimize(3))) inline void bPins::set(void) { write(highState); }
__attribute__((optimize(3))) inline void bPins::toggle(void) {
  port->ODR ^= (pin);
}
bool bPins::isActive(void) { return (onTime == 0); }
uint32_t bPins::getRemaining(void) { return onTime; }
uint32_t bPins::getTime(void) { return onInitTime; }
void bPins::setTime(uint32_t time) {
  // bool timerPaused = false;
  if (timerInited) { // threadsafe, don't bother onTime till we changing
    // it here
    // T->pause();
    // timerPaused = true;
  }
  if (!onTime) { // if pin finished any job already, just init it as NORMAL
    // and reset
    onInitTime = onTime = time;
  } else {
    switch (mode) {
    default:
    case BPIN_NORMAL:
      onTime = onInitTime = time;
      break;
    case BPIN_ADDITIVE:          // simply add some extra time to it
      if (onTime + time <= ~0UL) // check for overflow
        onTime += time;
      else
        onTime = ~0UL;
      if (onInitTime + time <= ~0UL) // same check here
        onInitTime += time;
      else
        onInitTime = ~0UL;
      break;
    }
  }
  // if (timerPaused) // switch timer on back
  // T->resume();
}
void bPins::noInterrupt(void) { noInterruptable = true; }
void bPins::setMode(uint8_t newMode) {
  if (newMode < BPIN_MODEEND)
    mode = newMode;
}

void bPins::setPWM(uint16_t duty) {
  if (duty <= 100) {
    Duty = duty;
    T->setCaptureCompare(channel, Duty, PERCENT_COMPARE_FORMAT);
  }
}
void bPins::updateFrequency() {
  T->setCaptureCompare(channel, Duty, PERCENT_COMPARE_FORMAT);
}
void bPins::updateChannel(uint8_t ch) {
  if ((ch >= 1) && (ch <= 4)) {
    channel = ch;
    T->setCaptureCompare(channel, Duty, PERCENT_COMPARE_FORMAT);
  }
}

void bPins::on(uint32_t time) {
  if (noInterruptable && onTime)
    return;
  else
    noInterruptable = false;
  setTime(time);
}
void bPins::off() {
  if (noInterruptable && onTime)
    return;
  else
    noInterruptable = false;
  finishCallback(this); // call callback at the end
  onTime = 0;
  onInitTime = 0;
  reset();
}
uint16_t bPins::getDuty(void) { return (Duty); }

void bPins::initTimer(TIM_TypeDef *timer, uint32_t freq) {
  timerFreq = freq;
  if (timerInited) {
    freeTimer();
  }

  T = new HardwareTimer(timer);
  T->pause();
  T->setOverflow(timerFreq, HERTZ_FORMAT);
  T->attachInterrupt([]() {
    uint32_t ms = (((timerCounter * 1000)) / (timerFreq));
    for (uint16_t i = 0; i < bPins::instances.size(); i++) {
      bPins &cur = *bPins::instances[i];
      if (!cur.onTime) {
        cur.reset();
        continue;
      }
      if (ms > prevms) {
        if (cur.onTime > ms - prevms) {
          cur.onTime -= (ms - prevms);
        } else {
          if (cur.onTime)
            cur.finishCallback(bPins::instances[i]); // call callback at the end
          cur.onTime = 0;
          cur.reset();
          continue;
        }
        cur.tickCallback(bPins::instances[i]);
      }
      if ((cur.onTime) && (cur.getDuty())) {
        cur.set();
      }
    }
    if (ms > prevms)
      prevms = ms;
    timerCounter++;
  });
  for (uint8_t i = 1; i <= 4; i++) {
    T->setMode(i, TIMER_OUTPUT_COMPARE);
    T->setCaptureCompare((uint32_t)i, 100, PERCENT_COMPARE_FORMAT);
    T->attachInterrupt((uint32_t)i, [i]() {
      for (uint16_t z = 0; z < instances.size(); z++) {
        bPins &cur = *bPins::instances[z];
        if ((!cur.onTime) || (cur.channel != i))
          continue;
        if (cur.getDuty() != 100) {
          cur.reset();
        }
      }
    });
  }
  timerInited = true;
  T->resume();
}
void bPins::freeTimer(void) {
  T->pause();
  T->detachInterrupt();
  for (uint16_t i = 0; i < 4; i++) {
    T->detachInterrupt(i);
  }
  delete T;
  timerInited = false;
  timerFreq = 0;
}
void bPins::setFrequency(uint32_t freq) {
  if (!timerInited)
    return;
  T->pause();
  T->setOverflow(freq, HERTZ_FORMAT);
  for (uint16_t z = 0; z < instances.size(); z++) {
    bPins &cur = *bPins::instances[z];
    cur.updateFrequency();
  }
  timerFreq = freq;
  T->resume();
}

bPins::~bPins() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
  if (instances.empty() && timerInited)
    freeTimer(); // normally it should never happen cuz all pins have to be
  // statically defined, but if you want to malloc cpins
  // instances, it will free timer after last pin deletion
}