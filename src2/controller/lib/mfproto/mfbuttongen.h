#ifndef MF_BUTTON_GEN_H
#define MF_BUTTON_GEN_H

#include <vector>
#include <wiring.h>

class mfBusButtonGen {
public:
  inline static std::vector<mfBusButton *> instances = {};

  char name[32] = "";
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
