#include "sketch.h"
#include "timer.h"

#define pin PA6

TIM_TypeDef *TIM =
    (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(pin), PinMap_PWM);
uint32_t channel =
    STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(pin), PinMap_PWM));

uint32_t input_freq = 0;
volatile uint32_t lastCapture = uwTick;
volatile uint64_t resetCompareCount = 0;
volatile uint32_t timFreq = 0;
HardwareTimer SpdTIM = HardwareTimer(TIM);
int timChannel = SpdTIM.getChannel(channel);
TIM_HandleTypeDef *timHandle = SpdTIM.getHandle();

void __attribute__((optimize("O3"))) measureCallback(void) {
  SpdTIM.pause();
  uint32_t speed = (__HAL_TIM_GET_COMPARE(timHandle, timChannel) +
                    (resetCompareCount * 0x10000));
  timFreq = (input_freq / (speed + 1));
  lastCapture = uwTick;

  SpdTIM.setCount(0);
  resetCompareCount = 0;
  SpdTIM.resume();
}

void __attribute__((optimize("O3"))) resetCallback(void) {
  ++resetCompareCount;
}

void initMeasurement() {
  SpdTIM.setMode(channel, TIMER_INPUT_CAPTURE_FALLING, pin);
  uint32_t PrescalerFactor = 10;
  SpdTIM.setPrescaleFactor(PrescalerFactor);
  SpdTIM.setOverflow(0x10000);
  SpdTIM.attachInterrupt(channel, measureCallback);
  SpdTIM.attachInterrupt(resetCallback);
  SpdTIM.resume();

  // Compute this scale factor only once
  input_freq = SpdTIM.getTimerClkFreq() / PrescalerFactor;
}
uint32_t getMeasured() {
  HAL_GetTick();
  if ((timFreq && (uwTick - lastCapture > 2000))) {
    lastCapture = uwTick;
    timFreq = 0;
  }
  return timFreq;
}
