
#include <sketch.h>

#include "mfprotophy.h"
#include "mfprotophyacm.h"
#include "mfprotophyserial.h"

#include "mfbus.h"
#include "mfbusrgb.h"
#include "mfbustone.h"
#include "mfbutton.h"
#include "mfrgb.h"
#include "stm32yyxx_ll_pwr.h"
#include "stm32yyxx_ll_rcc.h"
#include "stm32yyxx_ll_system.h"
#include "stm32yyxx_ll_utils.h"
#include <cPins.h>

#define BAUD (MFPROTO_SERIAL_MAXBAUD / 20)

MFPROTOPHYSERIAL(COM1, Serial1, PA12, BAUD /*115200*/);
MFPROTO(serial, COM1);
MFBUS(SlaveBus, serial, MF_ALPRO_V10_BUTTON, MFBUS_CAP_KEYS);

CPIN(LR1, PA3, CPIN_LED, CPIN_HIGH);
CPIN(LG1, PA4, CPIN_LED, CPIN_HIGH);
CPIN(LB1, PA5, CPIN_LED, CPIN_HIGH);
MFRGB(ledUP, &LR1, &LG1, &LB1);

CPIN(LR2, PA6, CPIN_LED, CPIN_HIGH);
CPIN(LG2, PA7, CPIN_LED, CPIN_HIGH);
CPIN(LB2, PB0, CPIN_LED, CPIN_HIGH);
MFRGB(ledOK, &LR2, &LG2, &LB2);

CPIN(LR3, PB1, CPIN_LED, CPIN_HIGH);
CPIN(LG3, PB2, CPIN_LED, CPIN_HIGH);
CPIN(LB3, PA8, CPIN_LED, CPIN_HIGH);
MFRGB(ledDOWN, &LR3, &LG3, &LB3);

MFBUTTON(UP, PA2, INPUT_PULLUP);
MFBUTTON(OK, PA1, INPUT_PULLUP);
MFBUTTON(DOWN, PA0, INPUT_PULLUP);

#include "debug.h"
DebugSerial debug(SlaveBus);

#define BUZZ PA11

__IO bool recvd = false;

void setup() {
  char _name[] = "alpro_v10_button";
  SlaveBus.setName(_name);
  cPins::initTimer(TIM1, 60);
  COM1.redefineIrqHandler();
  SlaveBus.setRecvCallback([](mfBus *bus) -> void {
    recvd = true;
    // ledOK.blink(300, 50);
  });
  SlaveBus.begin();
  for (uint8_t i = 0; i < mfButton::instances.size(); i++) {
    mfButton::instances[i]->begin();
  }
}
void loop() {
  if (!recvd) {
    YIELD();
  }
  if (recvd) {
    recvd = false;
    while (SlaveBus.haveMessages()) {
      MF_MESSAGE msg;
      SlaveBus.readFirstMessage(msg);
      SlaveBus.answerMessage(
          &msg, [](mfBus *instance, MF_MESSAGE *msg, MF_MESSAGE *newmsg) {
            uint8_t ret = 1;
            switch (msg->head.type) {
              /********************************
               ************ buttons ***********
               ********************************/
            case MFBUS_BUTTON_GETSTATECMD: {
              if (msg->head.size) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  for (uint32_t i = 0; i < mfButton::instances.size(); i++) {
                    if (!strcmp((const char *)msg->data,
                                mfButton::instances[i]->name)) {
                      mfButtonFlags state = mfButton::instances[i]->getState();
                      instance->sendMessage(newmsg, (uint8_t *)&state,
                                            sizeof(mfButtonFlags));
                    }
                  }
                }
              }
            } break;
              /********************************
               *********** rgb leds ***********
               ********************************/
            case MFBUS_RGB_ONCMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_ON)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_ON *cmd = (MFBUS_RGB_ON *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->on(cmd->time);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_OFFCMD: {
              if (msg->head.size) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  char *_name = (char *)msg->data;
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->off();
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_BLINKCMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->blink(cmd->time, cmd->period);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_BREATHECMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->breathe(cmd->time, cmd->period);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_BLINKFADECMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->blinkfade(cmd->time, cmd->period);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_SETCOLORCMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_SETCOLOR)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_SETCOLOR *cmd = (MFBUS_RGB_SETCOLOR *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->setColor(cmd->r, cmd->g, cmd->b);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_SETBRIGHTNESSCMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_BRIGHTNESS)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_BRIGHTNESS *cmd = (MFBUS_RGB_BRIGHTNESS *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    rgb->setBrightness(cmd->brightness);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
              /********************************
               ************* tone *************
               ********************************/
            case MFBUS_BEEP_TONECMD: {
              if (msg->head.size == sizeof(MFBUS_BEEP_TONE)) {
                MFBUS_BEEP_TONE *cmd = (MFBUS_BEEP_TONE *)msg->data;
                tone(BUZZ, cmd->freq, cmd->duration);
              }
            } break;
            case MFBUS_BEEP_NOTONECMD: {
              noTone(BUZZ);
            } break;
            default:
              ret = 0;
            }
            return ret;
          });
    }
  }
}
#if 1
extern "C" {
void SystemClock_Config(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

  if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
    Error_Handler();
  }
  LL_RCC_HSI_Enable();

  /* Wait till HSI is ready */
  while (LL_RCC_HSI_IsReady() != 1) {
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1) {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }
  LL_SetSystemCoreClock(48000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  };
  LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_SYSCLK);
}
}
#endif