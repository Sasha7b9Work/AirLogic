#ifndef MF_BUS_BUTTON_H
#define MF_BUS_BUTTON_H

#include "mfbus.h"
#include "mfbutton.h"
#include "mfproto.h"
#include <vector>
#include <wiring.h>

class mfBusButton {
public:
  inline static std::vector<mfBusButton *> instances = {};

  char name[32] = "";
  void begin();
  mfButtonFlags getState();
  bool read();
  mfBusButton(const char *_name, mfBus &_bus, uint8_t _device);
  void setDevice(uint8_t _deviceId) { device = _deviceId; }
  ~mfBusButton();

private:
  std::atomic<bool> lock;
  mfBus &bus;
  uint8_t device;
  mfButtonFlags flag = MFBUTTON_NOSIGNAL;
};

#define MFBUSBUTTON(a, ...) mfBusButton(a)(#a, __VA_ARGS__)
#endif
