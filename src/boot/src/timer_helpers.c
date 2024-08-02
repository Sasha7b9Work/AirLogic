#include <wiring.h>

#include "timer_helpers.h"

#if 0
uint8_t getTimerClkSrc(TIM_TypeDef *tim) {
  uint8_t clkSrc = 0;

  if (tim != (TIM_TypeDef *)NC) {
#ifdef STM32F0xx
    clkSrc = 1;
#else
    std::vector<TIM_TypeDef *> _clk1 = {
#if defined(TIM2_BASE)
      TIM2,
#endif
#if defined(TIM3_BASE)
      TIM3,
#endif
#if defined(TIM4_BASE)
      TIM4,
#endif
#if defined(TIM5_BASE)
      TIM5,
#endif
#if defined(TIM6_BASE)
      TIM6,
#endif
#if defined(TIM7_BASE)
      TIM7,
#endif
#if defined(TIM12_BASE)
      TIM12,
#endif
#if defined(TIM13_BASE)
      TIM13,
#endif
#if defined(TIM14_BASE)
      TIM14,
#endif
#if defined(TIM18_BASE)
      TIM18,
#endif
    };

    std::vector<TIM_TypeDef *> _clk2 = {
#if defined(TIM1_BASE)
      TIM1,
#endif
#if defined(TIM8_BASE)
      TIM8,
#endif
#if defined(TIM9_BASE)
      TIM9,
#endif
#if defined(TIM10_BASE)
      TIM10,
#endif
#if defined(TIM11_BASE)
      TIM11,
#endif
#if defined(TIM15_BASE)
      TIM15,
#endif
#if defined(TIM16_BASE)
      TIM16,
#endif
#if defined(TIM17_BASE)
      TIM17,
#endif
#if defined(TIM19_BASE)
      TIM19,
#endif
#if defined(TIM20_BASE)
      TIM20,
#endif
#if defined(TIM21_BASE)
      TIM21,
#endif
#if defined(TIM22_BASE)
      TIM22,
#endif
    };
    std::vector<TIM_TypeDef *>::iterator it;
    it = std::find(_clk1.begin(), _clk1.end(), tim);
    if (it != _clk1.end()) {
      clkSrc = 1;
    }
    it = std::find(_clk2.begin(), _clk2.end(), tim);
    if (it != _clk2.end()) {
      clkSrc = 2;
    }
    if (!clkSrc)
      Error_Handler();
#endif
  }
  return clkSrc;
}
#else
uint8_t getTimerClkSrc(TIM_TypeDef *tim) {
  uint8_t clkSrc = 0;

  if (tim != (TIM_TypeDef *)NC)
#ifdef STM32F0xx
    /* TIMx source CLK is PCKL1 */
    clkSrc = 1;
#else
  {
    /* Get source clock depending on TIM instance */
    switch ((uint32_t)tim) {
#if defined(TIM2_BASE)
    case (uint32_t)TIM2:
#endif
#if defined(TIM3_BASE)
    case (uint32_t)TIM3:
#endif
#if defined(TIM4_BASE)
    case (uint32_t)TIM4:
#endif
#if defined(TIM5_BASE)
    case (uint32_t)TIM5:
#endif
#if defined(TIM6_BASE)
    case (uint32_t)TIM6:
#endif
#if defined(TIM7_BASE)
    case (uint32_t)TIM7:
#endif
#if defined(TIM12_BASE)
    case (uint32_t)TIM12:
#endif
#if defined(TIM13_BASE)
    case (uint32_t)TIM13:
#endif
#if defined(TIM14_BASE)
    case (uint32_t)TIM14:
#endif
#if defined(TIM18_BASE)
    case (uint32_t)TIM18:
#endif
      clkSrc = 1;
      break;
#if defined(TIM1_BASE)
    case (uint32_t)TIM1:
#endif
#if defined(TIM8_BASE)
    case (uint32_t)TIM8:
#endif
#if defined(TIM9_BASE)
    case (uint32_t)TIM9:
#endif
#if defined(TIM10_BASE)
    case (uint32_t)TIM10:
#endif
#if defined(TIM11_BASE)
    case (uint32_t)TIM11:
#endif
#if defined(TIM15_BASE)
    case (uint32_t)TIM15:
#endif
#if defined(TIM16_BASE)
    case (uint32_t)TIM16:
#endif
#if defined(TIM17_BASE)
    case (uint32_t)TIM17:
#endif
#if defined(TIM19_BASE)
    case (uint32_t)TIM19:
#endif
#if defined(TIM20_BASE)
    case (uint32_t)TIM20:
#endif
#if defined(TIM21_BASE)
    case (uint32_t)TIM21:
#endif
#if defined(TIM22_BASE)
    case (uint32_t)TIM22:
#endif
      clkSrc = 2;
      break;
    default:
      _Error_Handler("TIM: Unknown timer instance", (int)tim);
      break;
    }
  }
#endif
  return clkSrc;
}
#endif
uint32_t getTimerClkFreq(TIM_TypeDef *_TIM) {
  RCC_ClkInitTypeDef clkconfig = {};
  uint32_t pFLatency = 0U;
  uint32_t uwTimclock = 0U, uwAPBxPrescaler = 0U;

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  switch (getTimerClkSrc(_TIM)) {
  case 1:
    uwAPBxPrescaler = clkconfig.APB1CLKDivider;
    uwTimclock = HAL_RCC_GetPCLK1Freq();
    break;
#if !defined(STM32F0xx) && !defined(STM32G0xx)
  case 2:
    uwAPBxPrescaler = clkconfig.APB2CLKDivider;
    uwTimclock = HAL_RCC_GetPCLK2Freq();
    break;
#endif
  default:
  case 0: // Unknown timer clock source
    Error_Handler();
    break;
  }

#if defined(STM32F4xx) || defined(STM32F7xx)
#if !defined(STM32F405xx) && !defined(STM32F415xx) && !defined(STM32F407xx) && \
    !defined(STM32F417xx)
  RCC_PeriphCLKInitTypeDef PeriphClkConfig = {};
  HAL_RCCEx_GetPeriphCLKConfig(&PeriphClkConfig);

  if (PeriphClkConfig.TIMPresSelection == RCC_TIMPRES_ACTIVATED)
    switch (uwAPBxPrescaler) {
    default:
    case RCC_HCLK_DIV1:
    case RCC_HCLK_DIV2:
    case RCC_HCLK_DIV4:
      uwTimclock = HAL_RCC_GetHCLKFreq();
      break;
    case RCC_HCLK_DIV8:
    case RCC_HCLK_DIV16:
      uwTimclock *= 4;
      break;
    }
  else
#endif
#endif
    switch (uwAPBxPrescaler) {
    default:
    case RCC_HCLK_DIV1:
      // uwTimclock*=1;
      break;
    case RCC_HCLK_DIV2:
    case RCC_HCLK_DIV4:
    case RCC_HCLK_DIV8:
    case RCC_HCLK_DIV16:
      uwTimclock *= 2;
      break;
    }
  return uwTimclock;
}

