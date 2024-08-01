#include "stm32yyxx_ll_pwr.h"
#include "stm32yyxx_ll_rcc.h"
#include "stm32yyxx_ll_system.h"
#include "stm32yyxx_ll_utils.h"

#include <wiring.h>

#ifndef BL_F401
DMA_HandleTypeDef hdma_spi1_tx;

void initDMA(void) {

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

  hdma_spi1_tx.Instance = DMA2_Stream3;
  hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi1_tx.Init.Mode = DMA_NORMAL;
  hdma_spi1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK) {
    Error_Handler();
  }

  __HAL_LINKDMA(&SPI._spi.handle, hdmatx, hdma_spi1_tx);

  // Link SPI and DMA
  // SPI._spi.handle.hdmatx = &hdma_spi1_tx;
  // hdma_spi1_tx.Parent = &SPI._spi.handle;
}
// HAL_SPI_MspInit();
extern "C" {
void DMA2_Stream3_IRQHandler(void) {
  refreshMutex = false;
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}
}
#endif
extern "C" {
void SystemClock_Config_HSI(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);

  if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
    Error_Handler();
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_HSI_Enable();

  /* Wait till HSI is ready */
  while (LL_RCC_HSI_IsReady() != 1) {
  }
  LL_RCC_LSI_Enable();

  /* Wait till LSI is ready */
  while (LL_RCC_LSI_IsReady() != 1) {
  }
  LL_PWR_EnableBkUpAccess();
  LL_RCC_ForceBackupDomainReset();
  LL_RCC_ReleaseBackupDomainReset();
  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
  LL_RCC_EnableRTC();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_16, 336,
                              LL_RCC_PLLP_DIV_4);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1) {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }
  LL_SetSystemCoreClock(84000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  };
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

void SystemClock_Config_HSE(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);

  if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
    Error_Handler();
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSE_Enable();

  /* Wait till HSE is ready */
  while (LL_RCC_HSE_IsReady() != 1) {
  }
  LL_RCC_LSI_Enable();

  /* Wait till LSI is ready */
  while (LL_RCC_LSI_IsReady() != 1) {
  }
  LL_PWR_EnableBkUpAccess();
  LL_RCC_ForceBackupDomainReset();
  LL_RCC_ReleaseBackupDomainReset();
  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
  LL_RCC_EnableRTC();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336,
                              LL_RCC_PLLP_DIV_4);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1) {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }
  LL_SetSystemCoreClock(84000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  };
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

void SystemClock_Config(void) { SystemClock_Config_HSE(); }

#ifdef BL_F401
SPI_HandleTypeDef hspi2;
#define _W25QXX_CS_Pin GPIO_PIN_12
#define _W25QXX_CS_GPIO_Port GPIOB

void _INT_SPI_Init(void) {
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(_W25QXX_CS_GPIO_Port, _W25QXX_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : _W25QXX_CS_Pin */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = _W25QXX_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(_W25QXX_CS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct = {0};
  /**SPI2 GPIO Configuration
  PB13     ------> SPI2_SCK
  PB14     ------> SPI2_MISO
  PB15     ------> SPI2_MOSI
  */
  GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK) {
    Error_Handler();
  }
}
#endif
}

/*
#ifdef HAL_TIM_MODULE_ENABLED
const PinMap PinMap_PWM[] = {
  //  {PA_0,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1 {PA_0,  TIM5,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 1, 0)}, // TIM5_CH1
  //  {PA_1,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM2, 2, 0)}, // TIM2_CH2 {PA_1,  TIM5,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 2, 0)}, // TIM5_CH2
  //  {PA_2,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM2, 3, 0)}, // TIM2_CH3 {PA_2,  TIM5,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 3, 0)}, // TIM5_CH3
  //  {PA_2,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF3_TIM9, 1, 0)}, // TIM9_CH1
  //  {PA_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM2, 4, 0)}, // TIM2_CH4 {PA_3,  TIM5,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 4, 0)}, // TIM5_CH4
  //  {PA_3,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF3_TIM9, 2, 0)}, // TIM9_CH2 {PA_5,  TIM2,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1
  {PA_6,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3,
1, 0)}, // TIM3_CH1
  //  {PA_7,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM1, 1, 1)}, // TIM1_CH1N {PA_7,  TIM3,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 2, 0)}, // TIM3_CH2
  {PA_8,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1,
1, 0)}, // TIM1_CH1 {PA_9,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP,
GPIO_PULLUP, GPIO_AF1_TIM1, 2, 0)}, // TIM1_CH2 {PA_10, TIM1,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 0)}, // TIM1_CH3
  {PA_11, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1,
4, 0)}, // TIM1_CH4 {PA_15, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP,
GPIO_PULLUP, GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1
  //  {PB_0,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM1, 2, 1)},  // TIM1_CH2N {PB_0,  TIM3,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 3, 0)}, // TIM3_CH3
  //  {PB_1,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF1_TIM1, 3, 1)}, // TIM1_CH3N {PB_1,  TIM3,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 4, 0)}, // TIM3_CH4
  {PB_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2,
2, 0)}, // TIM2_CH2 {PB_4,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP,
GPIO_PULLUP, GPIO_AF2_TIM3, 1, 0)}, // TIM3_CH1 {PB_5,  TIM3,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 2, 0)}, // TIM3_CH2
  {PB_6,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4,
1, 0)}, // TIM4_CH1 {PB_7,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP,
GPIO_PULLUP, GPIO_AF2_TIM4, 2, 0)}, // TIM4_CH2 {PB_8,  TIM4,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 3, 0)}, // TIM4_CH3
  //  {PB_8,  TIM10,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF3_TIM10, 1, 0)}, // TIM10_CH1 {PB_9,  TIM4,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 4, 0)}, // TIM4_CH4
  //  {PB_9,  TIM11,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP,
GPIO_AF3_TIM11, 1, 0)}, // TIM11_CH1 {PB_10, TIM2,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 3, 0)}, // TIM2_CH3
  {PB_13, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1,
1, 1)}, // TIM1_CH1N {PB_14, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP,
GPIO_PULLUP, GPIO_AF1_TIM1, 2, 1)}, // TIM1_CH2N {PB_15, TIM1,
STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 1)}, //
TIM1_CH3N {NC,    NP,    0}
};
#endif*/