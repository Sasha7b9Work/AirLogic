#ifndef TIMER_INIT_H
#define TIMER_INIT_H
extern "C" {
#include "timer_helpers.h"
}
#include <wiring.h>

TIM_HandleTypeDef htimObj;
#define DEFINE_TIMER(_x_)                                                      \
  extern "C" void _x_##_IRQHandler(void) { HAL_TIM_IRQHandler(&htimObj); }     \
  TIM_TypeDef *_TIMER_ = (_x_);

#if defined(BL_F401)
DEFINE_TIMER(TIM9);
#elif defined(BL_MASTER_F405)
DEFINE_TIMER(TIM3);
#elif defined(BL_F051)
DEFINE_TIMER(TIM6);
#else
#error Dont know how to init timer
#endif

using callback_f = std::function<void(void)>;
callback_f TIMirqHandler = nullptr;

void TIM_Init(uint32_t freq) {

  uint32_t period = getTimerClkFreq(_TIMER_) / freq;
  uint32_t prescaler = (period / 0x10000) + 1;
  uint32_t ticks = period / prescaler;

  htimObj.Instance = _TIMER_;
  htimObj.Init.Prescaler = prescaler - 1;
  htimObj.Init.CounterMode = TIM_COUNTERMODE_UP;
  htimObj.Init.Period = ticks - 1;
  htimObj.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htimObj.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  enableTimerClock(&htimObj);
  HAL_TIM_Base_Init(&htimObj);
  HAL_NVIC_SetPriority(getTimerUpIrq(_TIMER_), 6, 6);
  HAL_NVIC_EnableIRQ(getTimerUpIrq(_TIMER_));
  if (getTimerCCIrq(_TIMER_) != getTimerUpIrq(_TIMER_)) {
    // configure Capture Compare interrupt
    HAL_NVIC_SetPriority(getTimerCCIrq(_TIMER_), 6, 6);
    HAL_NVIC_EnableIRQ(getTimerCCIrq(_TIMER_));
  }
  HAL_TIM_Base_Start_IT(&htimObj);
}

extern "C" {
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7); /*
  if (htim->Instance == _TIMER_) {
    if (TIMirqHandler != nullptr)
      TIMirqHandler();
  }
}
}
#endif