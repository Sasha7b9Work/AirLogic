
#include "debug.h"
#include "Stream.h"
#include "mfproto.h"
#include <inttypes.h>

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>

DebugSerial::DebugSerial(mfBus &_bus) : bus(_bus) {
  buffer.clear();
  /*bufMutex.store(0);*/
}
DebugSerial::~DebugSerial() {}
int DebugSerial::available(void) { return buffer.size(); }
int DebugSerial::peek(void) { return buffer.first(); }
int DebugSerial::read(void) {
  char c = '\0';
  if (mfProto::getMutex(bufMutex)) {
    if (buffer.size()) {
      c = buffer.shift();
    }
    mfProto::freeMutex(bufMutex);
  }
  return c;
}
int DebugSerial::availableForWrite(void) {
  return DEBUG_BUFFER_SIZE - buffer.size();
}
void DebugSerial::flush(void) {
  if (mfProto::getMutex(bufMutex)) {
    buffer.clear();
    mfProto::freeMutex(bufMutex);
  }
}
size_t DebugSerial::write(uint8_t _byte) {
  if (mfProto::getMutex(bufMutex)) {
    buffer.push(_byte);
    mfProto::freeMutex(bufMutex);
  }
  return sizeof(_byte);
}
inline size_t DebugSerial::write(const uint8_t *_buffer, size_t size) {
  uint8_t *cbuf = (uint8_t *)_buffer;
  uint32_t len = size;
  uint16_t c = 0;
  while (c < len) {
    write(cbuf[c++]);
  }
  return c;
}