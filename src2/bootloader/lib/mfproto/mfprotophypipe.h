#ifndef MF_PROTO_PHY_PIPE_H
#define MF_PROTO_PHY_PIPE_H

#ifdef STM32_CORE
#include <wiring.h>
#else
#include <ctime>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include <functional>
#include <mfproto.h>
#include <mfprotophy.h>

#ifndef MFPROTO_SERIAL_MAXBAUD
#define MFPROTO_SERIAL_MAXBAUD (5250000UL)
#endif

class mfProtoPhyPipe : public mfProtoPhy {
private:
  char name[32] = ""; // object name, filled by MFPROTO define, can be
                                // used for debug purposes

public:
  inline static std::vector<mfProtoPhyPipe *> instances = {};

  void receiveHandler(uint8_t *, uint32_t);
  std::function<void(mfProtoPhyPipe *, uint8_t *, uint32_t)> writeToPipe =
      nullptr;

  mfProtoPhyPipe(const char *_name);
  ~mfProtoPhyPipe();
  void begin();
  void read();
  void send(uint8_t *packet, uint16_t len, uint8_t flags = MFPHY_STARTEND);
  uint32_t getSpeed(void) { return MFPROTO_SERIAL_MAXBAUD; }
  uint32_t getMaxSpeed(void) { return MFPROTO_SERIAL_MAXBAUD; }
};

#define MFPROTOPHYPIPE(a) mfProtoPhyPipe(a)(#a)
#endif