/**
 * @brief  This function return IRQ number corresponding to Capture or Compare
 * interrupt event of timer instance.
 * @param  tim: timer instance
 * @retval IRQ number
 */
IRQn_Type getTimerCCIrq(TIM_TypeDef *tim) {
  IRQn_Type IRQn = NonMaskableInt_IRQn;

  if (tim != (TIM_TypeDef *)NC) {
    /* Get IRQn depending on TIM instance */
    switch ((uint32_t)tim) {
#if defined(TIM1_BASE)
    case (uint32_t)TIM1_BASE:
      IRQn = TIM1_CC_IRQn;
      break;
#endif
#if defined(TIM2_BASE)
    case (uint32_t)TIM2_BASE:
      IRQn = TIM2_IRQn;
      break;
#endif
#if defined(TIM3_BASE)
    case (uint32_t)TIM3_BASE:
      IRQn = TIM3_IRQn;
      break;
#endif
#if defined(TIM4_BASE)
    case (uint32_t)TIM4_BASE:
      IRQn = TIM4_IRQn;
      break;
#endif
#if defined(TIM5_BASE)
    case (uint32_t)TIM5_BASE:
      IRQn = TIM5_IRQn;
      break;
#endif
#if defined(TIM6_BASE)
    case (uint32_t)TIM6_BASE:
      IRQn = TIM6_IRQn;
      break;
#endif
#if defined(TIM7_BASE)
    case (uint32_t)TIM7_BASE:
      IRQn = TIM7_IRQn;
      break;
#endif
#if defined(TIM8_BASE)
    case (uint32_t)TIM8_BASE:
      IRQn = TIM8_CC_IRQn;
      break;
#endif
#if defined(TIM9_BASE)
    case (uint32_t)TIM9_BASE:
      IRQn = TIM9_IRQn;
      break;
#endif
#if defined(TIM10_BASE)
    case (uint32_t)TIM10_BASE:
      IRQn = TIM10_IRQn;
      break;
#endif
#if defined(TIM11_BASE)
    case (uint32_t)TIM11_BASE:
      IRQn = TIM11_IRQn;
      break;
#endif
#if defined(TIM12_BASE)
    case (uint32_t)TIM12_BASE:
      IRQn = TIM12_IRQn;
      break;
#endif
#if defined(TIM13_BASE)
    case (uint32_t)TIM13_BASE:
      IRQn = TIM13_IRQn;
      break;
#endif
#if defined(TIM14_BASE)
    case (uint32_t)TIM14_BASE:
      IRQn = TIM14_IRQn;
      break;
#endif
#if defined(TIM15_BASE)
    case (uint32_t)TIM15_BASE:
      IRQn = TIM15_IRQn;
      break;
#endif
#if defined(TIM16_BASE)
    case (uint32_t)TIM16_BASE:
      IRQn = TIM16_IRQn;
      break;
#endif
#if defined(TIM17_BASE)
    case (uint32_t)TIM17_BASE:
      IRQn = TIM17_IRQn;
      break;
#endif
#if defined(TIM18_BASE)
    case (uint32_t)TIM18_BASE:
      IRQn = TIM18_IRQn;
      break;
#endif
#if defined(TIM19_BASE)
    case (uint32_t)TIM19_BASE:
      IRQn = TIM19_IRQn;
      break;
#endif
#if defined(TIM20_BASE)
    case (uint32_t)TIM20_BASE:
      IRQn = TIM20_CC_IRQn;
      break;
#endif
#if defined(TIM21_BASE)
    case (uint32_t)TIM21_BASE:
      IRQn = TIM21_IRQn;
      break;
#endif
#if defined(TIM22_BASE)
    case (uint32_t)TIM22_BASE:
      IRQn = TIM22_IRQn;
      break;
#endif
      break;
    default:
      _Error_Handler("TIM: Unknown timer IRQn", (int)tim);
      break;
    }
  }
  return IRQn;
}
void enableTimerClock(TIM_HandleTypeDef *htim) {
  // Enable TIM clock
#if defined(TIM1_BASE)
  if (htim->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_ENABLE();
  }
#endif
#if defined(TIM2_BASE)
  if (htim->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();
  }
#endif
#if defined(TIM3_BASE)
  if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
  }
