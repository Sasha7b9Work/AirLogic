#include "mfbusrgb.h"

#define MFBUS_RGB_BLINKFADE MFBUS_RGB_BLINK
#define MFBUS_RGB_BREATHE MFBUS_RGB_BLINK
#define __MAX_TEMP_SIZE (MFPROTO_MAXDATA)
#define __PACK_NAME_WITH_CMD(a)                                                \
  uint8_t tempBuffer[__MAX_TEMP_SIZE];                                         \
  for (uint16_t _i = 0; _i < sizeof((a)); _i++)                                \
    tempBuffer[_i] = ((uint8_t *)&(a))[_i];                                    \
  for (uint16_t _i = 0; _i <= strlen(name); _i++)                              \
    tempBuffer[_i + sizeof((a))] = name[_i];                                   \
  uint16_t tempBufferSize = sizeof((a)) + strlen(name) + 1;

mfBusRGB::mfBusRGB(const char *_name, mfBus &_bus, uint8_t _deviceid)
    : bus(_bus), device(_deviceid) {

  instances.push_back(this);
  strncpy(name, _name, 31);
}
mfBusRGB::~mfBusRGB() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}
void mfBusRGB::setColor(uint8_t r, uint8_t g, uint8_t b) {
  MFBUS_RGB_SETCOLOR cmd = {.r = r, .g = g, .b = b};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_SETCOLORCMD, tempBuffer,
                                    tempBufferSize, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusRGB::setColor(uint32_t color) {
  uint8_t r, g, b;
  r = (color & 0xFF0000) >> 16;
  g = (color & 0xFF00) >> 8;
  b = (color & 0xFF);
  setColor(r, g, b);
}
uint8_t mfBusRGB::setBrightness(uint8_t brightness) {
  MFBUS_RGB_BRIGHTNESS cmd = {.brightness = brightness};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_SETBRIGHTNESSCMD,
                                    tempBuffer, tempBufferSize);
  uint8_t ret = 0;
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.size == sizeof(uint8_t)) {
      ret = *((uint8_t *)msg.data);
    }
  }
  return ret;
}
void mfBusRGB::blink(uint32_t time, uint32_t period, uint32_t phase) {
  MFBUS_RGB_BLINK cmd = {.time = time, .period = period, .phase = phase};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BLINKCMD, tempBuffer,
                                    tempBufferSize, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusRGB::blinkfade(uint32_t time, uint32_t period, uint32_t phase) {
  MFBUS_RGB_BLINK cmd = {.time = time, .period = period, .phase = phase};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BLINKFADECMD, tempBuffer,
                                    tempBufferSize, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusRGB::breathe(uint32_t time, uint32_t period, uint32_t phase) {
  MFBUS_RGB_BLINK cmd = {.time = time, .period = period, .phase = phase};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_BREATHECMD, tempBuffer,
                                    tempBufferSize, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusRGB::on(uint32_t time) {
  MFBUS_RGB_ON cmd = {.time = time};
  __PACK_NAME_WITH_CMD(cmd);
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_ONCMD, tempBuffer,
                                    tempBufferSize, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusRGB::off() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_RGB_OFFCMD, (uint8_t *)name,
                                    strlen(name) + 1, 2, 20);
  if (id)
    bus.deleteMessageId(id);
}