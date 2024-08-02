
#include <sketch.h>

#include "mfprotophy.h"
//#include "mfprotophyacm.h"
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

#if defined(REV1)
#define deviceName "alpro_v10_button"
static const uint8_t deviceId = MF_ALPRO_V10_BUTTON;
constexpr uint16_t timerFreq = 60;
#elif defined(REV2)
#define deviceName "alpro_v10_button_v2"
static const uint8_t deviceId = MF_ALPRO_V10_BUTTON_V2;
constexpr uint16_t timerFreq = 30;
volatile bool autoMode = true;
ADC_HandleTypeDef hadc;
#else
#error "Wrong revision"
#endif
MFBUS(SlaveBus, serial, deviceId, MFBUS_CAP_KEYS);

#if defined(REV1)
HardwareTimer beep(TIM3);
CPIN(LR3, PB1, CPIN_HWLED, CPIN_HIGH);
CPIN(LG3, PB2, CPIN_LED, CPIN_HIGH);
CPIN(LB3, PA8, CPIN_HWLED, CPIN_HIGH);

CPIN(LR2, PA6, CPIN_HWLED, CPIN_HIGH);
CPIN(LG2, PA7, CPIN_HWLED, CPIN_HIGH);
CPIN(LB2, PB0, CPIN_HWLED, CPIN_HIGH);

CPIN(LR1, PA3, CPIN_HWLED, CPIN_HIGH);
CPIN(LG1, PA4, CPIN_HWLED, CPIN_HIGH);
CPIN(LB1, PA5, CPIN_HWLED, CPIN_HIGH);

MFBUTTON(UP, PA0, INPUT_PULLUP);
MFBUTTON(OK, PA1, INPUT_PULLUP);
MFBUTTON(DOWN, PA2, INPUT_PULLUP);
#elif defined(REV2)
HardwareTimer beep(TIM2);
CPIN(LR3, PB5, CPIN_HWLED, CPIN_HIGH);
CPIN(LG3, PB6, CPIN_HWLED, CPIN_HIGH);
CPIN(LB3, PA8, CPIN_HWLED, CPIN_HIGH);

CPIN(LR2, PB0, CPIN_HWLED, CPIN_HIGH);
CPIN(LG2, PB1, CPIN_HWLED, CPIN_HIGH);
CPIN(LB2, PB4, CPIN_HWLED, CPIN_HIGH);

CPIN(LR1, PA3, CPIN_HWLED, CPIN_HIGH);
CPIN(LG1, PA4, CPIN_HWLED, CPIN_HIGH);
CPIN(LB1, PA7, CPIN_HWLED, CPIN_HIGH);

MFBUTTON(UP, PB2, INPUT_PULLUP);
MFBUTTON(OK, PA0, INPUT_PULLUP);
MFBUTTON(DOWN, PA1, INPUT_PULLUP);
#else
#error "Wrong revision"
#endif
MFRGB(ledDOWN, &LR1, &LG1, &LB1);
MFRGB(ledUP, &LR3, &LG3, &LB3);
MFRGB(ledOK, &LR2, &LG2, &LB2);
//#include "debug.h"
// DebugSerial debug(SlaveBus);

#define BUZZER PA11
#define BZ_PORT GPIOA
#define BZ_PIN GPIO_PIN_11
__IO uint32_t toneStart = HAL_GetTick();
__IO bool playing = false;
std::vector<MFBUS_BEEP_TONE> notes_queue;

__IO bool recvd = false;

typedef struct {
  uint32_t id = 0;
  mfButtonFlags state;
} buttonState_t;

buttonState_t *states;

HardwareTimer ledTimer(TIM6);
void InitADC();
void protect();

void beepOn() { BZ_PORT->BSRR = BZ_PIN << (!(1U) << 4); };
void beepOff() { BZ_PORT->BSRR = BZ_PIN << (!(0U) << 4); };
void beepFreq(uint16_t freq) {
  beep.pause();
  if (!freq) {
    beepOff();
    return;
  }
  beep.setOverflow(freq, HERTZ_FORMAT);
  beep.setCaptureCompare(1, 50, PERCENT_COMPARE_FORMAT);
  beep.resume();
}
inline void _tone(uint16_t freq) { beepFreq(freq); }
inline void _notone(void) { beepFreq(0); }