#endif
#if defined(TIM4_BASE)
  if (htim->Instance == TIM4) {
    __HAL_RCC_TIM4_CLK_ENABLE();
  }
#endif
#if defined(TIM5_BASE)
  if (htim->Instance == TIM5) {
    __HAL_RCC_TIM5_CLK_ENABLE();
  }
#endif
#if defined(TIM6_BASE)
  if (htim->Instance == TIM6) {
    __HAL_RCC_TIM6_CLK_ENABLE();
  }
#endif
#if defined(TIM7_BASE)
  if (htim->Instance == TIM7) {
    __HAL_RCC_TIM7_CLK_ENABLE();
  }
#endif
#if defined(TIM8_BASE)
  if (htim->Instance == TIM8) {
    __HAL_RCC_TIM8_CLK_ENABLE();
  }
#endif
#if defined(TIM9_BASE)
  if (htim->Instance == TIM9) {
    __HAL_RCC_TIM9_CLK_ENABLE();
  }
#endif
#if defined(TIM10_BASE)
  if (htim->Instance == TIM10) {
    __HAL_RCC_TIM10_CLK_ENABLE();
  }
#endif
#if defined(TIM11_BASE)
  if (htim->Instance == TIM11) {
    __HAL_RCC_TIM11_CLK_ENABLE();
  }
#endif
#if defined(TIM12_BASE)
  if (htim->Instance == TIM12) {
    __HAL_RCC_TIM12_CLK_ENABLE();
  }
#endif
#if defined(TIM13_BASE)
  if (htim->Instance == TIM13) {
    __HAL_RCC_TIM13_CLK_ENABLE();
  }
#endif
#if defined(TIM14_BASE)
  if (htim->Instance == TIM14) {
    __HAL_RCC_TIM14_CLK_ENABLE();
  }
