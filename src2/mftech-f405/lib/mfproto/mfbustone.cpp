#include "mfbustone.h"

mfBusBeep::mfBusBeep(const char *_name, mfBus &_bus, uint8_t _device)
    : bus(_bus), device(_device) {
  strncpy(name, _name, 31);
}
mfBusBeep::~mfBusBeep() {
}
void mfBusBeep::tone(std::vector<MFBUS_BEEP_TONE> notes) {
  uint8_t _len = notes.size();
  uint16_t cmdlen = 1 + _len * sizeof(MFBUS_BEEP_TONE);
  uint8_t cmd[cmdlen];
  cmd[0] = _len;
  for (uint8_t i = 0; i < _len; i++) {
    MFBUS_BEEP_TONE *n =
        (MFBUS_BEEP_TONE *)(&cmd[1 + i * sizeof(MFBUS_BEEP_TONE)]);
    *n = notes.at(i);
  }
  uint32_t id =
      bus.sendMessageWait(device, MFBUS_BEEP_TONECMD, cmd, cmdlen);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusBeep::noTone() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_BEEP_NOTONECMD, NULL, 0);
  if (id)
    bus.deleteMessageId(id);
}