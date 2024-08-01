#ifndef TIMER_HELPERS_H
#define TIMER_HELPERS_H

#if defined(TIM1_BASE) && !defined(TIM1_IRQn)
#if defined(STM32F0xx) || defined(STM32G0xx)
#define TIM1_IRQn TIM1_BRK_UP_TRG_COM_IRQn
#define TIM1_IRQHandler TIM1_BRK_UP_TRG_COM_IRQHandler
#elif defined(STM32F1xx) || defined(STM32G4xx)
#define TIM1_IRQn TIM1_UP_TIM16_IRQn
#if !defined(TIM10_BASE)
#define TIM1_IRQHandler TIM1_UP_TIM16_IRQHandler
#elif defined(TIM10_BASE)
#define TIM1_IRQHandler TIM1_UP_TIM10_IRQHandler
#endif
#elif defined(STM32F3xx) || defined(STM32L4xx) || defined(STM32WBxx)
#define TIM1_IRQn TIM1_UP_TIM16_IRQn
#define TIM1_IRQHandler TIM1_UP_TIM16_IRQHandler
#elif defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
#if !defined(TIM10_BASE)
#define TIM1_IRQn TIM1_UP_IRQn
#define TIM1_IRQHandler TIM1_UP_IRQHandler
#else
#define TIM1_IRQn TIM1_UP_TIM10_IRQn
#define TIM1_IRQHandler TIM1_UP_TIM10_IRQHandler
#endif
#elif defined(STM32H7xx) || defined(STM32MP1xx)
#define TIM1_IRQn TIM1_UP_IRQn
#define TIM1_IRQHandler TIM1_UP_IRQHandler
#endif
#endif
#if defined(TIM6_BASE) && !defined(TIM6_IRQn)
#if defined(DAC_BASE) || defined(DAC1_BASE)
#if defined(STM32G0xx)
#define TIM6_IRQn TIM6_DAC_LPTIM1_IRQn
#define TIM6_IRQHandler TIM6_DAC_LPTIM1_IRQHandler
#elif !defined(STM32F1xx) && !defined(STM32L1xx) && !defined(STM32MP1xx)
#define TIM6_IRQn TIM6_DAC_IRQn
#define TIM6_IRQHandler TIM6_DAC_IRQHandler
#endif
#endif
#endif
#if defined(TIM7_BASE) && !defined(TIM7_IRQn)
#if defined(STM32G0xx)
#define TIM7_IRQn TIM7_LPTIM2_IRQn
#define TIM7_IRQHandler TIM7_LPTIM2_IRQHandler
#elif defined(STM32G4xx)
#define TIM7_IRQn TIM7_DAC_IRQn
#define TIM7_IRQHandler TIM7_DAC_IRQHandler
#endif
#endif

#if defined(TIM8_BASE) && !defined(TIM8_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx) || defined(STM32H7xx)
#define TIM8_IRQn TIM8_UP_TIM13_IRQn
#define TIM8_IRQHandler TIM8_UP_TIM13_IRQHandler
#elif defined(STM32F3xx) || defined(STM32G4xx) || defined(STM32L4xx) ||        \
    defined(STM32MP1xx)
#define TIM8_IRQn TIM8_UP_IRQn
#define TIM8_IRQHandler TIM8_UP_IRQHandler
#endif
#endif
#if defined(TIM9_BASE) && !defined(TIM9_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx)
#define TIM9_IRQn TIM1_BRK_TIM9_IRQn
#define TIM9_IRQHandler TIM1_BRK_TIM9_IRQHandler
#endif
#endif
#if defined(TIM10_BASE) && !defined(TIM10_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx)
#define TIM10_IRQn TIM1_UP_TIM10_IRQn
// TIM10_IRQHandler is mapped on TIM1_IRQHandler  when TIM10_IRQn is not defined
#endif
#endif
#if defined(TIM11_BASE) && !defined(TIM11_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx)
#define TIM11_IRQn TIM1_TRG_COM_TIM11_IRQn
#define TIM11_IRQHandler TIM1_TRG_COM_TIM11_IRQHandler
#endif
#endif
#if defined(TIM12_BASE) && !defined(TIM12_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx) || defined(STM32H7xx)
#define TIM12_IRQn TIM8_BRK_TIM12_IRQn
#define TIM12_IRQHandler TIM8_BRK_TIM12_IRQHandler
#endif
#endif
#if defined(TIM13_BASE) && !defined(TIM13_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx) || defined(STM32H7xx)
#define TIM13_IRQn TIM8_UP_TIM13_IRQn
#endif
#endif
#if defined(TIM14_BASE) && !defined(TIM14_IRQn)
#if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F4xx) ||          \
    defined(STM32F7xx) || defined(STM32H7xx)
#define TIM14_IRQn TIM8_TRG_COM_TIM14_IRQn
#define TIM14_IRQHandler TIM8_TRG_COM_TIM14_IRQHandler
#endif
#endif
#if defined(TIM15_BASE) && !defined(TIM15_IRQn)
#if defined(STM32F1xx) || defined(STM32F3xx) || defined(STM32G4xx) ||          \
    defined(STM32L4xx)
#define TIM15_IRQn TIM1_BRK_TIM15_IRQn
#define TIM15_IRQHandler TIM1_BRK_TIM15_IRQHandler
#endif
#endif
#if defined(TIM16_BASE) && !defined(TIM16_IRQn)
#if defined(STM32F1xx) || defined(STM32F3xx) || defined(STM32G4xx) ||          \
    defined(STM32L4xx) || defined(STM32WBxx)
#define TIM16_IRQn TIM1_UP_TIM16_IRQn
// TIM16_IRQHandler is mapped on TIM1_IRQHandler when TIM16_IRQn is not defined
#endif
#endif
#if defined(TIM17_BASE) && !defined(TIM17_IRQn)
#if defined(STM32F1xx) || defined(STM32F3xx) || defined(STM32G4xx) ||          \
    defined(STM32L4xx) || defined(STM32WBxx)
#define TIM17_IRQn TIM1_TRG_COM_TIM17_IRQn
#define TIM17_IRQHandler TIM1_TRG_COM_TIM17_IRQHandler
#endif
#endif
#if defined(TIM18_BASE) && !defined(TIM18_IRQn)
#if defined(STM32F3xx)
#define TIM18_IRQn TIM18_DAC2_IRQn
#define TIM18_IRQHandler TIM18_DAC2_IRQHandler
#endif
#endif
#if defined(TIM20_BASE) && !defined(TIM20_IRQn)
#if defined(STM32F3xx) || defined(STM32G4xx)
#define TIM20_IRQn TIM20_UP_IRQn
#define TIM20_IRQHandler TIM20_UP_IRQHandler
#endif
#endif

uint8_t getTimerClkSrc(TIM_TypeDef *tim);
uint32_t getTimerClkFreq(TIM_TypeDef *_TIM);
IRQn_Type getTimerCCIrq(TIM_TypeDef *tim);
void enableTimerClock(TIM_HandleTypeDef *htim);
IRQn_Type getTimerUpIrq(TIM_TypeDef *tim);
void disableTimerClock(TIM_HandleTypeDef *htim);

#endif