void setup() {
  protect();
  char _name[] = deviceName;
  SlaveBus.setName(_name);
#if defined(REV2)
  InitADC();
#endif
  // workaround for fake f051
  ledTimer.setOverflow(timerFreq * 256, HERTZ_FORMAT);
  cPins::timerFreq = timerFreq;
  ledTimer.attachInterrupt(cPins::timerCallback);
  ledTimer.resume();
  pinMode(BUZZER, OUTPUT);
  beep.attachInterrupt(beepOn);
  beep.setMode(1, TIMER_OUTPUT_COMPARE);
  beep.attachInterrupt(1, beepOff);
  // /workaround
  COM1.redefineIrqHandler();
  SlaveBus.setRecvCallback([](mfBus *bus) -> void { recvd = true; });
  SlaveBus.begin();
  for (uint8_t i = 0; i < mfButton::instances.size(); i++) {
    mfButton::instances[i]->begin();
  }
  notes_queue.reserve(20);
  states = new buttonState_t[mfButton::instances.size()];
}
#if defined(REV2)
volatile uint16_t adcValue = 4095;
volatile uint8_t dynamicPWM = 102;
uint8_t oldPWM = 102;
volatile uint32_t tickCounter = 0;
void HAL_SYSTICK_Callback() {
  if (autoMode)
    if (!(tickCounter % 64)) {
      HAL_ADC_Start(&hadc);
      HAL_ADC_PollForConversion(&hadc, 100);
      uint16_t adcCurrent = HAL_ADC_GetValue(&hadc);
      HAL_ADC_Stop(&hadc); // stop adc
      adcValue = ((adcValue * 37) + (adcCurrent)) / 38;
      dynamicPWM = adcValue / 40;
      if (abs(dynamicPWM - oldPWM) < 1)
        dynamicPWM = oldPWM;
      if ((dynamicPWM != oldPWM) && (dynamicPWM <= 90)) {
        ledUP.setBrightness(110 - dynamicPWM);
        ledOK.setBrightness(110 - dynamicPWM);
        ledDOWN.setBrightness(110 - dynamicPWM);
      }
    }
  ++tickCounter;
}
#endif
void loop() {
#if (MFBUTTON_USE_EXTI != 1)
  for (uint8_t i = 0; i < mfButton::instances.size(); i++)
    mfButton::instances[i]->read();
#endif
  if (!recvd) {
    if (!playing) {
      if (notes_queue.size()) {
        toneStart = HAL_GetTick();
        if (notes_queue[0].freq)
          _tone(notes_queue[0].freq);
        playing = true;
      }
    } else {
      if (HAL_GetTick() - toneStart >= (notes_queue[0].duration * 9) / 10) {
        _notone();
      }
      if (HAL_GetTick() - toneStart >= notes_queue[0].duration) {
        notes_queue.erase(notes_queue.begin());
        playing = false;
      }
    }
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
                      mfButtonFlags state;
                      if (states[i].id) {
                        state = states[i].state;
                      } else {
                        state = mfButton::instances[i]->getState();
                        if (state != MFBUTTON_NOSIGNAL) {
                          states[i] = {msg->head.id, state};
                        }
                      }
                      instance->sendMessage(newmsg, (uint8_t *)&state,
                                            sizeof(mfButtonFlags));
                    }
                  }
                }
              }
            } break;
            case MFBUS_BUTTON_CONFIRMSTATECMD: {
              if (msg->head.size) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  for (uint32_t i = 0; i < mfButton::instances.size(); i++) {
                    if (!strcmp((const char *)msg->data,
                                mfButton::instances[i]->name)) {
                      states[i].id = 0;
                      instance->sendMessage(newmsg, NULL, 0);
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
                  if (rgb) {
                    rgb->blink(cmd->time, cmd->period);
                    rgb->setPhase(cmd->phase);
                  }
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
                  if (rgb) {
                    rgb->breathe(cmd->time, cmd->period);
                    rgb->setPhase(cmd->phase);
                  }
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
                  if (rgb) {
                    rgb->blinkfade(cmd->time, cmd->period);
                    rgb->setPhase(cmd->phase);
                  }
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
#define PER(a, b) (((a) * (b)) / 100)
            case MFBUS_RGB_SETCOLORCMD: {
              if (msg->head.size > sizeof(MFBUS_RGB_SETCOLOR)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_SETCOLOR *cmd = (MFBUS_RGB_SETCOLOR *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb)
                    // rgb->setColor(PER(cmd->r, 90), PER(cmd->g, 100),
                    //              PER(cmd->b, 75));
                    rgb->setColor(cmd->r, cmd->g, cmd->b);
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_RGB_SETBRIGHTNESSCMD: {
              uint8_t answer = 0;
              if (msg->head.size > sizeof(MFBUS_RGB_BRIGHTNESS)) {
                if (msg->data[msg->head.size - 1] == '\0') {
                  MFBUS_RGB_BRIGHTNESS *cmd = (MFBUS_RGB_BRIGHTNESS *)msg->data;
                  char *_name = (char *)&msg->data[sizeof(*cmd)];
                  mfRGB *rgb = mfRGB::findByName(_name);
                  if (rgb) {
#if defined(REV2)
                    if (cmd->brightness > 100)
                      autoMode = true;
                    else {
                      autoMode = false;
                      rgb->setBrightness(cmd->brightness);
                    }
#else
                    rgb->setBrightness(cmd->brightness);
#endif
                  }
                  instance->sendMessage(newmsg, &answer, sizeof(answer));
                }
              }
            } break;
              /********************************
               ************* tone *************
               ********************************/
            case MFBUS_BEEP_TONECMD: {
              if (msg->head.size) {
                uint8_t _len = msg->data[0];
                if (msg->head.size == 1 + (_len * sizeof(MFBUS_BEEP_TONE))) {
                  MFBUS_BEEP_TONE *notes = (MFBUS_BEEP_TONE *)(&msg->data[1]);
                  for (uint8_t i = 0; i < _len; i++) {
                    notes_queue.push_back(notes[i]);
                  }
                  instance->sendMessage(newmsg, NULL, 0);
                }
              }
            } break;
            case MFBUS_BEEP_NOTONECMD: {
              notes_queue.clear();
              _notone();
              // noTone(pinNametoDigitalPin(BUZZ));
              instance->sendMessage(newmsg, NULL, 0);
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
/*const PinMap PinMap_PWM[] = {
    {PA_11, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 4,
                      0)}, // TIM1_CH4
};*/
#if defined(REV1)
const PinMap PinMap_PWM[] = {
    //{PA_3, TIM2,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM2, 4,
    //                  0)}, // TIM2_CH4
    {PA_3, TIM15,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF0_TIM15, 2,
                      0)}, // TIM15_CH2
    {PA_4, TIM14,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF4_TIM14, 1,
                      0)}, // TIM14_CH1
    {PA_5, TIM2,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM2, 1,
                      0)}, // TIM2_CH1
    //  {PA_6,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 1, 0)}, // TIM3_CH1
    {PA_6, TIM16,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_TIM16, 1,
                      0)}, // TIM16_CH1
    //{PA_7, TIM1,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 1,
    //                  1)}, // TIM1_CH1N - C_FET_LO
    //{PA_7, TIM3,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM3, 2,
    //                  0)}, // TIM3_CH2 - C_FET_LO
    //{PA_7, TIM14,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF4_TIM14, 1,
    //                  0)}, // TIM14_CH1 - C_FET_LO
    {PA_7, TIM17,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_TIM17, 1,
                      0)}, // TIM17_CH1 - C_FET_LO
    {PA_8, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 1,
                      0)}, // TIM1_CH1 - B_FET_HI
    //  {PA_9,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM1, 2, 0)}, // TIM1_CH2 - A_FET_LO
    //  {PA_10, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM1, 3, 0)}, // TIM1_CH3 - A_FET_HI
    {PA_11, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 4,
                      0)}, // TIM1_CH4
    //  {PA_15, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM2, 1, 0)}, // TIM2_CH1 - LED_RED
    {PB_0, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 2,
                      1)}, // TIM1_CH2N - C_FET_HI
    //  {PB_0,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 3, 0)}, // TIM3_CH3 - C_FET_HI
    {PB_1, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 3,
                      1)}, // TIM1_CH3N - B_FET_LO
    //  {PB_1,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 4, 0)}, // TIM3_CH4 - B_FET_LO
    //{PB_1, TIM14,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF0_TIM14, 1,
    //                  0)}, // TIM14_CH1 - B_FET_LO
    //  {PB_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM2, 2, 0)}, // TIM2_CH2 - LED_GREEN
    //  {PB_4,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 1, 0)}, // TIM3_CH1 - LED_BLUE
    //  {PB_5,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 2, 0)}, // TIM3_CH2
    //  {PB_6,  TIM16,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM16, 1, 1)}, // TIM16_CH1N
    //  {PB_7,  TIM17,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM17, 1, 1)}, // TIM17_CH1N
    //  {PB_8,  TIM16,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM16, 1, 0)}, // TIM16_CH1
    {NC, NP, 0}};
