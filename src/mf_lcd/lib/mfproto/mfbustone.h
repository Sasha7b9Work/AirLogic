#ifndef MFBUS_TONE_H
#define MFBUS_TONE_H

typedef struct {
  uint16_t freq, duration;
} MFBUS_BEEP_TONE;

class mfBusBeep {
private:
  mfBus &bus;
  uint8_t device;

public:
  char *name = nullptr;
  mfBusBeep(const char *_name, mfBus &_bus, uint8_t _device)
      : bus(_bus), device(_device) {
    name = (char *)malloc(strlen(_name) + 1);
    if (name)
      strcpy(name, _name);
  }
  ~mfBusBeep() {
    if (name)
      free(name);
  }
  void tone(uint16_t freq, uint16_t duration) {
    MFBUS_BEEP_TONE cmd = {.freq = freq, .duration = duration};
    uint32_t id = bus.sendMessageWait(device, MFBUS_BEEP_TONECMD,
                                      (uint8_t *)&cmd, sizeof(cmd));
    if (id)
      bus.deleteMessageId(id);
  }
  void noTone() {
    uint32_t id = bus.sendMessageWait(device, MFBUS_BEEP_NOTONECMD, NULL, 0);
    if (id)
      bus.deleteMessageId(id);
  }
};
#endif