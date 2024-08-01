#ifndef MF_BUTTON_H
#define MF_BUTTON_H

#include <wiring.h>

#include <vector>

typedef struct {
  __IO uint8_t val;
  __IO uint32_t time;
} MFBUTTON_STATE;

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
  inline static std::vector<mfButton *> instances = {};
  char name[32] = ""; // pin name, filled by CPIN define, can be used

  void begin();
  mfButtonFlags getState();
  void setState(mfButtonFlags newflag);
  void read();
  void poll(bool force = false);
  mfButton(const char *_name, uint16_t pin, uint8_t _mode = INPUT);
  ~mfButton();
  callback_function_t irqHandler = [this]() { this->poll(true); };

private:
  GPIO_TypeDef *port;
  uint16_t pin;
#if (MFBUTTON_USE_EXTI == 1)
  static uint16_t extimap;
#endif
  MFBUTTON_STATE prevState = {0, micros()}; // last button state
  MFBUTTON_STATE curState = {0, micros()};  // current button state
  uint16_t pinHW;                           // pin number connected to button
  uint8_t mode;                             // input mode
  bool longFlag = false;
  std::vector<mfButtonFlags> flag;
  bool readPin(void); // read pin value
  uint32_t debounceTime = 2000;
  bool pressed = false;
};

#define MFBUTTON(a, ...) mfButton a(#a, __VA_ARGS__);
#endif
