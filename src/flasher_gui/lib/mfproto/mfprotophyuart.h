#ifndef MF_PROTO_PHY_UART_H
#define MF_PROTO_PHY_UART_H
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include "serial/serial.h"
#include <iostream>

#include "mfprotophy.h"
#include "serial/serial.h"

using std::cout;
using std::endl;
using std::exception;
using std::string;
using std::vector;

class mfProtoPhyUart : public mfProtoPhy {
private:
  uint32_t baud;
  string findPort(string &vid, string &pid);
  char *name = (char *)nullptr;

public:
  serial::Serial *UART;
  mfProtoPhyUart(const char *_name, string _VID, string _PID, uint32_t baudrate,
                 uint32_t timeout = 100);
  ~mfProtoPhyUart();

  void begin();
  void send(uint8_t *packet, uint16_t len, uint8_t flags = MFPHY_STARTEND);
  void read();
  uint32_t getSpeed(void);
  uint32_t getMaxSpeed(void);
  bool isOpen(void);
};

#define MFPROTOPHYUART(a, ...) mfProtoPhyUart(a)(#a, __VA_ARGS__)
#endif