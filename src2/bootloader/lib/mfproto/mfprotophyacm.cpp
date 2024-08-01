#if defined(HAL_PCD_MODULE_ENABLED)
#include <USBSerial.h>
#include <usbd_cdc_if.h>

#include <wiring.h>

#include "mfproto.h"

#include "mfprotophy.h"
#include "mfprotophyacm.h"

mfProtoPhyACM::mfProtoPhyACM(const char *_name, uint32_t _SPEED)
    : baudrate(_SPEED) {
  strncpy(name, _name, 31);
  instances.push_back(this);
}
mfProtoPhyACM::~mfProtoPhyACM() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}

void mfProtoPhyACM::begin() {
  // SerialUSB.end();
  // CDC_deInit();
  SerialUSB.begin(baudrate);
}

uint32_t mfProtoPhyACM::getSpeed(void) { return baudrate; }

void mfProtoPhyACM::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  SerialUSB.write(packet, len);
  SerialUSB.flush();
}
void mfProtoPhyACM::read() {
  if (SerialUSB.available()) {
    while (SerialUSB.available()) {
      char c = SerialUSB.read();
      if (putChar) {
        putChar(c);
      }
    }
  }
}

#endif