#include "mfprotophypipe.h"

void mfProtoPhyPipe::receiveHandler(uint8_t *buffer, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    // uint32_t irq_state = __get_PRIMASK();
    //__disable_irq();
    putChar(buffer[i]);
    //__set_PRIMASK(irq_state);
  }
  return;
}

mfProtoPhyPipe::mfProtoPhyPipe(const char *_name) {
  strncpy(name, _name, 31);
  instances.push_back(this);
}
mfProtoPhyPipe::~mfProtoPhyPipe() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}

void mfProtoPhyPipe::begin() {}

void mfProtoPhyPipe::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  if (writeToPipe != nullptr)
    writeToPipe(this, packet, len);
}
void mfProtoPhyPipe::read() {}