#endif
#if defined(TIM15_BASE)
  if (htim->Instance == TIM15) {
    __HAL_RCC_TIM15_CLK_ENABLE();
  }
#endif
#if defined(TIM16_BASE)
  if (htim->Instance == TIM16) {
    __HAL_RCC_TIM16_CLK_ENABLE();
  }
#endif
#if defined(TIM17_BASE)
  if (htim->Instance == TIM17) {
    __HAL_RCC_TIM17_CLK_ENABLE();
  }
#endif
#if defined(TIM18_BASE)
  if (htim->Instance == TIM18) {
    __HAL_RCC_TIM18_CLK_ENABLE();
  }
#endif
#if defined(TIM19_BASE)
  if (htim->Instance == TIM19) {
    __HAL_RCC_TIM19_CLK_ENABLE();
  }
#endif
#if defined(TIM20_BASE)
  if (htim->Instance == TIM20) {
    __HAL_RCC_TIM20_CLK_ENABLE();
  }
#endif
#if defined(TIM21_BASE)
  if (htim->Instance == TIM21) {
    __HAL_RCC_TIM21_CLK_ENABLE();
  }
#endif
#if defined(TIM22_BASE)
  if (htim->Instance == TIM22) {
    __HAL_RCC_TIM22_CLK_ENABLE();
  }
#endif
}

/**
 * @brief  This function return IRQ number corresponding to update interrupt
 * event of timer instance.
 * @param  tim: timer instance
 * @retval IRQ number
 */
IRQn_Type getTimerUpIrq(TIM_TypeDef *tim) {
  IRQn_Type IRQn = NonMaskableInt_IRQn;

  if (tim != (TIM_TypeDef *)NC) {
    /* Get IRQn depending on TIM instance */
    switch ((uint32_t)tim) {
#if defined(TIM1_BASE)
    case (uint32_t)TIM1_BASE:
      IRQn = TIM1_IRQn;
      break;
#endif
#if defined(TIM2_BASE)
    case (uint32_t)TIM2_BASE:
      IRQn = TIM2_IRQn;
      break;
#endif
#if defined(TIM3_BASE)
    case (uint32_t)TIM3_BASE:
      IRQn = TIM3_IRQn;
      break;
#endif
#if defined(TIM4_BASE)
    case (uint32_t)TIM4_BASE:
      IRQn = TIM4_IRQn;
      break;
#endif
#if defined(TIM5_BASE)
    case (uint32_t)TIM5_BASE:
      IRQn = TIM5_IRQn;
      break;
#endif
#if defined(TIM6_BASE)
    case (uint32_t)TIM6_BASE:
      IRQn = TIM6_IRQn;
      break;
#endif
#if defined(TIM7_BASE)
    case (uint32_t)TIM7_BASE:
      IRQn = TIM7_IRQn;
      break;
#endif
#if defined(TIM8_BASE)
    case (uint32_t)TIM8_BASE:
      IRQn = TIM8_IRQn;
      break;
#endif
#if defined(TIM9_BASE)
    case (uint32_t)TIM9_BASE:
      IRQn = TIM9_IRQn;
      break;
#endif
#if defined(TIM10_BASE)
    case (uint32_t)TIM10_BASE:
      IRQn = TIM10_IRQn;
      break;
#endif
#if defined(TIM11_BASE)
    case (uint32_t)TIM11_BASE:
      IRQn = TIM11_IRQn;
      break;
#endif
#if defined(TIM12_BASE)
    case (uint32_t)TIM12_BASE:
      IRQn = TIM12_IRQn;
      break;
#endif
#if defined(TIM13_BASE)
    case (uint32_t)TIM13_BASE:
      IRQn = TIM13_IRQn;
      break;
#endif
#if defined(TIM14_BASE)
    case (uint32_t)TIM14_BASE:
      IRQn = TIM14_IRQn;
      break;
#endif
#if defined(TIM15_BASE)
    case (uint32_t)TIM15_BASE:
      IRQn = TIM15_IRQn;
      break;
#endif
#if defined(TIM16_BASE)
    case (uint32_t)TIM16_BASE:
      IRQn = TIM16_IRQn;
      break;
#endif
#if defined(TIM17_BASE)
    case (uint32_t)TIM17_BASE:
      IRQn = TIM17_IRQn;
      break;
#endif
#if defined(TIM18_BASE)
    case (uint32_t)TIM18_BASE:
      IRQn = TIM18_IRQn;
      break;
#endif
#if defined(TIM19_BASE)
    case (uint32_t)TIM19_BASE:
      IRQn = TIM19_IRQn;
      break;
#endif
#if defined(TIM20_BASE)
    case (uint32_t)TIM20_BASE:
      IRQn = TIM20_IRQn;
      break;
#endif
#if defined(TIM21_BASE)
    case (uint32_t)TIM21_BASE:
      IRQn = TIM21_IRQn;
      break;
#endif
#if defined(TIM22_BASE)
    case (uint32_t)TIM22_BASE:
      IRQn = TIM22_IRQn;
      break;
#endif

    default:
      _Error_Handler("TIM: Unknown timer IRQn", (int)tim);
      break;
    }
  }
  return IRQn;
}

