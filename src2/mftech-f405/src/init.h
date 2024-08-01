#if 1
extern "C" {
#include <malloc.h>

uint32_t freeHeap(void) {
  struct mallinfo heapInf;
  heapInf = mallinfo();
  return heapInf.fordblks;
}
uint32_t usedHeap(void) {
  struct mallinfo heapInf;
  heapInf = mallinfo();
  return heapInf.uordblks;
}
uint32_t freeRAM(void) {
  extern char *sbrk(int i);
  /* Use linker definition */
  extern char _estack;
  extern char _Min_Stack_Size;

  static char *ramend = &_estack;
  static char *minSP = (char *)(ramend - &_Min_Stack_Size);

  char *heapend = (char *)sbrk(0);
  char *stack_ptr = (char *)__get_MSP();
  struct mallinfo mi = mallinfo();
  return (((stack_ptr < minSP) ? stack_ptr : minSP) - heapend + mi.fordblks);
}
}

extern "C" {
#include "stm32yyxx_ll_pwr.h"
#include "stm32yyxx_ll_rcc.h"
#include "stm32yyxx_ll_system.h"
#include "stm32yyxx_ll_utils.h"

const static uint32_t STACK_CANARY_WORD = 0xDEADBEAFUL;
void SystemClock_Config(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);

  if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5) {
    Error_Handler();
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

  /* Wait till HSE is ready */
  while (LL_RCC_HSE_IsReady() != 1) {
  }
  LL_PWR_EnableBkUpAccess();
  LL_RCC_ForceBackupDomainReset();
  LL_RCC_ReleaseBackupDomainReset();
  LL_RCC_LSE_Enable();

  /* Wait till LSE is ready */
  while (LL_RCC_LSE_IsReady() != 1) {
  }
  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
  LL_RCC_EnableRTC();
  LL_RCC_HSE_EnableCSS();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336,
                              LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336,
                              LL_RCC_PLLQ_DIV_7);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1) {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }
  LL_SetSystemCoreClock(168000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  };
#ifdef STM32_FREERTOS_USE_CCM_HEAP
  extern char _sccmram, _eccmram;      // defined by linker script
  __HAL_RCC_CCMDATARAMEN_CLK_ENABLE(); // enable CCM clock after reset (seems
                                       // like DFU loader bug)
  memset(&_sccmram, 0, &_eccmram - &_sccmram); // fill CCM section by zeroes
#endif
}
}
#endif

/* clang-format off */
#ifdef HAL_ADC_MODULE_ENABLED
const PinMap PinMap_ADC[] = {
    // {PA_0, ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 0, 0)}, // ADC3_IN0
    // {PA_1, /*ADC2*/ ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 1, 0)}, // ADC3_IN1
    // {PA_2, /*ADC3*/ ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 2, 0)}, // ADC3_IN2
    // {PA_3, /*ADC3*/ ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 3, 0)}, // ADC3_IN3
    {PA_4, /*ADC2*/ ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 4, 0)}, // ADC2_IN4
    // {PA_5, ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 5, 0)}, // ADC1_IN5
    // {PA_6, ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 6, 0)}, // ADC1_IN6
    {PA_7, ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 7, 0)}, // ADC1_IN7
    {PB_0, ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 8, 0)}, // ADC2_IN8
    {PB_1, ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 9, 0)}, // ADC1_IN9
    {PC_0, /*ADC3*/ ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 10, 0)}, // ADC3_IN10
    {PC_1, /*ADC3*/ ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 11, 0)}, // ADC3_IN11
    {PC_2, /*ADC3*/ ADC1, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 12, 0)}, // ADC3_IN12
    {PC_3, ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 13, 0)}, // ADC1_IN13
    {PC_4, ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 14, 0)}, // ADC1_IN14
    {PC_5, /*ADC2*/ ADC2, STM_PIN_DATA_EXT(STM_MODE_ANALOG, GPIO_NOPULL, 0, 15, 0)}, // ADC2_IN15
    {NC, NP, 0}};
