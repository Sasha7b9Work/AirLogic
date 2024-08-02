#if defined(HAL_UART_MODULE_ENABLED)
#include <pins_stm.h>
#include <wiring.h>

#include "mfproto.h"
#include "stm32_def.h"
#include "stm32yyxx_ll_gpio.h"

#include "mfprotophy.h"
#include "mfprotophyserial.h"

void mfProtoPhySerial::serialIrqHandler(serial_t *obj) {
#ifdef MF_RTOS
  BaseType_t status = taskENTER_CRITICAL_FROM_ISR();
#endif
  mfProtoPhySerial *phyInstance = NULL;
  for (uint16_t i = 0; i < mfProtoPhySerial::instances.size(); i++) {
    if (obj == &mfProtoPhySerial::instances[i]->UART._serial) {
      phyInstance = mfProtoPhySerial::instances[i];
    }
  }
  if (phyInstance != NULL) {
    if (phyInstance->putChar != nullptr) {
      uint8_t c;
      if (uart_getc(&phyInstance->UART._serial, &c) == 0) {
        phyInstance->putChar(c);
      }
    }
  }
#ifdef MF_RTOS
  taskEXIT_CRITICAL_FROM_ISR(status);
#endif
}

mfProtoPhySerial::mfProtoPhySerial(const char *_name, HardwareSerial &_UART,
                                   uint16_t _RTS, uint32_t _SPEED)
    : baudrate(_SPEED), txEn(_RTS), UART(_UART),
      port(get_GPIO_Port(STM_PORT(digitalPinToPinName(_RTS)))),
      pin(STM_LL_GPIO_PIN(digitalPinToPinName(_RTS))) {
  name = (char *)malloc(strlen(_name) + 1);
  if (name != NULL) {
    strcpy(name, _name);
  }
  iCgot = instances.size();
  irq = UART._serial.irq;
  HAL_NVIC_SetPriority(irq, 0x8, iCgot);
  instances.push_back(this);
  _me = instances.end();
  if (txEn != MFPROTO_PHY_SERIAL_NC) {
    set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(txEn)));
    LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(port, pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(port, pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinSpeed(port, pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_ResetOutputPin(port, pin);
  }
}

mfProtoPhySerial::~mfProtoPhySerial() {
  instances.erase(_me);
  if (name != nullptr)
    free(name);
}

void mfProtoPhySerial::redefineIrqHandler(void) {
  // return;
  UART._serial.rx_callback = serialIrqHandler;
  irqSet = true;
  mfProto *parent = NULL;
  for (uint16_t i = 0; i < mfProto::instances.size(); i++) {
    if (&mfProto::instances[i]->PHY == this)
      parent = mfProto::instances[i];
  }
  if (parent != NULL)
    parent->irqSet = irqSet;
}
void mfProtoPhySerial::begin() { UART.begin(baudrate); }

uint32_t mfProtoPhySerial::getSpeed(void) { return baudrate; }

void mfProtoPhySerial::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  if ((txEn != MFPROTO_PHY_SERIAL_NC) && (flags & MFPHY_START)) {
    LL_GPIO_SetOutputPin(port, pin);
    __NOP();
  }
  UART.write(packet, len);
  UART.flush();
  if ((txEn != MFPROTO_PHY_SERIAL_NC) && (flags & MFPHY_END)) {
    LL_GPIO_ResetOutputPin(port, pin);
    __NOP();
  }
}
void mfProtoPhySerial::read() {
  if (irqSet)
    return;
  if (this->UART.available()) {
    while (this->UART.available()) {
      char c = this->UART.read();
      if (putChar != nullptr) {
        putChar(c);
      }
    }
  }
}

#endif
