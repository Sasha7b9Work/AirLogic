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
  name = (char *)malloc(strlen(_name) + 1);
  if (name != NULL) {
    strcpy(name, _name);
  }
  instances.push_back(this);
  _me = instances.end();
}
mfProtoPhyPipe::~mfProtoPhyPipe() {
  instances.erase(_me);
  if (name != nullptr)
    free(name);
}

void mfProtoPhyPipe::begin() {}

void mfProtoPhyPipe::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  if (writeToPipe != nullptr)
    writeToPipe(this, packet, len);
}
void mfProtoPhyPipe::read() {}
