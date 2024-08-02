#include "mfbutton.h"
//#include "mfproto.h"
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
  strncpy(name, _name, 31);
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
  instances.push_back(this);
}
mfButton::~mfButton() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}
void mfButton::begin() {
#if (MFBUTTON_USE_EXTI == 1)
  if (!READ_BIT(extimap, bit(pin))) {
    SET_BIT(extimap, bit(pin));
    attachInterrupt(pinHW, irqHandler, CHANGE);
  }
#endif
}

void mfButton::read() {
#if (MFBUTTON_USE_EXTI != 1)
  poll(false);
#endif
  uint8_t cur = curState.val;
  uint8_t prev = prevState.val;
  if (cur != prev) {
    if (micros() - curState.time > debounceTime) {
      if ((cur == 1) && (prev == 0)) {
        setState(MFBUTTON_PRESSED);
        pressed = true;
        longFlag = false;
      } else {
        if (pressed) {
          if (!longFlag) {
            setState(MFBUTTON_RELEASED);
          } else {
            setState(MFBUTTON_RELEASED_LONG);
            longFlag = false;
          }
        }
        pressed = false;
      }
      prevState.val = cur;
    }
  } else {
    // if (pressed)
    if ((!longFlag) && (cur == 1) &&
        ((micros() - curState.time) / 1000) >= MFBUTTON_LONG_TIME) {
      longFlag = true;
      setState(MFBUTTON_PRESSED_LONG);
    }
  }
}

mfButtonFlags mfButton::getState() {
  mfButtonFlags retFlag;
  read();
  if (flag.empty())
    retFlag = MFBUTTON_NOSIGNAL;
  else {
    retFlag = flag.at(0);
    flag.erase(flag.begin());
  }
  return retFlag;
}

void mfButton::setState(mfButtonFlags newflag) { flag.push_back(newflag); }

void mfButton::poll(bool force) {
  bool read = readPin();
  uint8_t curValue =
      (READ_BIT(mode, bit(0)) != 0) ? (uint8_t)read : (uint8_t)!read;
  if (force || (curState.val != curValue)) {
    prevState = curState;
    curState = {curValue, micros()};
  }
}

inline bool mfButton::readPin(void) {
  return (((port->IDR) & (1U << pin)) != 0);
}