#endif

#ifdef HAL_UART_MODULE_ENABLED
const PinMap PinMap_UART_TX[] = {
    //{PA_0, UART4, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_UART4)},
    {PA_2, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART2)},
    {PA_9, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART1)},
    //    {PB_6, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART1)},
    //    {PB_10, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    //    {PC_6, USART6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF8_USART6)},
    {PC_10, UART4, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_UART4)},
    //    {PC_10, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    {PC_12, UART5, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_UART5)},
    //    {PD_5, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART2)},
    {PD_8, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART3)},
    {NC, NP, 0}};
#endif

#ifdef HAL_UART_MODULE_ENABLED
const PinMap PinMap_UART_RX[] = {
    //    {PA_1, UART4, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF8_UART4)},
    {PA_3, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART2)},
    {PA_10, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART1)},
    //    {PB_7, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART1)},
    //    {PB_11, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    //    {PC_7, USART6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF8_USART6)},
    {PC_11, UART4, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_UART4)},
    //    {PC_11, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    {PD_2, UART5, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_UART5)},
    //    {PD_6, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART2)},
    {PD_9, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART3)},
    {NC, NP, 0}};
#endif

#ifdef HAL_UART_MODULE_ENABLED
const PinMap PinMap_UART_RTS[] = {
    {PA_1, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART2)},
    //    {PA_12, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART1)}, {PB_14, USART3, STM_PIN_DATA(STM_MODE_AF_PP,
    //    GPIO_PULLUP, GPIO_AF7_USART3)}, {PD_4, USART2,
    //    STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART2)},
    //{PD_12, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    // GPIO_AF7_USART3)},
    {NC, NP, 0}};
#endif

#ifdef HAL_UART_MODULE_ENABLED
const PinMap PinMap_UART_CTS[] = {
    //    {PA_0, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART2)},
    //    {PA_11, USART1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART1)},
    //    {PB_13, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    //    {PD_3, USART2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART2)},
    //    {PD_11, USART3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF7_USART3)},
    {NC, NP, 0}};
#endif

//*** SPI ***

