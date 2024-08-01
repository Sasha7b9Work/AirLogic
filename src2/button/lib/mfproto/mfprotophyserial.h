#ifndef MF_PROTO_PHY_SERIAL_H
#define MF_PROTO_PHY_SERIAL_H

#include <wiring.h>

#if defined(HAL_UART_MODULE_ENABLED)

#define MFPROTO_PHY_SERIAL_NC 0xFFFF

#ifndef MFPROTO_SERIAL_MAXBAUD
#define MFPROTO_SERIAL_MAXBAUD (5250000UL)
#endif

class mfProtoPhySerial : public mfProtoPhy {
private:
  uint32_t baudrate;            // uart baudrate
  uint16_t txEn;                // tx enable pin for rs485
  HardwareSerial &UART;         // Serial instance
  char name[32] = ""; // object name, filled by MFPROTO define, can be
                                // used for debug purposes
  GPIO_TypeDef *port;
  uint16_t pin;

  static void serialIrqHandler(serial_t *obj);
  bool irqSet = false;
  uint16_t iCgot = 0;
  IRQn_Type irq;

public:
  inline static std::vector<mfProtoPhySerial *> instances = {};

  mfProtoPhySerial(const char *_name, HardwareSerial &_UART,
                   uint16_t _RTS = MFPROTO_PHY_SERIAL_NC,
                   uint32_t _SPEED = 115200);
  ~mfProtoPhySerial();
  void begin();
  void read();
  void send(uint8_t *packet, uint16_t len, uint8_t flags = MFPHY_STARTEND);
  void redefineIrqHandler(void);
  uint32_t getSpeed(void);
  uint32_t getMaxSpeed(void) { return MFPROTO_SERIAL_MAXBAUD; }
};

#define MFPROTOPHYSERIAL(a, ...) mfProtoPhySerial(a)(#a, __VA_ARGS__)
#endif

#endif
