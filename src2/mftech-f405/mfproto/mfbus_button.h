#ifndef MF_BUS_BUTTON_H
#define MF_BUS_BUTTON_H

#include "mfbus.h"
#include "mfbutton.h"
#include "mfproto.h"
#include <vector>
#include <wiring.h>

class mfBusButton {
public:
#undef __inst__
#define __inst__ std::vector<mfBusButton *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;

  char *name = (char *)NULL;
  void begin();
  mfButtonFlags getState();
  void read();
  mfBusButton(const char *_name, mfBus &_bus, uint8_t _device);
  ~mfBusButton();

private:
  mfBus &bus;
  uint8_t device;
  mfButtonFlags flag = MFBUTTON_NOSIGNAL;
};

#define MFBUSBUTTON(a, ...) mfBusButton(a)(#a, __VA_ARGS__)
#endif
