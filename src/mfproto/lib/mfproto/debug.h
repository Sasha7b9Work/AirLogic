#ifndef STRDEBUG_H
#define STRDEBUG_H

#include "Stream.h"
#include "mfbus.h"
#include <inttypes.h>

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>

#ifndef DEBUG_BUFFER_SIZE
#define DEBUG_BUFFER_SIZE 2048
#endif
class DebugSerial : public Stream {
private:
  mfBus &bus;
  CircularBuffer<char, DEBUG_BUFFER_SIZE> buffer;
  atomic_bool bufMutex = ATOMIC_FLAG_INIT;

public:
  DebugSerial(mfBus &_bus);
  ~DebugSerial();
  operator bool() { return true; }
  void end();
  virtual int available(void);
  virtual int peek(void);
  virtual int read(void);
  int availableForWrite(void);
  virtual void flush(void);
  size_t write(const uint8_t *_buffer, size_t size);
  size_t write(uint8_t _byte);
  using Print::write; // pull in write(str) and
                      // write(buf, size) from Print
};
#endif
