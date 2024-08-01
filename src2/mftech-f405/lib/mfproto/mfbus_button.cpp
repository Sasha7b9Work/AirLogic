#include "mfbus_button.h"
//#include "debug.h"
// extern DebugSerial debug;

mfBusButton::mfBusButton(const char *_name, mfBus &_bus, uint8_t _device)
    : bus(_bus), device(_device) {
  instances.push_back(this);
  strncpy(name, _name, 31);
  lock.exchange(false);
}
mfBusButton::~mfBusButton() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}

void mfBusButton::begin() { read(); }

mfButtonFlags mfBusButton::getState() {
  if (read()) {
    mfButtonFlags retFlag = flag;
    flag = MFBUTTON_NOSIGNAL;
    return retFlag;
  } else {
    return flag;
  }
}

bool mfBusButton::read() {
  bool ret = false;
  if (!bus.slaveExists(device)) {
    //    debug.printf("[%s] No device!\n", name);
    return false;
  }

  if (lock.exchange(true)) {
    //    debug.printf("[%s] locked!\n", name);
    return false;
  }

  uint32_t id = bus.sendMessageWait(device, MFBUS_BUTTON_GETSTATECMD,
                                    (uint8_t *)name, strlen(name) + 1, 1, 50);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.size == sizeof(mfButtonFlags)) {
      mfButtonFlags newFlag = *((mfButtonFlags *)msg.data);
      if (newFlag != MFBUTTON_NOSIGNAL) {
        //        debug.printf("[%s] Got new state!\n", name);
        uint32_t id2 =
            bus.sendMessageWait(device, MFBUS_BUTTON_CONFIRMSTATECMD,
                                (uint8_t *)name, strlen(name) + 1, 1, 50);
        if (id2)
          bus.deleteMessageId(id2);
      }
      flag = newFlag;
      ret = true;
    }
  }
  //  if (!ret)
  //    debug.printf("[%s] no answer!\n", name);
  lock.store(false);
  return ret;
}