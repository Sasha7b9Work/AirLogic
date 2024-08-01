#ifndef MFBUS_RGB_H
#define MFBUS_RGB_H

#include "mfbus.h"
#include "mfproto.h"
#include <vector>

enum mfRgbSignals {
  MFBUS_RGB_ONCMD = MFBUS_RGB_RESERVED,
  MFBUS_RGB_OFFCMD,
  MFBUS_RGB_BLINKCMD,
  MFBUS_RGB_BLINKFADECMD,
  MFBUS_RGB_BREATHECMD,
  MFBUS_RGB_SETBRIGHTNESSCMD,
  MFBUS_RGB_SETCOLORCMD
};

typedef struct {
  uint32_t time;
} MFBUS_RGB_ON;

typedef struct {
  uint32_t time, period, phase;
} MFBUS_RGB_BLINK;

typedef struct {
  uint8_t brightness;
} MFBUS_RGB_BRIGHTNESS;

typedef struct {
  uint8_t r, g, b;
} MFBUS_RGB_SETCOLOR;
class mfBusRGB {
private:
  mfBus &bus;
  uint8_t device;

public:
  char name[32] = "";
  inline static std::vector<mfBusRGB *> instances = {};

  mfBusRGB(const char *_name, mfBus &_bus, uint8_t _deviceid);
  ~mfBusRGB();
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  void setColor(uint32_t color);
  void setDevice(uint8_t _deviceId) { device = _deviceId; }
  uint8_t setBrightness(uint8_t brightness);
  void blink(uint32_t time, uint32_t period, uint32_t phase = 0);
  void blinkfade(uint32_t time, uint32_t period, uint32_t phase = 0);
  void breathe(uint32_t time, uint32_t period, uint32_t phase = 0);
  void on(uint32_t time = ~0UL);
  void off();
};
#define MFBUSRGB(a, ...) mfBusRGB(a)(#a, __VA_ARGS__)

#endif