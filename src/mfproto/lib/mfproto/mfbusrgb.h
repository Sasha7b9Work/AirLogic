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
  uint32_t time, period;
} MFBUS_RGB_BLINK;

#define MFBUS_RGB_BLINKFADE MFBUS_RGB_BLINK
#define MFBUS_RGB_BREATHE MFBUS_RGB_BLINK

typedef struct {
  uint8_t brightness;
} MFBUS_RGB_BRIGHTNESS;

typedef struct {
  uint8_t r, g, b;
} MFBUS_RGB_SETCOLOR;
#define __MAX_TEMP_SIZE (MFPROTO_MAXDATA)
#define __PACK_NAME_WITH_CMD(a)                                                \
  uint8_t tempBuffer[__MAX_TEMP_SIZE];                                         \
  for (uint16_t _i = 0; _i < sizeof((a)); _i++)                                \
    tempBuffer[_i] = ((uint8_t *)&(a))[_i];                                    \
  for (uint16_t _i = 0; _i <= strlen(name); _i++)                              \
    tempBuffer[_i + sizeof((a))] = name[_i];                                   \
  uint16_t tempBufferSize = sizeof((a)) + strlen(name) + 1;
class mfBusRGB {
private:
  std::vector<mfBusRGB *>::iterator _me;
  mfBus &bus;
  uint8_t device;

public:
  char *name = nullptr;
  inline static std::vector<mfBusRGB *> instances = {};

  mfBusRGB(const char *_name, mfBus &_bus, uint8_t _deviceid)
      : bus(_bus), device(_deviceid) {

    instances.push_back(this);
    _me = instances.end();
    name = (char *)malloc(strlen(_name) + 1);
    if (name)
      strcpy(name, _name);
  }
  ~mfBusRGB() {
    if (name)
      free(name);
    instances.erase(_me);
  }
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    MFBUS_RGB_SETCOLOR cmd = {.r = r, .g = g, .b = b};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_SETCOLORCMD, tempBuffer,
                                      tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void setBrightness(uint8_t brightness) {
    MFBUS_RGB_BRIGHTNESS cmd = {.brightness = brightness};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_SETBRIGHTNESSCMD,
                                      tempBuffer, tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void blink(uint32_t time, uint32_t period) {
    MFBUS_RGB_BLINK cmd = {.time = time, .period = period};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BLINKCMD, tempBuffer,
                                      tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void blinkfade(uint32_t time, uint32_t period) {
    MFBUS_RGB_BLINK cmd = {.time = time, .period = period};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BLINKFADECMD,
                                      tempBuffer, tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void breathe(uint32_t time, uint32_t period) {
    MFBUS_RGB_BLINK cmd = {.time = time, .period = period};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BREATHECMD, tempBuffer,
                                      tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void on(uint32_t time) {
    MFBUS_RGB_ON cmd = {.time = time};
    __PACK_NAME_WITH_CMD(cmd);
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_ONCMD, tempBuffer,
                                      tempBufferSize);
    if (id)
      bus.deleteMessageId(id);
  }
  void off() {
    uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_OFFCMD, (uint8_t *)name,
                                      strlen(name) + 1);
    if (id)
      bus.deleteMessageId(id);
  }
};
#define MFBUSRGB(a, ...) mfBusRGB(a)(#a, __VA_ARGS__)

#endif