#ifndef MF_BUTTON_GEN_H
#define MF_BUTTON_GEN_H

#include <vector>
#include <wiring.h>

class mfBusButtonGen {
public:
#undef __inst__
#define __inst__ std::vector<mfBusButton *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;

  char *name = (char *)NULL;
  virtual void begin();
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
