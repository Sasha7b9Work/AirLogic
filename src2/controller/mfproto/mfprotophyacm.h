#ifndef MF_PROTO_PHY_ACM_H
#define MF_PROTO_PHY_ACM_H

#include <wiring.h>

#if defined(HAL_PCD_MODULE_ENABLED)

#ifndef MFPROTO_ACM_MAXBAUD
#define MFPROTO_ACM_MAXBAUD (5250000UL)
#endif

class mfProtoPhyACM : public mfProtoPhy {
private:
  uint32_t baudrate;            // uart baudrate
  char *name = (char *)nullptr; // object name, filled by MFPROTO define, can be
                                // used for debug purposes
public:
#undef __inst__
#define __inst__ std::vector<mfProtoPhyACM *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;

  mfProtoPhyACM(const char *_name, uint32_t _SPEED = 115200);
  ~mfProtoPhyACM();
  void begin();
  void read();
  void send(uint8_t *packet, uint16_t len, uint8_t flags = MFPHY_STARTEND);
  uint32_t getSpeed(void);
  uint32_t getMaxSpeed(void) { return MFPROTO_ACM_MAXBAUD; }
};

#define MFPROTOPHYACM(a, ...) mfProtoPhyACM(a)(#a, __VA_ARGS__)
#endif

#endif
