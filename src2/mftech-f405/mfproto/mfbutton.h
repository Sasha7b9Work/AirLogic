#ifndef MF_BUTTON_H
#define MF_BUTTON_H

#include <wiring.h>

#include <vector>

typedef struct {
  __IO uint8_t val;
  __IO uint32_t time;
} MFBUTTON_STATE;

#define MFBUTTON_STATES 2U

enum mfButtonFlags {
  MFBUTTON_NOSIGNAL = 0,
  MFBUTTON_PRESSED,
  MFBUTTON_PRESSED_LONG,
  MFBUTTON_RELEASED,
  MFBUTTON_RELEASED_LONG
};

#ifndef MFBUTTON_LONG_TIME
#define MFBUTTON_LONG_TIME 500
#endif

#ifndef MFBUTTON_USE_EXTI
#define MFBUTTON_USE_EXTI 1
#endif

class mfButton {
public:
#undef __inst__
#define __inst__ std::vector<mfButton *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)NULL; // pin name, filled by CPIN define, can be used

  void begin();
  mfButtonFlags getState();
  void setState(mfButtonFlags newflag);
  void read();
  uint32_t lastChange();
  void poll(bool force = false);
#if (MFBUTTON_USE_EXTI == 1)
#endif
  mfButton(const char *_name, uint16_t pin, uint8_t _mode = INPUT);
  ~mfButton();

private:
  GPIO_TypeDef *port;
  uint16_t pin;
#if (MFBUTTON_USE_EXTI == 1)
  static uint16_t extimap;
#endif
  std::vector<MFBUTTON_STATE> state; // button state buffer
  uint16_t pinHW;                    // pin number connected to button
  uint8_t mode;                      // input mode
  bool longFlag = false;
  mfButtonFlags flag = MFBUTTON_NOSIGNAL;
  bool readPin(void); // read pin value
  callback_function_t irqHandler = [this]() { this->poll(true); };
};

#define MFBUTTON(a, ...) mfButton a(#a, __VA_ARGS__);
#endif
