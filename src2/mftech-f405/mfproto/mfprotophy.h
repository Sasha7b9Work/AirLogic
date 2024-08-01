#ifndef MF_PROTO_PHY_H
#define MF_PROTO_PHY_H

#include <functional>
#include <stdint.h>

/* abstract PHY level interface */
#define MFPHY_CONTINUE (0)
#define MFPHY_START ((1U << 0U))
#define MFPHY_END ((1U << 1U))
#define MFPHY_STARTEND ((MFPHY_START | MFPHY_END))

class mfProtoPhy {
private:
public:
  typedef std::function<void(uint8_t)> putChar_f;
  putChar_f putChar = nullptr; // initialized by mfProto instance, used to put
                               // char read to mfProto
  virtual void begin() = 0;
  virtual void send(uint8_t *packet, uint16_t len,
                    uint8_t flags = MFPHY_STARTEND) = 0;
  virtual void read() = 0;
  virtual uint32_t getSpeed(void) = 0;
  virtual uint32_t getMaxSpeed(void) = 0;
};

#endif
