#include "mfbus_button.h"

mfBusButton::mfBusButton(const char *_name, mfBus &_bus, uint8_t _device)
    : bus(_bus), device(_device) {
  instances.push_back(this);
  _me = instances.end();
  name = (char *)malloc(strlen(_name) + 1);
  if (name)
    strcpy(name, _name);
}
mfBusButton::~mfBusButton() {
  instances.erase(_me);
  if (name)
    free(name);
}

void mfBusButton::begin() { read(); }
mfButtonFlags mfBusButton::getState() {
  read();
  mfButtonFlags retFlag = flag;
  flag = MFBUTTON_NOSIGNAL;
  return retFlag;
}

void mfBusButton::read() {
  if (!bus.slaveExists(device))
    return;
  uint32_t id = bus.sendMessageWait(device, MFBUS_BUTTON_GETSTATECMD,
                                    (uint8_t *)name, strlen(name) + 1);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.size == sizeof(mfButtonFlags)) {
      flag = *((mfButtonFlags *)msg.data);
    }
  }
}