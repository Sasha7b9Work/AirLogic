#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mfproto.h"
#include "serial/serial.h"
#include <iostream>

using std::cout;
using std::endl;
using std::exception;
using std::string;
using std::vector;

#include "mfprotophyuart.h"

mfProtoPhyUart::mfProtoPhyUart(const char *_name, string _VID, string _PID,
                               uint32_t baudrate, uint32_t timeout)
    : baud(baudrate) {
  name = (char *)malloc(strlen(_name) + 1);
  if (name != NULL) {
    strcpy(name, _name);
  }
  string port = findPort(_VID, _PID);
  UART =
      new serial::Serial(port, baud, serial::Timeout::simpleTimeout(timeout));
  if (!UART->isOpen())
    UART->open();
}
mfProtoPhyUart::~mfProtoPhyUart() {
  if (name != NULL)
    free(name);
}
void mfProtoPhyUart::begin() {}
void mfProtoPhyUart::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  UART->write(packet, len);
  // UART->flush();
}
void mfProtoPhyUart::read() {
  if (UART->available()) {
    while (UART->available()) {
      uint8_t c;
      // printf("[%.2x]", c);
      UART->read(&c, 1);
      if (putChar)
        putChar(c);
    }
  }
}
uint32_t mfProtoPhyUart::getSpeed(void) { return baud; }
uint32_t mfProtoPhyUart::getMaxSpeed(void) { return 5250000UL; }
bool mfProtoPhyUart::isOpen(void) { return UART->isOpen(); }
string mfProtoPhyUart::findPort(string &vid, string &pid) {
  vector<serial::PortInfo> devices_found = serial::list_ports();
  vector<serial::PortInfo>::iterator iter = devices_found.begin();
  while (iter != devices_found.end()) {
    serial::PortInfo device = *iter++;
    if ((device.hardware_id.find(vid) >= 0) &&
        (device.hardware_id.find(pid) >= 0)) {
      return device.port;
    }
  }
  return "";
}