#elif defined(REV2)
const PinMap PinMap_PWM[] = {
    //{PA_3, TIM2,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM2, 4,
    //                  0)}, // TIM2_CH4
    {PA_3, TIM15,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF0_TIM15, 2,
                      0)}, // TIM15_CH2
    {PA_4, TIM14,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF4_TIM14, 1,
                      0)}, // TIM14_CH1
    //{PA_5, TIM2,     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    // GPIO_AF2_TIM2, 1,                      0)}, // TIM2_CH1
    //  {PA_6,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 1, 0)}, // TIM3_CH1
    // {PA_6, TIM16, STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    // GPIO_AF5_TIM16, 1, 0)}, // TIM16_CH1
    //{PA_7, TIM1,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 1,
    //                  1)}, // TIM1_CH1N - C_FET_LO
    //{PA_7, TIM3,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM3, 2,
    //                  0)}, // TIM3_CH2 - C_FET_LO
    //{PA_7, TIM14,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF4_TIM14, 1,
    //                  0)}, // TIM14_CH1 - C_FET_LO
    {PA_7, TIM17,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_TIM17, 1,
                      0)}, // TIM17_CH1 - C_FET_LO
    {PA_8, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 1,
                      0)}, // TIM1_CH1 - B_FET_HI
    //  {PA_9,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM1, 2, 0)}, // TIM1_CH2 - A_FET_LO
    //  {PA_10, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM1, 3, 0)}, // TIM1_CH3 - A_FET_HI
    {PA_11, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 4,
                      0)}, // TIM1_CH4
    //  {PA_15, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM2, 1, 0)}, // TIM2_CH1 - LED_RED
    {PB_0, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 2,
                      1)}, // TIM1_CH2N - C_FET_HI
    //  {PB_0,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 3, 0)}, // TIM3_CH3 - C_FET_HI
    {PB_1, TIM1,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM1, 3,
                      1)}, // TIM1_CH3N - B_FET_LO
    //  {PB_1,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF1_TIM3, 4, 0)}, // TIM3_CH4 - B_FET_LO
    //{PB_1, TIM14,
    // STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF0_TIM14, 1,
    //                  0)}, // TIM14_CH1 - B_FET_LO
    //  {PB_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM2, 2, 0)}, // TIM2_CH2 - LED_GREEN
    {PB_4, TIM3,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM3, 1,
                      0)}, // TIM3_CH1 - LED_BLUE
    {PB_5, TIM3,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM3, 2,
                      0)}, // TIM3_CH2
    {PB_6, TIM16,
     STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM16, 1,
                      1)}, // TIM16_CH1N
    //  {PB_7,  TIM17,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM17, 1, 1)}, // TIM17_CH1N
    //  {PB_8,  TIM16,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
    //  GPIO_AF2_TIM16, 1, 0)}, // TIM16_CH1
    {NC, NP, 0}};
#else
#error "wrong revision"
#endif
extern "C" {
void SystemClock_Config(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
  }
  LL_RCC_HSI_Enable();

  /* Wait till HSI is ready */
  while (LL_RCC_HSI_IsReady() != 1) {
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_HSI14_Enable();

  /* Wait till HSI14 is ready */
  while (LL_RCC_HSI14_IsReady() != 1) {
  }
  LL_RCC_HSI14_SetCalibTrimming(16);
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
  }
  LL_RCC_HSI14_EnableADCControl();
  LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK1);
}
}
#endif
void _Error_Handler(const char *, int) {}
#if defined(REV2)
void InitADC(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  ADC_ChannelConfTypeDef sConfig = {0};
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
    Error_Handler();
  }
}
#endif
void protect() {
  FLASH_OBProgramInitTypeDef config;
  HAL_FLASHEx_OBGetConfig(&config);
  if (config.RDPLevel != OB_RDP_LEVEL_2) {
    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    config.OptionType = OPTIONBYTE_RDP;
    config.RDPLevel = OB_RDP_LEVEL_2;
    HAL_FLASHEx_OBProgram(&config);
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    __set_PRIMASK(irq_state);
  }
}
