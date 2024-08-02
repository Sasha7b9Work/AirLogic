#include "mfbutton.h"
#include "mfproto.h"
#include "stm32_def.h"
#include "stm32yyxx_ll_gpio.h"
#include <functional>
#include <pins_stm.h>
#include <vector>
#include <wiring.h>

#if (MFBUTTON_USE_EXTI == 1)
uint16_t mfButton::extimap = 0U;
#endif

mfButton::mfButton(const char *_name, uint16_t pin, uint8_t _mode)
    : port(get_GPIO_Port(STM_PORT(digitalPinToPinName(pin)))),
      pin(STM_PIN(digitalPinToPinName(pin))), pinHW(pin), mode(_mode) {
  name = (char *)malloc(strlen(_name) + 1);
  if (name != NULL) {
    strcpy(name, _name);
  }
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(pinHW)));
  uint32_t ll_pin = STM_LL_GPIO_PIN(digitalPinToPinName(pin));
  LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_INPUT);
  uint32_t __mode;
  switch (mode) {
  case INPUT_PULLDOWN:
    __mode = LL_GPIO_PULL_DOWN;
    break;
  case INPUT_PULLUP:
    __mode = LL_GPIO_PULL_UP;
    break;
  default:
  case INPUT:
#ifndef STM32F1xx
    __mode = LL_GPIO_PULL_NO;
#else
    (void)__mode;
#endif
    break;
  };
#ifndef STM32F1xx
  LL_GPIO_SetPinPull(port, ll_pin, __mode);
#endif
  LL_GPIO_SetPinSpeed(port, ll_pin, LL_GPIO_SPEED_FREQ_LOW);
  // pinMode(pinHW, mode);
  state.reserve(MFBUTTON_STATES);
  instances.push_back(this);
  _me = instances.end();
}
mfButton::~mfButton() {
  instances.erase(_me);
  if (name)
    free(name);
}
void mfButton::begin() {
  state.clear();
  state.push_back({0, TICKS()}); // initial state
#if (MFBUTTON_USE_EXTI == 1)
  if (!READ_BIT(extimap, bit(pin))) {
    SET_BIT(extimap, bit(pin));
    attachInterrupt(pinHW, irqHandler, CHANGE);
  } else {
#if defined(debug)
    debug.printf("%s: FAILED, pin %u, extimap %u\n", name, pin, extimap);
#endif
    // unfortunately we can't have multiple pins attached to one EXTI port
  }
#endif
}

void mfButton::read() {
  MFBUTTON_STATE first = state.front();
  if (state.size() > 1) {
    MFBUTTON_STATE last = state.back();
    if ((first.val == 0) && (last.val == 1)) {
      setState(MFBUTTON_PRESSED);
    }
    if ((first.val == 1) && (last.val == 0)) {
      if (!longFlag) {
        setState(MFBUTTON_RELEASED);
      } else {
        setState(MFBUTTON_RELEASED_LONG);
      }
      longFlag = false;
    }
    state.erase(state.begin());
  } else {
    if ((!longFlag) && (first.val == 1) &&
        (TICKS() - first.time >= MFBUTTON_LONG_TIME)) {
      longFlag = true;
      setState(MFBUTTON_PRESSED_LONG);
    }
  }
}

mfButtonFlags mfButton::getState() {
  read();
  mfButtonFlags retFlag = flag;
  flag = MFBUTTON_NOSIGNAL;
  return retFlag;
}

void mfButton::setState(mfButtonFlags newflag) { flag = newflag; }

void mfButton::poll(bool force) {
  bool read = readPin();
  uint8_t curValue =
      (READ_BIT(mode, bit(0)) != 0) ? (uint8_t)read : (uint8_t)!read;
  uint32_t curTime = TICKS();
  MFBUTTON_STATE curState = state.front();
  if (force || (curValue == curState.val)) {
    state.clear();
  } else {
    if (state.size() > 1) {
      state.pop_back();
    }
  }
  state.push_back({curValue, curTime});
}

bool mfButton::readPin(void) { return (((port->IDR) & (1U << pin)) != 0); }