/**
 * @brief  Disable the timer clock
 * @param  htim: TIM handle
 * @retval None
 */
void disableTimerClock(TIM_HandleTypeDef *htim) {
  // Enable TIM clock
#if defined(TIM1_BASE)
  if (htim->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_DISABLE();
  }
#endif
#if defined(TIM2_BASE)
  if (htim->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_DISABLE();
  }
#endif
#if defined(TIM3_BASE)
  if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_DISABLE();
  }
#endif
#if defined(TIM4_BASE)
  if (htim->Instance == TIM4) {
    __HAL_RCC_TIM4_CLK_DISABLE();
  }
#endif
#if defined(TIM5_BASE)
  if (htim->Instance == TIM5) {
    __HAL_RCC_TIM5_CLK_DISABLE();
  }
#endif
#if defined(TIM6_BASE)
  if (htim->Instance == TIM6) {
    __HAL_RCC_TIM6_CLK_DISABLE();
  }
#endif
#if defined(TIM7_BASE)
  if (htim->Instance == TIM7) {
    __HAL_RCC_TIM7_CLK_DISABLE();
  }
#endif
#if defined(TIM8_BASE)
  if (htim->Instance == TIM8) {
    __HAL_RCC_TIM8_CLK_DISABLE();
  }
#endif
#if defined(TIM9_BASE)
  if (htim->Instance == TIM9) {
    __HAL_RCC_TIM9_CLK_DISABLE();
  }
#endif
#if defined(TIM10_BASE)
  if (htim->Instance == TIM10) {
    __HAL_RCC_TIM10_CLK_DISABLE();
  }
#endif
#if defined(TIM11_BASE)
  if (htim->Instance == TIM11) {
    __HAL_RCC_TIM11_CLK_DISABLE();
  }
#endif
#if defined(TIM12_BASE)
  if (htim->Instance == TIM12) {
    __HAL_RCC_TIM12_CLK_DISABLE();
  }
#endif
#if defined(TIM13_BASE)
  if (htim->Instance == TIM13) {
    __HAL_RCC_TIM13_CLK_DISABLE();
  }
#endif
#if defined(TIM14_BASE)
  if (htim->Instance == TIM14) {
    __HAL_RCC_TIM14_CLK_DISABLE();
  }
#endif
#if defined(TIM15_BASE)
  if (htim->Instance == TIM15) {
    __HAL_RCC_TIM15_CLK_DISABLE();
  }
#endif
#if defined(TIM16_BASE)
  if (htim->Instance == TIM16) {
    __HAL_RCC_TIM16_CLK_DISABLE();
  }
#endif
#if defined(TIM17_BASE)
  if (htim->Instance == TIM17) {
    __HAL_RCC_TIM17_CLK_DISABLE();
  }
#endif
#if defined(TIM18_BASE)
  if (htim->Instance == TIM18) {
    __HAL_RCC_TIM18_CLK_DISABLE();
  }
#endif
#if defined(TIM19_BASE)
  if (htim->Instance == TIM19) {
    __HAL_RCC_TIM19_CLK_DISABLE();
  }
#endif
#if defined(TIM20_BASE)
  if (htim->Instance == TIM20) {
    __HAL_RCC_TIM20_CLK_DISABLE();
  }
#endif
#if defined(TIM21_BASE)
  if (htim->Instance == TIM21) {
    __HAL_RCC_TIM21_CLK_DISABLE();
  }
#endif
#if defined(TIM22_BASE)
  if (htim->Instance == TIM22) {
    __HAL_RCC_TIM22_CLK_DISABLE();
  }
#endif
}