#ifdef HAL_SPI_MODULE_ENABLED
const PinMap PinMap_SPI_MOSI[] = {
    //    {PA_7, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI1)},
    {PB_5, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    //    {PB_5, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    {PB_15, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    //    {PC_3, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI2)},
    //    {PC_12, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    {NC, NP, 0}};

const PinMap PinMap_SPI_MISO[] = {
    //    {PA_6, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI1)},
    {PB_4, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    //    {PB_4, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    {PB_14, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    //    {PC_2, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI2)},
    //    {PC_11, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    {NC, NP, 0}};

const PinMap PinMap_SPI_SCLK[] = {
    //    {PA_5, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI1)},
    {PB_3, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    //    {PB_3, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    //    {PB_10, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI2)},
    {PB_13, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    //    {PC_10, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    {NC, NP, 0}};

const PinMap PinMap_SPI_SSEL[] = {
    //  {PA_4, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    //  {PA_4,  SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {PA_15, SPI1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    //    {PA_15, SPI3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF6_SPI3)},
    //    {PB_9, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP,
    //    GPIO_AF5_SPI2)},
    {PB_12, SPI2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {NC, NP, 0}};
#endif

#ifdef HAL_CAN_MODULE_ENABLED
const PinMap PinMap_CAN_RD[] = {
    {PD_0, CAN1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, GPIO_AF9_CAN1)},
    {NC, NP, 0}};

const PinMap PinMap_CAN_TD[] = {
    {PD_1, CAN1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, GPIO_AF9_CAN1)},
    {NC, NP, 0}};
#endif

#ifdef HAL_TIM_MODULE_ENABLED
const PinMap PinMap_PWM[] = {
  // {PA_0,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1
  //  {PA_0,  TIM5,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 1, 0)}, // TIM5_CH1
  //  {PA_1,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 2, 0)}, // TIM2_CH2
  // {PA_1,  TIM5,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 2, 0)}, // TIM5_CH2
  //  {PA_2,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 3, 0)}, // TIM2_CH3
  // {PA_2,  TIM5,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 3, 0)}, // TIM5_CH3
  //  {PA_2,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM9, 1, 0)}, // TIM9_CH1
  //  {PA_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 4, 0)}, // TIM2_CH4
  // {PA_3,  TIM5,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM5, 4, 0)}, // TIM5_CH4
  //  {PA_3,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM9, 2, 0)}, // TIM9_CH2
  // {PA_5,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1
  //  {PA_5,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 1, 1)}, // TIM8_CH1N
  //  {PA_6,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 1, 0)}, // TIM3_CH1
  {PA_6,  TIM13,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF9_TIM13, 1, 0)}, // TIM13_CH1
  //  {PA_7,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 1, 1)}, // TIM1_CH1N
  //  {PA_7,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 2, 0)}, // TIM3_CH2
  //  {PA_7,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 1, 1)}, // TIM8_CH1N
  // {PA_7,  TIM14,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF9_TIM14, 1, 0)}, // TIM14_CH1
  // {PA_8,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 1, 0)}, // TIM1_CH1
  // {PA_9,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 2, 0)}, // TIM1_CH2
  // {PA_10, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 0)}, // TIM1_CH3
  // {PA_11, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 4, 0)}, // TIM1_CH4
  // {PA_15, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 1, 0)}, // TIM2_CH1
  //  {PB_0,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 2, 1)}, // TIM1_CH2N
  //  {PB_0,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 3, 0)}, // TIM3_CH3
  // {PB_0,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 2, 1)}, // TIM8_CH2N
  //  {PB_1,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 1)}, // TIM1_CH3N
  //  {PB_1,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 4, 0)}, // TIM3_CH4
  // {PB_1,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 3, 1)}, // TIM8_CH3N
  // {PB_2,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 4, 0)}, // TIM2_CH4
  // {PB_3,  TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 2, 0)}, // TIM2_CH2
  // {PB_4,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 1, 0)}, // TIM3_CH1
  // {PB_5,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 2, 0)}, // TIM3_CH2
  {PB_6,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 1, 0)}, // TIM4_CH1
  {PB_7,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 2, 0)}, // TIM4_CH2
  {PB_8,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 3, 0)}, // TIM4_CH3
  //  {PB_8,  TIM10,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM10, 1, 0)}, // TIM10_CH1
  //  {PB_9,  TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 4, 0)}, // TIM4_CH4
  // {PB_9,  TIM11,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM11, 1, 0)}, // TIM11_CH1
  // {PB_10, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 3, 0)}, // TIM2_CH3
  // {PB_11, TIM2,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM2, 4, 0)}, // TIM2_CH4
  // {PB_13, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 1, 1)}, // TIM1_CH1N
  //  {PB_14, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 2, 1)}, // TIM1_CH2N
  //  {PB_14, TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 2, 1)}, // TIM8_CH2N
  // {PB_14, TIM12,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF9_TIM12, 1, 0)}, // TIM12_CH1
  //  {PB_15, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 1)}, // TIM1_CH3N
  //  {PB_15, TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 3, 1)}, // TIM8_CH3N
  // {PB_15, TIM12,  STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF9_TIM12, 2, 0)}, // TIM12_CH2
  // {PC_6,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 1, 0)}, // TIM3_CH1
  //  {PC_6,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 1, 0)}, // TIM8_CH1
  //  {PC_7,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 2, 0)}, // TIM3_CH2
  // {PC_7,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 2, 0)}, // TIM8_CH2
  // {PC_8,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 3, 0)}, // TIM3_CH3
  //  {PC_8,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 3, 0)}, // TIM8_CH3
  //  {PC_9,  TIM3,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3, 4, 0)}, // TIM3_CH4
  // {PC_9,  TIM8,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM8, 4, 0)}, // TIM8_CH4
  // {PD_12, TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 1, 0)}, // TIM4_CH1
  // {PD_13, TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 2, 0)}, // TIM4_CH2
  // {PD_14, TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 3, 0)}, // TIM4_CH3
  // {PD_15, TIM4,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM4, 4, 0)}, // TIM4_CH4
  // {PE_5,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM9, 1, 0)}, // TIM9_CH1
  // {PE_6,  TIM9,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF3_TIM9, 2, 0)}, // TIM9_CH2
  // {PE_8,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 1, 1)}, // TIM1_CH1N
  // {PE_9,  TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 1, 0)}, // TIM1_CH1
  // {PE_10, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 2, 1)}, // TIM1_CH2N
  // {PE_11, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 2, 0)}, // TIM1_CH2
  // {PE_12, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 1)}, // TIM1_CH3N
  // {PE_13, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 3, 0)}, // TIM1_CH3
  // {PE_14, TIM1,   STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF1_TIM1, 4, 0)}, // TIM1_CH4
  {NC,    NP,    0}
};
#endif

/* clang-format on */

#ifdef STM32_FREERTOS_USE_CCM_HEAP
#include <STM32FreeRTOS.h>
uint8_t ucHeap[STM32_FREERTOS_CCM_HEAP_SIZE]
    __attribute__((section(".ccmram"))) = {};
#endif

ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;
#define DMACount 10
volatile uint16_t adcReadings[DMACount] = {0};
uint16_t adcValues[DMACount] = {0};

static void configADC2(void) {
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = ENABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = DMACount;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;

  // HAL_ADC_DeInit(&hadc2);

  if (HAL_ADC_Init(&hadc2) != HAL_OK) {
    Error_Handler();
  }

  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES; // important
                                                   // channels

  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }

  sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES; // not so important channels

  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = 5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 6;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 7;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = 8;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 9;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = 10;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_ADC2_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN ADC2_MspInit 0 */

  /* USER CODE END ADC2_MspInit 0 */
  /* Peripheral clock enable */
  __HAL_RCC_ADC2_CLK_ENABLE();

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /**ADC2 GPIO Configuration
  PC0     ------> ADC2_IN10
  PC3     ------> ADC2_IN13
  PA4     ------> ADC2_IN4
  PC4     ------> ADC2_IN14
  PC5     ------> ADC2_IN15
  PB0     ------> ADC2_IN8
  PB1     ------> ADC2_IN9
  */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                        GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_4 || GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  hdma_adc2.Instance = DMA2_Stream2;
  hdma_adc2.Init.Channel = DMA_CHANNEL_1;
  hdma_adc2.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc2.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc2.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc2.Init.Mode = DMA_CIRCULAR;
  hdma_adc2.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_adc2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_adc2) != HAL_OK) {
    Error_Handler();
  }

  __HAL_LINKDMA(&hadc2, DMA_Handle, hdma_adc2);

  configADC2();
  HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adcReadings, DMACount);
}

void protect() {
#define FLASH_TIMEOUT_VALUE 50000U /* 50 s */
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
#include "debug.h"
extern DebugSerial debug;

extern "C" {
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
  if (hadc == &hadc2)
    debug.printf("ADC: Error %d!\n", hadc->ErrorCode);
}
__attribute__((optimize(3))) void DMA2_Stream2_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_adc2);
}
//volatile uint32_t dmaReadingsCounter = 0;
__attribute__((optimize(3))) void
HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc == &hadc2) {
    //++dmaReadingsCounter;
    for (uint8_t i = 0; i < DMACount; i++) {
      adcValues[i] += adcReadings[i];
      adcValues[i] >>= 1;
    }
  }
}
}
